#ifndef __PLAYBACK_FX_H__
#define __PLAYBACK_FX_H__

union PlaybackFXData {

};

struct PlaybackFXState {
  uint8_t fx;
  uint8_t value;
  union PlaybackFXData data;
};

#endif