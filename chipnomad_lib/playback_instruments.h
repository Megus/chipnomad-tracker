#ifndef __PLAYBACK_INSTRUMENTS_H__
#define __PLAYBACK_INSTRUMENTS_H__

#include "playback.h"

typedef struct PlaybackInstrument {
  void (*init)(PlaybackState* state, int trackIdx);
  void (*handle)(PlaybackState* state, int trackIdx);

} PlaybackInstrument;

#endif