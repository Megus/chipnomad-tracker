#ifndef __PLAYBACK_FX_H__
#define __PLAYBACK_FX_H__

struct PlaybackFXData_PBN {
  int value;
  uint8_t lowByte;
};

union PlaybackFXData {
  struct PlaybackFXData_PBN pbn;
};

struct PlaybackFXState {
  uint8_t fx;
  uint8_t value;
  union PlaybackFXData data;
};

#endif