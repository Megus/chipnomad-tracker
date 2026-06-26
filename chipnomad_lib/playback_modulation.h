#ifndef __PLAYBACK_MODULATION_H__
#define __PLAYBACK_MODULATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "project_instruments.h"

typedef struct PlaybackModState {
  const Modulation* modulation; // Pointer to the source Modulation struct
  int16_t amountOffset;
  int16_t p1Offset;
  int16_t p2Offset;
  int16_t p3Offset;
  int16_t p4Offset;
  uint8_t step; // Internal mod step, e.g. AHD/ADSR step. 0xff - modulation is stopped
  uint8_t counter; // Internal tick counter
  int16_t data1;  // ADSR: From
  int16_t data2;  // ADSR: To
  int16_t outValue; // Read calculated value from this field (16-bit signed range)

  // Cached values to detect changes that require reinitialization
  enum ModulationType cachedType;
  uint8_t cachedP2; // For LFO trigger mode detection
} PlaybackModState;

void playbackModInit(PlaybackModState* state, Modulation* mod);
void playbackModNext(PlaybackModState* state);
void playbackModNoteOff(PlaybackModState* state);

// Helper function to scale modulation output to a specific range
// modValue: input in range [-32385, 32385] (255 * 127)
// maxAmplitude: maximum value for the target parameter (e.g., 15 for AY volume)
// Returns scaled value in range [-maxAmplitude, maxAmplitude] with rounding
int16_t playbackModScaleToRange(int16_t modValue, int16_t maxAmplitude);


#ifdef __cplusplus
}
#endif

#endif
