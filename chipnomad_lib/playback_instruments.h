#ifndef __PLAYBACK_INSTRUMENTS_H__
#define __PLAYBACK_INSTRUMENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "playback.h"
#include "playback_modulation.h"

struct PlaybackInstrument {
  void (*init)(PlaybackState* state, int trackIdx);
  void (*handle)(PlaybackState* state, int trackIdx);

};

// Apply ADSR/AHD volume modulation and return the resulting volume (0-maxVolume)
// Returns -1 if the modulation is not ADSR/AHD (e.g., LFO)
// maxVolume: maximum volume value (e.g., 15 for AY chips)
// stopNote: set to 1 if the note should be stopped (ADSR release complete)
int16_t playbackApplyVolumeEnvelope(PlaybackModState* mod, int maxVolume, int* stopNote);

// Apply volume modulation (handles both LFO and ADSR/AHD)
// For LFO: adds to volumeOffset
// For ADSR/AHD: rewrites the volume directly
// Returns 1 if note should be stopped (ADSR release complete), 0 otherwise
int playbackApplyVolumeModulation(PlaybackModState* mod, int8_t* volumeOffset, uint8_t* volume, int maxVolume);


#ifdef __cplusplus
}
#endif

#endif
