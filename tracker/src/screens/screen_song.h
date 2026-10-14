#ifndef __SCREEN_SONG_H__
#define __SCREEN_SONG_H__

#include "screen.h"

class ScreenSong : public Screen {
  public:
    ScreenSong(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx): Screen(state, dispatcher, gfx) {};
};

#endif // __SCREEN_SONG_H__