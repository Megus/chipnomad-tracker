#ifndef __PLAYBACK_CHIPS_H__
#define __PLAYBACK_CHIPS_H__

#include <stdint.h>
#include "project_instruments.h"

typedef struct PlaybackAYNoteState {
  uint8_t mixer; // bit 0 - Tone, bit 1 - Noise, bit 2 - Envelope

  // Legacy for AY1. Being replaced with modulation
  uint8_t adsrStep;
  uint8_t adsrCounter;
  uint8_t adsrFrom;
  uint8_t adsrTo;
  uint8_t adsrVolume;

  // Tone
  int8_t tonePitchOffset;
  int16_t toneFineOffset;

  // Noise
  uint8_t noiseBase;
  int8_t noiseOffset;

  // Envelope
  uint8_t envShape;
  uint8_t envAutoN;
  uint8_t envAutoD;
  uint16_t envPeriodBase;
  int16_t envPeriodOffset;
  int8_t envPitchOffset;
  int16_t envFineOffset;

  // Software oscillator
  enum AYSoftwareOscType softType;
  int8_t softPitchOffset;
  int16_t softFineOffset;

} PlaybackAY1NoteState;

typedef union PlaybackChipNoteState {
  PlaybackAY1NoteState ay;
} PlaybackChipNoteState;

#endif
