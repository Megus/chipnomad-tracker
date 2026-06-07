#ifndef __PLAYBACK_CHIPS_H__
#define __PLAYBACK_CHIPS_H__

#include <stdint.h>
#include "project_instruments.h"
#include "playback_modulation.h"

extern uint8_t cnDACTableAY[16];
extern uint8_t cnDACTableYM[16];
extern uint8_t cnSampleLookupAY[256];
extern uint8_t cnSampleLookupYM[256];

typedef struct PlaybackAYNoteState {
  uint8_t mixer; // Bit 0 - tone, Bit 1 - noise

  // Volume
  PlaybackModState volumeModulation; // Legacy volume modulation (backward compatibility)
  uint8_t volume; // Current volume output from modulation (0-15)
  int8_t volumeOffset; // Volume offset from modulation sources (LFO)

  // Tone oscillator
  uint8_t toneFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int16_t tonePitchOffset;
  int16_t toneFineOffset;

  // Noise oscillator
  uint8_t noiseBase;
  int8_t noiseOffset;

  // Envelope oscillator
  uint8_t envShape;
  uint8_t envAutoN;
  uint8_t envAutoD;
  uint16_t envPeriodBase;
  int16_t envPeriodOffset;
  uint8_t envFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int16_t envPitchOffset;
  int16_t envFineOffset;

  // Software oscillator
  enum AYSoftwareOscType softType;
  uint16_t softPeriodCounter; // Counter for software oscillator timing
  uint8_t softFMPhase; // Phase for FM modulation (0/1)
  int16_t softFMPeriodOffset; // Period offset for FM modulation
  int16_t softFMDepthOffset; // FM Depth offset (for modulation)

  uint8_t softFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int16_t softPitchOffset;
  int16_t softFineOffset;
  int16_t pulseWidthOffset;
  int16_t pulseLowOffset;
  int16_t pulseWidthCurrent; // "Cached" current pulse width

  // Sample and wavetable variables
  uint32_t samplePosition;
  int8_t sampleDitherError;
  uint32_t sampleIncrement;
  int16_t wavetableIndexOffset;

  // Calculated values for the current tick. Used for timer function and visualization
  // Channel outputs
  uint8_t outVolume;
  uint16_t outTonePeriod;
  uint16_t outSoftPeriod;
  // Chip-wide outputs. Combined with other channels
  uint8_t outNoisePeriod;
  uint8_t outEnvShape;
  uint16_t outEnvPeriod;
} PlaybackAYNoteState;

typedef union PlaybackChipNoteState {
  PlaybackAYNoteState ay;
} PlaybackChipNoteState;

#endif
