#ifndef __PLAYBACK_MODULATION_H__
#define __PLAYBACK_MODULATION_H__

#include <stdint.h>
#include "project_instruments.h"

typedef struct PlaybackModState {
  enum ModulationType type;
  uint8_t destination;
  int8_t amount;
  uint8_t p1;
  uint8_t p2;
  uint8_t p3;
  uint8_t p4;
  uint8_t step; // Internal mod step, e.g. AHD/ADSR step. 0xff - stopped
  uint8_t counter;
  int16_t data1;  // ADSR: From
  int16_t data2;  // ADSR: To
  int16_t outValue; // Read calculated value from this field (16-bit signed range)
} PlaybackModState;

void playbackModInit(PlaybackModState* state, Modulation* mod);
void playbackModNext(PlaybackModState* state);
void playbackModNoteOff(PlaybackModState* state);

// Helper function to scale modulation output to a specific range
// maxAmplitude: maximum value for the target parameter (e.g., 15 for AY volume)
// Returns scaled value in range [0, maxAmplitude]
int16_t playbackModScaleToRange(int16_t modValue, int16_t maxAmplitude);

#endif