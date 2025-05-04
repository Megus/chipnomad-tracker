#ifndef __PLAYBACK_FX_H__
#define __PLAYBACK_FX_H__

struct PlaybackFXData_PBN {
  int value;
  uint8_t lowByte;
};

struct PlaybackFXData_CountFX {
  uint8_t counter;
};

union PlaybackFXData {
  struct PlaybackFXData_PBN pbn;
  struct PlaybackFXData_CountFX count_fx;
};

struct PlaybackFXState {
  uint8_t fx;
  uint8_t value;
  union PlaybackFXData data;
};

#endif