#include <common.h>

#ifdef DESKTOP_BUILD
#define DEFAULT_VOLUME (1.0)
#else
#define DEFAULT_VOLUME (0.5)
#endif

struct AppSettings appSettings = {
  .audioSampleRate = 44100,
  .audioBufferSize = 2048,
  .doubleTapFrames = 10,
  .keyRepeatDelay = 16,
  .keyRepeatSpeed = 2,
  .volume = DEFAULT_VOLUME,
  .colorScheme = {
    .background = 0x000f1a,
    .textEmpty = 0x002638,
    .textInfo = 0x4878b0,
    .textDefault = 0xa0d0f0,
    .textValue = 0xe2ebf8,
    .textTitles = 0xbfdf50,
    .playMarkers = 0xefe000,
    .cursor = 0x7ddcff,
    .selection = 0x00d090,
  }
};

int* pSongRow;
int* pSongTrack;
int* pChainRow;
struct PlaybackState playback;
