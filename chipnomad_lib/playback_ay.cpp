#include "chipnomad_lib.h"
#include "playback_internal.h"
#include "playback_modulation.h"
#include "playback_instruments.h"
#include "project_constants.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define PITCH_MOD_RANGE_CENTS (2400)
#define PITCH_MOD_RANGE_PERIOD (1024)

// DAC levels for AY/YM chips. Taken from Ayumi.
static float dacTableAYfloat[32] = {
  0.0, 0.0,
  0.00999465934234, 0.00999465934234,
  0.0144502937362, 0.0144502937362,
  0.0210574502174, 0.0210574502174,
  0.0307011520562, 0.0307011520562,
  0.0455481803616, 0.0455481803616,
  0.0644998855573, 0.0644998855573,
  0.107362478065, 0.107362478065,
  0.126588845655, 0.126588845655,
  0.20498970016, 0.20498970016,
  0.292210269322, 0.292210269322,
  0.372838941024, 0.372838941024,
  0.492530708782, 0.492530708782,
  0.635324635691, 0.635324635691,
  0.805584802014, 0.805584802014,
  1.0, 1.0
};
static float dacTableYMfloat[32] = {
  0.0, 0.0,
  0.00465400167849, 0.00772106507973,
  0.0109559777218, 0.0139620050355,
  0.0169985503929, 0.0200198367285,
  0.024368657969, 0.029694056611,
  0.0350652323186, 0.0403906309606,
  0.0485389486534, 0.0583352407111,
  0.0680552376593, 0.0777752346075,
  0.0925154497597, 0.111085679408,
  0.129747463188, 0.148485542077,
  0.17666895552, 0.211551079576,
  0.246387426566, 0.281101701381,
  0.333730067903, 0.400427252613,
  0.467383840696, 0.53443198291,
  0.635172045472, 0.75800717174,
  0.879926756695, 1.0
};

uint8_t cnDACTableAY[16];
uint8_t cnDACTableYM[16];
uint8_t cnSampleLookupAY[256];
uint8_t cnSampleLookupYM[256];

// ========================================
// Timer functions for software oscillators
// =======================================

// Pulse
static void timerFunctionPulse(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAY2) return;

  // Avoid zero pulse width which can happen when there's no cached pulse width
  if (track->note.chip.ay.pulseWidthCurrent == 0 || track->note.chip.ay.softPeriodCounter == 0) {
    track->note.chip.ay.pulseWidthCurrent = clampInt(instrument->chip.ay2.oscSoftware.pulseWidth + track->note.chip.ay.pulseWidthOffset, 1, 255);
  }

  // First phase - high volume
  if (track->note.chip.ay.softPeriodCounter == 0) {
    chip->setRegister(chip, channel + 8, track->note.chip.ay.outVolume);
  }

  // Second phase - low volume
  int pulseWidth = track->note.chip.ay.pulseWidthCurrent;
  if (!state->p->chipSetup.ay.pwmFullRange) {
    pulseWidth = pulseWidth & 0xf0;
    if (pulseWidth == 0) pulseWidth = 16; // Avoid zero pulse width in 16-step mode
  }
  int phase2 = period * pulseWidth / 256;

  if (track->note.chip.ay.softPeriodCounter >= phase2) {
    int lowVolume = clampInt(instrument->chip.ay2.oscSoftware.pulseLow + track->note.chip.ay.pulseLowOffset, 0, 15);
    lowVolume = (lowVolume * track->note.chip.ay.outVolume) / 15; // Apply volume scaling
    chip->setRegister(chip, channel + 8, lowVolume);
  }
}

// Sync Tone
static void timerFunctionSyncTone(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAY2) return;

  // Reset tone phase
  uint16_t periodCounter = track->note.chip.ay.softPeriodCounter;
  if (periodCounter == 0) {
    // Set tone period to 0 to reset AY tone phase to 0/180 degrees
    chip->setRegister(chip, channel * 2, 0);
    chip->setRegister(chip, channel * 2 + 1, 0);
  } else if (periodCounter == 1) {
    // Set tone period back
    chip->setRegister(chip, channel * 2, track->note.chip.ay.outTonePeriod & 0xff);
    chip->setRegister(chip, channel * 2 + 1, (track->note.chip.ay.outTonePeriod >> 8) & 0xff);
  }
}

// Sync Envelope
static void timerFunctionSyncEnv(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAY2) return;

  // Avoid zero pulse width which can happen when there's no cached pulse width
  if (track->note.chip.ay.pulseWidthCurrent == 0 || track->note.chip.ay.softPeriodCounter == 0) {
    track->note.chip.ay.pulseWidthCurrent = clampInt(instrument->chip.ay2.oscSoftware.pulseWidth + track->note.chip.ay.pulseWidthOffset, 1, 255);
  }

  uint8_t shapePair = instrument->chip.ay2.oscSoftware.envShapePair;

  // First phase
  if (track->note.chip.ay.softPeriodCounter == 0) {
    chip->setRegister(chip, 13, (shapePair & 0xf0) ? (shapePair & 0xf0) >> 4 : track->note.chip.ay.envShape);
  }

  // Second phase
  int pulseWidth = track->note.chip.ay.pulseWidthCurrent;
  if (!state->p->chipSetup.ay.pwmFullRange) {
    pulseWidth = pulseWidth & 0xf0;
    if (pulseWidth == 0) pulseWidth = 16; // Avoid zero pulse width in 16-step mode
  }
  int phase2 = period * pulseWidth / 256;

  if (track->note.chip.ay.softPeriodCounter == phase2) {
    chip->setRegister(chip, 13, (shapePair & 0x0f) ? shapePair & 0x0f : track->note.chip.ay.envShape);
  }
}

// Samples
static void timerFunctionSample(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAYSample) return;
  if (instrument->chip.aySample.sampleData == NULL) return;

  uint8_t* dacTable;
  uint8_t* dacLUT;

  // Bounds check
  uint32_t sampleIndex = track->note.chip.ay.samplePosition >> 16;
  if (sampleIndex >= instrument->chip.aySample.sampleLength) {
    // Handle loop or stop
    uint16_t loopStart = instrument->chip.aySample.sampleLoopStart;
    if (loopStart < instrument->chip.aySample.sampleLength) {
      // Loop is active - calculate overshoot and wrap to loop start
      int32_t overshoot16 = (int32_t)track->note.chip.ay.samplePosition - ((int32_t)instrument->chip.aySample.sampleLength << 16);
      track->note.chip.ay.samplePosition = ((int32_t)loopStart << 16) + overshoot16;

      // Safety check: if still out of bounds, clamp to loop start
      // This can happen if overshoot is larger than (length - loopStart)
      if (track->note.chip.ay.samplePosition >= (instrument->chip.aySample.sampleLength << 16)) {
        track->note.chip.ay.samplePosition = (int32_t)loopStart << 16;
      }
      // Additional safety: check for negative values (shouldn't happen)
      if ((int32_t)track->note.chip.ay.samplePosition < ((int32_t)loopStart << 16)) {
        track->note.chip.ay.samplePosition = (int32_t)loopStart << 16;
      }

      // Recalculate index after loop adjustment
      sampleIndex = track->note.chip.ay.samplePosition >> 16;
    } else {
      // No loop (one-shot) - stop playback
      chip->setRegister(chip, channel + 8, 0);
      track->note.pitchBase = EMPTY_VALUE_8; // Mark note as stopped
      track->note.chip.ay.softType = aySoftwareOscNone;
      return;
    }
  }

  // Get sample value
  uint8_t sampleValue = instrument->chip.aySample.sampleData[sampleIndex];

  // Apply volume
  if (state->p->chipType == chipAY) {
    dacTable = cnDACTableAY;
    dacLUT = cnSampleLookupAY;
  } else {
    dacTable = cnDACTableYM;
    dacLUT = cnSampleLookupYM;
  }

  // Scale sample by volume (result is 0-255)
  int16_t scaledSample = (uint16_t)(sampleValue * dacTable[track->note.chip.ay.outVolume]) / 255;

  // Apply error diffusion dithering if enabled
  if (chipNomadState->aySampleDithering) {
    // Add accumulated error from previous sample
    scaledSample += track->note.chip.ay.sampleDitherError;

    // Clamp to valid range and quantize to 4-bit using lookup table
    scaledSample = clampInt(scaledSample, 0, 255);
    uint8_t quantized = dacLUT[scaledSample];

    // Calculate quantization error (difference between input and quantized output)
    // Convert quantized 4-bit value back to 8-bit for error calculation
    int16_t quantizedValue = dacTable[quantized];
    int16_t error = scaledSample - quantizedValue;

    // Store error for next sample (simple error diffusion)
    track->note.chip.ay.sampleDitherError = error;

    sampleValue = quantized;
  } else {
    // No dithering - just quantize directly
    sampleValue = dacLUT[scaledSample];
  }

  chip->setRegister(chip, channel + 8, sampleValue);

  track->note.chip.ay.samplePosition += track->note.chip.ay.sampleIncrement;
}


void timerFunctionWavetable(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAY2) return;

  int counter = track->note.chip.ay.softPeriodCounter;

  int position = (counter * 32 / period) & 0x1f; // 32 is wavetable size
  int16_t wavetableIndex = clampInt16(instrument->chip.ay2.oscSoftware.wavetableIndex + track->note.chip.ay.wavetableIndexOffset, 0, 255);
  uint8_t value = state->p->ayWavetables[wavetableIndex][position];
  uint8_t volume = (value * track->note.chip.ay.outVolume) / 15;
  chip->setRegister(chip, channel + 8, volume);
}

void timerFunctionToneFM(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAY2) return;

  int16_t tonePeriod = track->note.chip.ay.outTonePeriod;
  int16_t fmDepth = clampInt16(instrument->chip.ay2.oscSoftware.fmDepth + track->note.chip.ay.softFMDepthOffset, 0, 255);
  int16_t fmMaxOffset = tonePeriod / 3; // Gives max modulation depth of an octave
  int16_t fmOffset = (fmDepth * fmMaxOffset) / 255;
  tonePeriod = clampInt16(tonePeriod + (track->note.chip.ay.softFMPhase ? fmOffset : -fmOffset), 1, 4095);
  chip->setRegister(chip, channel * 2, tonePeriod & 0xff);
  chip->setRegister(chip, channel * 2 + 1, (tonePeriod >> 8) & 0xff);
}

void timerFunctionEnvFM(ChipNomadState *chipNomadState, struct SoundChip* chip, int channel, uint16_t period, PlaybackTrackState* track) {
  PlaybackState* state = &chipNomadState->playbackState;
  Instrument *instrument = &state->p->instruments[track->note.instrument];
  if (instrument->type != instAY2) return;

  int16_t envPeriod = track->note.chip.ay.outEnvPeriod;
  int16_t fmDepth = clampInt16(instrument->chip.ay2.oscSoftware.fmDepth + track->note.chip.ay.softFMDepthOffset, 0, 255);
  int16_t fmMaxOffset = envPeriod / 3; // Gives max modulation depth of an octave
  int16_t fmOffset = (fmDepth * fmMaxOffset) / 255;
  envPeriod = clampInt16(envPeriod + (track->note.chip.ay.softFMPhase ? fmOffset : -fmOffset), 1, 4095);
  chip->setRegister(chip, 11, envPeriod & 0xff);
  chip->setRegister(chip, 12, (envPeriod >> 8) & 0xff);
}

int timerFunctionAY(struct SoundChip* chip, void* userdata) {
  ChipNomadState* chipNomadState = (ChipNomadState*)userdata;
  PlaybackState* state = &chipNomadState->playbackState;
  int chipIdx = chip - chipNomadState->chips;
  int firstTrack = chipIdx * 3;

  for (int ch = 0; ch < 3; ch++) {
    PlaybackTrackState* track = &state->tracks[firstTrack + ch];
    if (track->note.instrument == EMPTY_VALUE_8) continue; // No instrument, skip

    Instrument *instrument = &state->p->instruments[track->note.instrument];
    uint16_t baseSoftOscPeriod = clampUInt16(track->note.chip.ay.outSoftPeriod, 1, 65535);

    if (track->note.chip.ay.softType == aySoftwareOscSyncEnvelope) {
      baseSoftOscPeriod *= 2; // SyncEnv needs to run at half speed
    }

    uint16_t softOscPeriod = baseSoftOscPeriod;

    // Soft osc period is not modulated by FM for toneFM and envFM types
    int isFMModulated = instrument->type == instAY2 && (track->note.chip.ay.softType != aySoftwareOscToneFM && track->note.chip.ay.softType != aySoftwareOscEnvFM);

    if (isFMModulated) {
      softOscPeriod = clampUInt16(baseSoftOscPeriod + track->note.chip.ay.softFMPeriodOffset, 1, 65535);
    }

    // Period reset
    if (track->note.chip.ay.softPeriodCounter >= softOscPeriod * 2) {
      track->note.chip.ay.softPeriodCounter = 0;
      track->note.chip.ay.softFMPhase ^= 1; // Toggle FM phase

      // Calculate FM period offset based on phase and FM depth
      if (isFMModulated) {
        int16_t fmDepth = clampInt16(instrument->chip.ay2.oscSoftware.fmDepth + track->note.chip.ay.softFMDepthOffset, 0, 255);
        int16_t fmMaxOffset = baseSoftOscPeriod / 3; // Gives max modulation depth of an octave
        int16_t fmOffset = (fmDepth * fmMaxOffset) / 255;

        track->note.chip.ay.softFMPeriodOffset = track->note.chip.ay.softFMPhase ? fmOffset : -fmOffset;

        softOscPeriod = clampUInt16(baseSoftOscPeriod + track->note.chip.ay.softFMPeriodOffset, 1, 65535);
      } else {
        track->note.chip.ay.softFMPeriodOffset = 0; // No FM modulation
      }
    }

    softOscPeriod *= 2; // Double the AY period value for the full wave duration

    switch (track->note.chip.ay.softType) {
      case aySoftwareOscPulse:
        timerFunctionPulse(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      case aySoftwareOscSyncTone:
        timerFunctionSyncTone(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      case aySoftwareOscSyncEnvelope:
        timerFunctionSyncEnv(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      case aySoftwareOscWavetable:
        timerFunctionWavetable(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      case aySoftwareOscToneFM:
        timerFunctionToneFM(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      case aySoftwareOscEnvFM:
        timerFunctionEnvFM(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      case aySoftwareOscSample:
        timerFunctionSample(chipNomadState, chip, ch, softOscPeriod, track);
        break;
      default:
        break;
    }

    // Period counter
    track->note.chip.ay.softPeriodCounter++;

    // If track is disabled, force output to 0
    if (state->trackEnabled[firstTrack + ch] == 0) {
      chip->setRegister(chip, ch + 8, 0);
    }
  }

  return 1;
}



// Core AY playback logic

void resetTrackAY(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  // Clear all AY playback state
  // This function is generic and works for all AY instrument types (AY1, AY2, AYSample, AYWavetable)
  memset(&track->note.chip.ay, 0, sizeof(track->note.chip.ay));

  // Set non-zero defaults for fields that need them
  track->note.chip.ay.noiseBase = EMPTY_VALUE_8;

  // Reset fixed pitches
  track->note.chip.ay.toneFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.envFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.softFixedPitch = EMPTY_VALUE_8;
}

void resetOffsetsAY(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  // Reset AY-specific offsets
  track->note.chip.ay.envPeriodOffset = 0;
  track->note.chip.ay.noiseOffset = 0;
  track->note.chip.ay.tonePitchOffset = 0;
  track->note.chip.ay.toneFineOffset = 0;
  track->note.chip.ay.envPitchOffset = 0;
  track->note.chip.ay.envFineOffset = 0;
  track->note.chip.ay.softFMDepthOffset = 0;
  track->note.chip.ay.softPitchOffset = 0;
  track->note.chip.ay.softFineOffset = 0;
  track->note.chip.ay.pulseWidthOffset = 0;
  track->note.chip.ay.pulseLowOffset = 0;
  track->note.chip.ay.wavetableIndexOffset = 0;
  track->note.chip.ay.volumeOffset = 0;
}

void setupInstrumentAY1(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;

  // Mixer
  uint8_t defaultMixer = p->instruments[track->note.instrument].chip.ay.defaultMixer;
  uint8_t mixerValue = ~(defaultMixer & 0x0F);
  track->note.chip.ay.mixer = (mixerValue & 0x1) + ((mixerValue & 0x2) << 2);

  // Noise
  track->note.chip.ay.noiseBase = EMPTY_VALUE_8;

  // Reset fixed pitches
  track->note.chip.ay.toneFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.envFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.softFixedPitch = EMPTY_VALUE_8;

  // Envelope
  track->note.chip.ay.envShape = (defaultMixer >> 4) & 0x0F;
  track->note.chip.ay.envAutoN = p->instruments[track->note.instrument].chip.ay.autoEnvN;
  track->note.chip.ay.envAutoD = p->instruments[track->note.instrument].chip.ay.autoEnvD;
  track->note.chip.ay.envPeriodBase = 0;

  // Initialize AY1 legacy volume modulation (backward compatibility)
  // AY1 uses the dedicated volumeModulation field in addition to the 4 generic modulation slots
  Modulation* volumeMod = &p->instruments[track->note.instrument].chip.ay.volumeEnvelope;
  playbackModInit(&track->note.chip.ay.volumeModulation, volumeMod);
  // Scale sustain from 0-15 (AY volume range) to 0-255 (modulation range) with rounding
  // Original sustain is in p3, we need to scale it
  // We can't modify the instrument data, so we use p3Offset
  int16_t scaledSustain = (volumeMod->p3 * 255 + 7) / 15;
  track->note.chip.ay.volumeModulation.p3Offset = scaledSustain - volumeMod->p3;

  // Software oscillator (not available for AY1)
  track->note.chip.ay.softType = aySoftwareOscNone;
}

void setupInstrumentAY2(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;
  InstrumentAY2* ay2 = &p->instruments[track->note.instrument].chip.ay2;

  // Mixer
  uint8_t mixer = 0;
  if (!ay2->oscTone.isOn) mixer |= 1;
  if (!ay2->oscNoise.isOn) mixer |= 8;
  track->note.chip.ay.mixer = mixer;

  // Noise
  track->note.chip.ay.noiseBase = ay2->oscNoise.isOn ? ay2->oscNoise.noisePeriod : EMPTY_VALUE_8;

  // Reset fixed pitches
  track->note.chip.ay.toneFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.envFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.softFixedPitch = EMPTY_VALUE_8;

  // Envelope
  track->note.chip.ay.envShape = ay2->oscEnvelope.shape;
  track->note.chip.ay.envAutoN = ay2->oscEnvelope.autoEnvN;
  track->note.chip.ay.envAutoD = ay2->oscEnvelope.autoEnvD;
  track->note.chip.ay.envPeriodBase = 0;

  // Software oscillator
  if (track->note.chip.ay.softType != ay2->oscSoftware.type) {
    // If changing softType, reset soft osc state to avoid glitches
    track->note.chip.ay.softPeriodCounter = 0;
    track->note.chip.ay.softFMPhase = 0;
  }
  track->note.chip.ay.softType = ay2->oscSoftware.type;
}

void setupInstrumentAYSample(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;
  InstrumentAYSample* aySample = &p->instruments[track->note.instrument].chip.aySample;

  // Mixer
  uint8_t mixer = 0;
  if (!aySample->oscTone.isOn) mixer |= 1;
  if (!aySample->oscNoise.isOn) mixer |= 8;
  track->note.chip.ay.mixer = mixer;

  // Noise
  track->note.chip.ay.noiseBase = aySample->oscNoise.isOn ? aySample->oscNoise.noisePeriod : EMPTY_VALUE_8;

  // Reset fixed pitches
  track->note.chip.ay.toneFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.envFixedPitch = EMPTY_VALUE_8;
  track->note.chip.ay.softFixedPitch = EMPTY_VALUE_8;

  // Envelope (always off for sample instruments)
  track->note.chip.ay.envShape = 0;
  track->note.chip.ay.envAutoN = 0;
  track->note.chip.ay.envAutoD = 0;
  track->note.chip.ay.envPeriodBase = 0;

  // Software oscillator (always sample type)
  track->note.chip.ay.softType = aySoftwareOscSample;
  track->note.chip.ay.softPeriodCounter = 0;
  track->note.chip.ay.samplePosition = aySample->sampleStart << 16; // Sample position is 16.16 fixed point
  track->note.chip.ay.sampleDitherError = 0;
}

void setupInstrument(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;

  if (track->note.instrument == EMPTY_VALUE_8) return;

  enum InstrumentType instType = (enum InstrumentType)p->instruments[track->note.instrument].type;

  switch (instType) {
    case instAY1:
      setupInstrumentAY1(state, trackIdx);
      break;
    case instAY2:
      setupInstrumentAY2(state, trackIdx);
      break;
    case instAYSample:
      setupInstrumentAYSample(state, trackIdx);
      break;
    case instNone:
      // No setup needed
      break;
  }
}

void handleInstrumentAY1(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;

  // Process AY1 legacy volume modulation (backward compatibility)
  if (track->note.chip.ay.volumeModulation.modulation) {
    playbackModNext(&track->note.chip.ay.volumeModulation);
  }

  // Initialize volume to full (15) before applying modulations
  track->note.chip.ay.volume = 15;

  // Apply AY1 legacy volume modulation (backward compatibility)
  if (track->note.chip.ay.volumeModulation.modulation) {
    int16_t scaledVolume = playbackModScaleToRange(track->note.chip.ay.volumeModulation.outValue, 15);
    scaledVolume = clampInt16(scaledVolume, 0, 15);
    track->note.chip.ay.volume = (uint8_t)scaledVolume;
    // Check if note should be stopped (release phase complete)
    if (track->note.chip.ay.volumeModulation.step == 0xff) {
      track->note.pitchBase = EMPTY_VALUE_8;
    }
  }

  // Apply all 4 generic modulation slots
  // AY1 destinations: 1=Volume, 2=TonePitch, 3=Noise, 4=EnvPeriod
  for (int i = 0; i < 4; i++) {
    PlaybackModState* mod = &track->note.modulation[i];
    if (!mod->modulation || mod->modulation->destination == 0) continue;

    switch (mod->modulation->destination) {
      case 1: { // Volume
        if (playbackApplyVolumeModulation(mod, &track->note.volumeOffset, &track->note.chip.ay.volume, 15)) {
          track->note.pitchBase = EMPTY_VALUE_8;
        }
        break;
      }
      case 2: { // Pitch
        track->note.fineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 3: { // Noise
        track->note.chip.ay.noiseOffset += playbackModScaleToRange(mod->outValue, 127);
        break;
      }
      case 4: { // Env Period
        track->note.chip.ay.envPeriodOffset += playbackModScaleToRange(mod->outValue, 256);
        break;
      }
    }
  }

}

void handleInstrumentAY2(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;

  // Initialize volume to full (15) before applying modulations
  track->note.chip.ay.volume = 15;

  // Apply all 4 generic modulation slots
  // AY2 destinations: 1=Volume, 2=TonePitch, 3=Noise, 4=EnvPitch, 5=SoftPitch
  for (int i = 0; i < 4; i++) {
    PlaybackModState* mod = &track->note.modulation[i];
    if (!mod->modulation || mod->modulation->destination == 0) continue;

    switch (mod->modulation->destination) {
      case 1: { // Volume
        if (playbackApplyVolumeModulation(mod, &track->note.volumeOffset, &track->note.chip.ay.volume, 15)) {
          track->note.pitchBase = EMPTY_VALUE_8;
        }
        break;
      }
      case 2: { // Pitch
        track->note.fineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 3: { // Tone Pitch
        track->note.chip.ay.toneFineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 4: { // Noise
        track->note.chip.ay.noiseOffset += playbackModScaleToRange(mod->outValue, 127);
        break;
      }
      case 5: { // Env Pitch
        track->note.chip.ay.envFineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : 256);
        break;
      }
      case 6: { // Software Oscillator Pitch
        track->note.chip.ay.softFineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 7: { // Software FM Depth
        track->note.chip.ay.softFMDepthOffset += playbackModScaleToRange(mod->outValue, 255);
        break;
      }
      case 8: { // Pulse Width
        track->note.chip.ay.pulseWidthOffset += playbackModScaleToRange(mod->outValue, 127);
        break;
      }
      case 9: { // Pulse Low
        track->note.chip.ay.pulseLowOffset += playbackModScaleToRange(mod->outValue, 127);
        break;
      }
      case 10: { // Wavetable index
        track->note.chip.ay.wavetableIndexOffset += playbackModScaleToRange(mod->outValue, 127);
        break;
      }
    }
  }

  // Offsets from instrument parameters
  track->note.chip.ay.tonePitchOffset += p->instruments[track->note.instrument].chip.ay2.oscTone.pitchOffset;
  track->note.chip.ay.toneFineOffset += p->instruments[track->note.instrument].chip.ay2.oscTone.fineTune;
  track->note.chip.ay.envPitchOffset += p->instruments[track->note.instrument].chip.ay2.oscEnvelope.pitchOffset;
  track->note.chip.ay.envFineOffset += p->instruments[track->note.instrument].chip.ay2.oscEnvelope.fineTune;
  track->note.chip.ay.softPitchOffset += p->instruments[track->note.instrument].chip.ay2.oscSoftware.pitchOffset;
  track->note.chip.ay.softFineOffset += p->instruments[track->note.instrument].chip.ay2.oscSoftware.fineTune;
}

void handleInstrumentAYSample(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;

  // Initialize volume to full (15) before applying modulations
  track->note.chip.ay.volume = 15;

  // Apply all 4 generic modulation slots
  // AYSample destinations: 1=Volume, 2=Pitch, 3=SmplPit, 4=TonePit, 5=Noise
  for (int i = 0; i < 4; i++) {
    PlaybackModState* mod = &track->note.modulation[i];
    if (!mod->modulation || mod->modulation->destination == 0) continue;

    switch (mod->modulation->destination) {
      case 1: { // Volume
        if (playbackApplyVolumeModulation(mod, &track->note.volumeOffset, &track->note.chip.ay.volume, 15)) {
          track->note.pitchBase = EMPTY_VALUE_8;
        }
        break;
      }
      case 2: { // Pitch (main pitch)
        track->note.fineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 3: { // Sample Pitch
        track->note.chip.ay.softFineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 4: { // Tone Pitch
        track->note.chip.ay.toneFineOffset += playbackModScaleToRange(mod->outValue, p->linearPitch ? PITCH_MOD_RANGE_CENTS : PITCH_MOD_RANGE_PERIOD);
        break;
      }
      case 5: { // Noise
        track->note.chip.ay.noiseOffset += playbackModScaleToRange(mod->outValue, 127);
        break;
      }
    }
  }

  // Offsets from instrument parameters
  InstrumentAYSample* aySample = &p->instruments[track->note.instrument].chip.aySample;
  track->note.chip.ay.tonePitchOffset += aySample->oscTone.pitchOffset;
  track->note.chip.ay.toneFineOffset += aySample->oscTone.fineTune;
  track->note.chip.ay.softPitchOffset += aySample->pitchOffset;
  track->note.chip.ay.softFineOffset += aySample->fineTune;
}

void outputRegistersAY(ChipNomadState* chipNomadState, int trackIdx, int chipIdx) {
  PlaybackState* state = &chipNomadState->playbackState;
  Project* p = &chipNomadState->project;
  SoundChip* chip = &chipNomadState->chips[chipIdx];
  int ayChannel = 0;

  // Tracking chip-wide reg values
  uint8_t mixer = 0;
  uint8_t noise = 0;
  uint8_t envShape = 0;
  int16_t envPeriod = 0;

  // Write flags for chip-wide registers
  int shouldWriteMixer = 1; // Always write mixer, but have the variable just in case
  int shouldWriteNoise = 0;
  int shouldWriteEnvPeriod = 0;
  int hasSoftOsc = 0;

  for (int t = trackIdx; t < trackIdx + projectGetChipTracks(p, chipIdx); t++) {
    // Write flags for channel registers
    int shouldWriteTonePeriod = 0;
    int shouldWriteVolume = 0;

    PlaybackTrackState* track = &state->tracks[t];

    if (track->note.pitchFinal == EMPTY_VALUE_8 || p->instruments[track->note.instrument].type == instNone) {
      // Silence channel
      chip->setRegister(chip, 8 + ayChannel, 0);
      track->note.chip.ay.softType = aySoftwareOscNone; // Ensure soft osc is also silenced
    } else {
      enum InstrumentType instType = (enum InstrumentType)p->instruments[track->note.instrument].type;

      // =====================
      // Calculate tone period
      // =====================

      uint8_t toneBasePitch = (track->note.chip.ay.toneFixedPitch != EMPTY_VALUE_8) ? track->note.chip.ay.toneFixedPitch : track->note.pitchFinal;
      int16_t period = calculateAYPeriod(p, toneBasePitch, track->note.chip.ay.tonePitchOffset,
                                         track->note.fineOffset, track->note.chip.ay.toneFineOffset,
                                         track->note.periodOffset, 1);
      track->note.chip.ay.outTonePeriod = period;

      // Shouldn't write only when soft osc type is SyncTone ot ToneFM, timer function will handle it
      if (!(track->note.chip.ay.softType == aySoftwareOscSyncTone || track->note.chip.ay.softType == aySoftwareOscToneFM)) {
        shouldWriteTonePeriod = 1;
      }

      // ===============================
      // Calculate software osc period
      // ===============================
      // Software oscillator period calculation is identical to tone period
      // (uses fineOffset in both linear and period modes)

      if (track->note.chip.ay.softType != aySoftwareOscNone) {
        uint8_t softBasePitch = (track->note.chip.ay.softFixedPitch != EMPTY_VALUE_8) ? track->note.chip.ay.softFixedPitch : track->note.pitchFinal;
        track->note.chip.ay.outSoftPeriod = calculateAYPeriod(p, softBasePitch, track->note.chip.ay.softPitchOffset,
          track->note.fineOffset, track->note.chip.ay.softFineOffset, track->note.periodOffset, 1);

        // Calculate sample increment if we're playing a sample
        if (track->note.chip.ay.softType == aySoftwareOscSample) {
            // Safety check: ensure instrument is valid and is a sample type
            if (track->note.instrument != EMPTY_VALUE_8 &&
                p->instruments[track->note.instrument].type == instAYSample) {
              uint16_t period = track->note.chip.ay.outSoftPeriod;
              if (period == 0) period = 1; // Avoid division by zero

              uint32_t sampleRate = p->instruments[track->note.instrument].chip.aySample.sampleRate;
              // Calculate sample increment. 26163 is the reference frequency (C-4)
              uint64_t numerator = ((uint64_t)sampleRate * 50) << 16;
              uint32_t denominator = (uint32_t)period * 26163;
              track->note.chip.ay.sampleIncrement = (uint32_t)(numerator / denominator);
            }
        }

        hasSoftOsc = 1;
      }

      // ===============
      // Volume register
      // ===============

      int volume = 0;
      // Envelope is on only when envShape is non-zero and software oscillator type is
      // not Pulse, Wavetable, or Sample, because those types modulate the volume
      if (track->note.chip.ay.envShape != 0 && (track->note.chip.ay.softType != aySoftwareOscPulse && track->note.chip.ay.softType != aySoftwareOscWavetable && track->note.chip.ay.softType != aySoftwareOscSample)) {
        // ===========
        // Envelope on
        // ===========
        envShape = track->note.chip.ay.envShape;

        // Env period calculation
        if (track->note.chip.ay.envAutoN != 0) {
          // 1. Auto envelope (based on tone period)
          int n = track->note.chip.ay.envAutoN;
          int d = track->note.chip.ay.envAutoD;
          if (d == 0) d = 1; // Just in case, to avoid division by zero
          int tempPeriod = period * n / d;
          envPeriod = ((tempPeriod & 0xf) >= 8) ? (tempPeriod >> 4) + 1 : (tempPeriod >> 4);
          envPeriod -= track->note.chip.ay.envPeriodOffset;
        } else {
          // 2. Manual envelope (AY1 style or AY2 with pitch control)
          if (instType == instAY2) {
            // AY2: Calculate envelope period from pitch
            uint8_t envBasePitch = (track->note.chip.ay.envFixedPitch != EMPTY_VALUE_8) ? track->note.chip.ay.envFixedPitch : track->note.pitchFinal;
            // Note: envelope uses fineOffset in linear mode only (not in period mode)
            envPeriod = calculateAYPeriod(p, envBasePitch, track->note.chip.ay.envPitchOffset,
                                          track->note.fineOffset, track->note.chip.ay.envFineOffset,
                                          track->note.chip.ay.envPeriodOffset, p->linearPitch);
          } else {
            // AY1: Use manual envelope period base
            envPeriod = track->note.chip.ay.envPeriodBase;
            envPeriod -= track->note.chip.ay.envPeriodOffset;
            // Clamp envelope period to valid range
            envPeriod = clampInt(envPeriod, 0, 4095);
          }
        }

        // Note: For AY2, clamping is done inside calculateAYPeriod
        // For AY1, clamping is done above after applying envPeriodOffset
        track->note.chip.ay.outEnvPeriod = envPeriod;
        shouldWriteEnvPeriod = 1;
        volume = 16;
      } else {
        // ===============================
        // Envelope off - calculate volume
        // ===============================

        volume = clampInt(track->note.volume + track->note.volumeOffset, 0, 15);
        volume *= track->note.chip.ay.volume; // Instrument volume

        // Instrument table volume
        int tableIdx = track->note.instrumentTable.tableIdx;
        if (tableIdx != EMPTY_VALUE_8 && p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].volume != EMPTY_VALUE_8) {
          volume *= p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].volume;
        } else {
          volume *= 15;
        }

        // Aux table volume
        tableIdx = track->note.auxTable.tableIdx;
        if (tableIdx != EMPTY_VALUE_8 && p->tables[tableIdx].rows[track->note.auxTable.rows[0]].volume != EMPTY_VALUE_8) {
          volume *= p->tables[tableIdx].rows[track->note.auxTable.rows[0]].volume;
        } else {
          volume *= 15;
        }

        // Normalize volume
        volume = volume / (15 * 15 * 15);

        // Sanity limits (even though it should never happen)
        volume = clampInt(volume, 0, 15);
      }

      track->note.chip.ay.outVolume = volume;

      // Pulse, Wavetable, and Sample modulate volume. They will use outVolume to scale their output
      // Don't output volume, timer function will handle it
      if (!(track->note.chip.ay.softType == aySoftwareOscPulse || track->note.chip.ay.softType == aySoftwareOscWavetable || track->note.chip.ay.softType == aySoftwareOscSample)) {
        shouldWriteVolume = 1;
      }

      // =====
      // Noise
      // =====
      if ((track->note.chip.ay.mixer & 8) == 0 && track->note.chip.ay.noiseBase != EMPTY_VALUE_8) {
        noise = track->note.chip.ay.noiseBase + track->note.chip.ay.noiseOffset;
        shouldWriteNoise = 1;
      }

      // =====
      // Mixer
      // =====
      mixer = mixer & (~(9 << ayChannel));
      mixer = mixer | ((track->note.chip.ay.mixer & 9) << ayChannel);
    }

    // Channel register writes
    if (shouldWriteTonePeriod) {
      chip->setRegister(chip, ayChannel * 2, track->note.chip.ay.outTonePeriod & 0xff);
      chip->setRegister(chip, ayChannel * 2 + 1, (track->note.chip.ay.outTonePeriod & 0xf00) >> 8);
    }
    if (shouldWriteVolume) {
      chip->setRegister(chip, 8 + ayChannel, state->trackEnabled[t] ? track->note.chip.ay.outVolume : 0);
    }

    ayChannel++;
  }

  // Chip-wide register writes

  // Env register write
  if (envShape != 0) {
    if (envShape != state->chips[chipIdx].ay.envShape) {
      chip->setRegister(chip, 13, envShape);
      state->chips[chipIdx].ay.envShape = envShape;
    }
    if (shouldWriteEnvPeriod) {
      chip->setRegister(chip, 11, envPeriod & 0xff);
      chip->setRegister(chip, 12, (envPeriod & 0xff00) >> 8);
    }
  }

  // Noise register write
  if (shouldWriteNoise) {
    chip->setRegister(chip, 6, noise);
  }
  // Mixer register write
  if (shouldWriteMixer) {
    chip->setRegister(chip, 7, mixer);
  }
  // Soft osc
  if (hasSoftOsc) {
    chip->setTimerFunc(chip, timerFunctionAY, chipNomadState);
  } else {
    chip->setTimerFunc(chip, NULL, NULL);
  }
}

// Convert frequency to AY period
int frequencyToAYPeriod(float frequency, int clockHz) {
  if (frequency <= 0.0f) return 4095; // Avoid division by zero

  float periodf = (float)clockHz / (16.0f * frequency);
  float freqL = (float)clockHz / (16.0f * floorf(periodf));
  float freqH = (float)clockHz / (16.0f * ceilf(periodf));

  int period = (fabsf(freqL - frequency) < fabsf(freqH - frequency)) ? floorf(periodf) : ceilf(periodf);
  // Clamp to AY period range
  return clampInt(period, 0, 4095);
}

// Calculate AY period from pitch with offsets
// This function handles both linear (cents-based) and traditional (period-based) pitch modes
int16_t calculateAYPeriod(Project* p, uint8_t basePitch, int8_t pitchOffset, int16_t fineOffset,
                          int16_t specificFineOffset, int16_t periodOffset, int useFineOffset) {
  // Apply coarse pitch offset and clamp to pitch table range
  int8_t pitch = clampInt8(basePitch + pitchOffset, 0, p->pitchTable.length - 1);

  int16_t period;

  if (p->linearPitch) {
    // Linear pitch mode: convert cents to frequency, then to period
    int cents = p->pitchTable.values[pitch];
    if (useFineOffset) {
      cents += fineOffset;
    }
    cents += specificFineOffset;

    float frequency = centsToFrequency(cents);
    period = frequencyToAYPeriod(frequency, p->chipSetup.ay.clock);
  } else {
    // Traditional period mode
    period = p->pitchTable.values[pitch];
    if (useFineOffset) {
      period -= fineOffset;
    }
    period -= specificFineOffset;
  }

  // Apply period offset
  period -= periodOffset;

  // Clamp period to valid AY range
  period = clampInt16(period, 1, 4095);

  return period;
}

// Initialize additional tables for sample playback
void initAYSampleTables(void) {
  // Fill 8-bit DAC tables
  for (int i = 0; i < 16; i++) {
    cnDACTableAY[i] = (uint8_t)(dacTableAYfloat[i * 2 + 1] * 255);
    cnDACTableYM[i] = (uint8_t)(dacTableYMfloat[i * 2 + 1] * 255);
  }

  // Create sample LUTs by mapping 8-bit sample values to 4-bit AY volume levels
  uint8_t volumeAY = 0;
  uint8_t volumeYM = 0;
  for (int i = 0; i < 256; i++) {
    if (volumeAY < 15 && abs(i - cnDACTableAY[volumeAY + 1]) <= abs(i - cnDACTableAY[volumeAY])) volumeAY++;
    if (volumeYM < 15 && abs(i - cnDACTableYM[volumeYM + 1]) <= abs(i - cnDACTableYM[volumeYM])) volumeYM++;

    cnSampleLookupAY[i] = volumeAY;
    cnSampleLookupYM[i] = volumeYM;
  }
}
