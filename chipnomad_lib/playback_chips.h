#ifndef __PLAYBACK_CHIPS_H__
#define __PLAYBACK_CHIPS_H__

typedef struct PlaybackAYNoteState {
  uint8_t mixer; // bit 0 - Tone, bit 1 - Noise, bit 2 - Envelope

  // Legacy for AY1. Being replaced with modulation
  uint8_t adsrStep;
  uint8_t adsrCounter;
  uint8_t adsrFrom;
  uint8_t adsrTo;
  uint8_t adsrVolume;

  // Envelope
  uint8_t envShape;
  uint8_t envAutoN;
  uint8_t envAutoD;
  uint16_t envBase;
  int16_t envOffset;

  // Noise
  uint8_t noiseBase;
  int8_t noiseOffset;

  // TODO: Additional values for AY2, AYSample, AYWavetable
  int8_t toneOffset;
  int8_t toneFine;
  int8_t envFine;

} PlaybackAY1NoteState;

typedef union PlaybackChipNoteState {
  PlaybackAY1NoteState ay;
} PlaybackChipNoteState;

#endif
