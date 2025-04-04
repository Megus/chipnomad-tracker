#ifndef __CHIPS_H__
#define __CHIPS_H__

#include <stdint.h>

struct SoundChip {
  void* userdata;

  int (*init)(struct SoundChip* self);
  void (*render)(struct SoundChip* self, int16_t* buffer, int samples);
  int (*cleanup)(struct SoundChip* self);
};

struct SoundChip createAyumi(int sampleRate);

#endif