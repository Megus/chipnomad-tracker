#ifndef __PLAYBACK_FX_H__
#define __PLAYBACK_FX_H__

typedef struct PlaybackFXData_PBN {
  int value;
  uint8_t lowByte;
} PlaybackFXData_PBN;

typedef struct PlaybackFXData_CountFX {
  uint8_t counter;
} PlaybackFXData_CountFX;

typedef struct PlaybackFXData_PSL {
  int16_t startPeriod;
  int16_t endPeriod;
  uint8_t counter;
} PlaybackFXData_PSL;

typedef union PlaybackFXData {
  PlaybackFXData_PBN pbn;
  PlaybackFXData_CountFX count_fx;
  PlaybackFXData_PSL psl;
} PlaybackFXData;

typedef struct PlaybackFXState {
  uint8_t fx;
  uint8_t value;
  PlaybackFXData data;
} PlaybackFXState;

#endif