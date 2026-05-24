#ifndef __PLAYBACK_CHIPS_H__
#define __PLAYBACK_CHIPS_H__

#include <stdint.h>
#include "project_instruments.h"
#include "playback_modulation.h"

typedef struct PlaybackAYNoteState {
  uint8_t mixer; // Bit 0 - tone, Bit 1 - noise

  // Volume
  PlaybackModState volumeModulation; // Legacy volume modulation (backward compatibility)
  uint8_t volume; // Current volume output from modulation (0-15)
  int8_t volumeOffset; // Volume offset from modulation sources (LFO)

  // Tone oscillator
  uint8_t toneFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int8_t tonePitchOffset;
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
  int8_t envPitchOffset;
  int16_t envFineOffset;

  // Software oscillator
  enum AYSoftwareOscType softType;
  uint8_t softFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int8_t softPitchOffset;
  int16_t softFineOffset;

  // Sample
  uint8_t sampleFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int8_t samplePitchOffset;
  int16_t sampleFineOffset;
  uint32_t samplePosition;

  // Wavetable
  uint8_t wavetableIndexOffset;
  uint8_t wavetableFixedPitch; // If not EMPTY_VALUE_8, use this fixed pitch instead of note pitch
  int8_t wavetablePitchOffset;
  int16_t wavetableFineOffset;
  uint16_t wavetablePhase;
} PlaybackAYNoteState;

typedef union PlaybackChipNoteState {
  PlaybackAYNoteState ay;
} PlaybackChipNoteState;

#endif
