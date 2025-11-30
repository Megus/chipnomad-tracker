#ifndef __CHIPS_H__
#define __CHIPS_H__

#include <stdint.h>
#include "../project.h"

typedef struct SoundChip {
  void* userdata;
  uint8_t regs[256];  // Space for 256 chip registers

  int (*init)(struct SoundChip* self);
  void (*setRegister)(struct SoundChip* self, uint16_t reg, uint8_t value);
  void (*render)(struct SoundChip* self, float* buffer, int samples);
  void (*setQuality)(struct SoundChip* self, int quality);
  int (*cleanup)(struct SoundChip* self);
} SoundChip;

SoundChip createChipAY(int sampleRate, ChipSetup setup);
void updateChipAYType(SoundChip* chip, uint8_t isYM);
void updateChipAYStereoMode(SoundChip* chip, enum StereoModeAY stereoMode, uint8_t separation);
void updateChipAYClock(SoundChip* chip, int clockRate, int sampleRate);

#endif