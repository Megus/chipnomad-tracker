#include "chipnomad_lib.h"
#include "playback_internal.h"
#include "playback_modulation.h"
#include "playback_instruments.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define PITCH_MOD_RANGE_CENTS (2400)
#define PITCH_MOD_RANGE_PERIOD (1024)

///////////////////////////////////////////////////////////////////////////////
//
// AY-specific logic
//

int timerFunctionAY(struct SoundChip* chip, void* userdata) {
  static int counter = 0;
  static int vol = 15;

  //ChipNomadState* state = (ChipNomadState*)userdata;

  //int chipIdx = chip - state->chips;
  int period = ((chip->regs[1] << 8) | chip->regs[0]) + 1;
  counter++;
  if (counter >= period) {
    counter = 0;
    vol ^= 15;
  }

  chip->setRegister(chip, 8, vol); // Random noise value for channel A

  return 1;
}


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
  track->note.chip.ay.softPitchOffset = 0;
  track->note.chip.ay.softFineOffset = 0;
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

  // Envelope
  track->note.chip.ay.envShape = ay2->oscEnvelope.shape;
  track->note.chip.ay.envAutoN = ay2->oscEnvelope.autoEnvN;
  track->note.chip.ay.envAutoD = ay2->oscEnvelope.autoEnvD;
  track->note.chip.ay.envPeriodBase = 0;

  // Software oscillator
  track->note.chip.ay.softType = ay2->oscSoftware.type;
}

void setupInstrumentAYSample(PlaybackState* state, int trackIdx) {
  // TODO: Implement in future
}

void setupInstrumentAYWavetable(PlaybackState* state, int trackIdx) {
  // TODO: Implement in future
}

void setupInstrument(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];
  Project* p = state->p;

  if (track->note.instrument == EMPTY_VALUE_8) return;

  enum InstrumentType instType = p->instruments[track->note.instrument].type;

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
    case instAYWavetable:
      setupInstrumentAYWavetable(state, trackIdx);
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
    if (scaledVolume < 0) scaledVolume = 0;
    if (scaledVolume > 15) scaledVolume = 15;
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
        if (mod->modulation->type == modLFO) {
          // LFO: Add to volumeOffset
          track->note.volumeOffset += playbackModScaleToRange(mod->outValue, 127);
        } else {
          // ADSR/AHD: Rewrite volume
          int stopNote = 0;
          int16_t envelopeVolume = playbackApplyVolumeEnvelope(mod, 15, &stopNote);
          track->note.chip.ay.volume = envelopeVolume;
          if (stopNote) {
            track->note.pitchBase = EMPTY_VALUE_8;
          }
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
        if (mod->modulation->type == modLFO) {
          // LFO: Add to volumeOffset
          track->note.volumeOffset += playbackModScaleToRange(mod->outValue, 127);
        } else {
          // ADSR/AHD: Rewrite volume
          int stopNote = 0;
          int16_t envelopeVolume = playbackApplyVolumeEnvelope(mod, 15, &stopNote);
          track->note.chip.ay.volume = envelopeVolume;
          if (stopNote) {
            track->note.pitchBase = EMPTY_VALUE_8;
          }
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
  // TODO
}

void handleInstrumentAYWavetable(PlaybackState* state, int trackIdx) {
  // TODO
}


void outputRegistersAY(ChipNomadState* chipNomadState, int trackIdx, int chipIdx) {
  PlaybackState* state = &chipNomadState->playbackState;
  Project* p = &chipNomadState->project;
  SoundChip* chip = &chipNomadState->chips[chipIdx];
  int ayChannel = 0;

  uint8_t mixer = 0;
  uint8_t noise = EMPTY_VALUE_8;
  uint8_t envShape = 0;
  int16_t envPeriod = 0;

  for (int t = trackIdx; t < trackIdx + projectGetChipTracks(p, chipIdx); t++) {
    PlaybackTrackState* track = &state->tracks[t];

    if (track->note.pitchFinal == EMPTY_VALUE_8 || p->instruments[track->note.instrument].type == instNone) {
      // Silence channel
      chip->setRegister(chip, 8 + ayChannel, 0);
    } else {
      enum InstrumentType instType = p->instruments[track->note.instrument].type;

      // =====================
      // Calculate tone period
      // =====================

      int16_t period;
      uint8_t toneBasePitch = (track->note.chip.ay.toneFixedPitch != EMPTY_VALUE_8) ? track->note.chip.ay.toneFixedPitch : track->note.pitchFinal;
      int8_t tonePitch = toneBasePitch + track->note.chip.ay.tonePitchOffset;

      if (tonePitch < 0) tonePitch = 0;
      if (tonePitch > p->pitchTable.length - 1) tonePitch = p->pitchTable.length - 1;

      if (p->linearPitch) {
        // Linear pitch mode: convert cents to frequency, then to period
        int cents = p->pitchTable.values[tonePitch] + track->note.fineOffset + track->note.chip.ay.toneFineOffset;

        float frequency = centsToFrequency(cents);
        period = frequencyToAYPeriod(frequency, p->chipSetup.ay.clock);
      } else {
        // Traditional period mode
        period = p->pitchTable.values[tonePitch] - track->note.fineOffset - track->note.chip.ay.toneFineOffset;
      }
      period -= track->note.periodOffset;

      // Clamp period to valid AY range
      if (period < 1) period = 1;
      if (period > 4095) period = 4095;
      chip->setRegister(chip, ayChannel * 2, period & 0xff);
      chip->setRegister(chip, ayChannel * 2 + 1, (period & 0xf00) >> 8);

      // ===============
      // Volume register
      // ===============

      int volume = 0;
      if (track->note.chip.ay.envShape != 0) {
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
        } else {
          // 2. Manual envelope (AY1 style or AY2 with pitch control)
          if (instType == instAY2) {
            // AY2: Calculate envelope period from pitch
            uint8_t envBasePitch = (track->note.chip.ay.envFixedPitch != EMPTY_VALUE_8) ? track->note.chip.ay.envFixedPitch : track->note.pitchFinal;
            int8_t envPitch = envBasePitch + track->note.chip.ay.envPitchOffset;
            if (envPitch < 0) envPitch = 0;
            if (envPitch > p->pitchTable.length - 1) envPitch = p->pitchTable.length - 1;

            if (p->linearPitch) {
              int envCents = p->pitchTable.values[envPitch] + track->note.fineOffset;
              envCents += track->note.chip.ay.envFineOffset;
              float envFrequency = centsToFrequency(envCents);
              envPeriod = frequencyToAYPeriod(envFrequency, p->chipSetup.ay.clock);
            } else {
              envPeriod = p->pitchTable.values[envPitch] /* - track->note.fineOffset */;
              envPeriod -= track->note.chip.ay.envFineOffset;
            }
          } else {
            // AY1: Use manual envelope period base
            envPeriod = track->note.chip.ay.envPeriodBase;
          }
        }

        // Envelope period modification
        envPeriod -= track->note.chip.ay.envPeriodOffset;
        // Clamp envelope period to valid range
        if (envPeriod < 0) envPeriod = 0;
        if (envPeriod > 4095) envPeriod = 4095;

        volume = 16;
      } else {
        // ===============================
        // Envelope off - calculate volume
        // ===============================

        volume = track->note.volume + track->note.volumeOffset;
        if (volume < 0) volume = 0;
        if (volume > 15) volume = 15;
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
        if (volume < 0) volume = 0;
        if (volume > 15) volume = 15;
      }

      chip->setRegister(chip, 8 + ayChannel, state->trackEnabled[t] ? volume : 0);

      // =====
      // Noise
      // =====
      if ((track->note.chip.ay.mixer & 8) == 0 && track->note.chip.ay.noiseBase != EMPTY_VALUE_8) {
        noise = track->note.chip.ay.noiseBase + track->note.chip.ay.noiseOffset;
      }

      // =====
      // Mixer
      // =====
      mixer = mixer & (~(9 << ayChannel));
      mixer = mixer | ((track->note.chip.ay.mixer & 9) << ayChannel);
    }

    ayChannel++;
  }

  // Env register write
  if (envShape != 0) {
    if (envShape != state->chips[chipIdx].ay.envShape) {
      chip->setRegister(chip, 13, envShape);
      state->chips[chipIdx].ay.envShape = envShape;
    }
    chip->setRegister(chip, 11, envPeriod & 0xff);
    chip->setRegister(chip, 12, (envPeriod & 0xff00) >> 8);
  }

  // Noise register write
  if (noise != EMPTY_VALUE_8) {
    chip->setRegister(chip, 6, noise);
  }
  // Mixer register write
  chip->setRegister(chip, 7, mixer);
}

// Convert frequency to AY period
int frequencyToAYPeriod(float frequency, int clockHz) {
  if (frequency <= 0.0f) return 4095; // Avoid division by zero

  float periodf = (float)clockHz / (16.0f * frequency);
  float freqL = (float)clockHz / (16.0f * floorf(periodf));
  float freqH = (float)clockHz / (16.0f * ceilf(periodf));

  int period = (fabsf(freqL - frequency) < fabsf(freqH - frequency)) ? floorf(periodf) : ceilf(periodf);
  // Clamp to AY period range
  if (period > 4095) period = 4095;
  if (period < 0) period = 0;

  return period;
}
