#ifndef __CHIPS_H__
#define __CHIPS_H__

#include <stdint.h>
#include <project.h>

struct SoundChip {
  void* userdata;
  uint8_t regs[256];  // Space for 256 chip registers

  int (*init)(struct SoundChip* self);
  void (*setRegister)(struct SoundChip* self, uint16_t reg, uint8_t value);
  void (*render)(struct SoundChip* self, int16_t* buffer, int samples, float volume);
  int (*cleanup)(struct SoundChip* self);
};

struct SoundChip createChipAY(int sampleRate, union ChipSetup setup);

#endif