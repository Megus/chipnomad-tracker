#ifndef __CHIPS_H__
#define __CHIPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../project.h"

typedef struct SoundChip {
  void* userdata;
  uint8_t regs[256];  // Space for 256 chip registers

  int (*timerFunc)(struct SoundChip* self, void* userdata);
  void* timerUserdata;

  int (*init)(struct SoundChip* self);
  void (*setRegister)(struct SoundChip* self, uint16_t reg, uint8_t value);
  void (*setTimerFunc)(struct SoundChip* self, int (*timerFunc)(struct SoundChip* self, void* userdata), void* timerUserdata);
  void (*render)(struct SoundChip* self, float* buffer, int samples);
  void (*setQuality)(struct SoundChip* self, int quality);
  int (*cleanup)(struct SoundChip* self);
} SoundChip;

SoundChip createChipAY(int sampleRate, ChipSetup setup);
void updateChipAYType(SoundChip* chip, uint8_t isYM);
void updateChipAYStereoMode(SoundChip* chip, enum StereoModeAY stereoMode, uint8_t separation);
void updateChipAYClock(SoundChip* chip, int clockRate, int sampleRate);

#ifdef __cplusplus
}
#endif

#endif