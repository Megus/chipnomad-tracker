#include "playback_instruments.h"
#include "playback_modulation.h"

// Apply ADSR/AHD volume modulation and return the resulting volume
// Takes a single modulation state and returns the calculated volume (0-maxVolume)
// Returns -1 if the modulation is not ADSR/AHD (e.g., LFO)
// stopNote: set to 1 if the note should be stopped (ADSR release complete)
int16_t playbackApplyVolumeEnvelope(PlaybackModState* mod, int maxVolume, int* stopNote) {
  *stopNote = 0;

  // Only handle ADSR/AHD - skip LFO
  if (mod->modulation->type == modLFO) {
    return -1;
  }

  // Volume modulation scale uses absolute volume values, hence the range is 127
  int16_t scaledVolume = playbackModScaleToRange(mod->outValue, 127);

  uint8_t volume;
  if (scaledVolume < 0) {
    volume = maxVolume + scaledVolume;
  } else {
    volume = scaledVolume;
  }
  // Clamp volume to valid range for safety
  if (volume > maxVolume) volume = maxVolume;
  if (volume < 0) volume = 0;

  // Check if ADSR/AHD just completed release phase
  if (mod->step == 0xff) {
    *stopNote = 1;
  }

  return volume;
}
