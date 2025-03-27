#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>

struct AppSettings {
  int audioSampleRate;
  int audioBufferSize;
};

struct SoundChip {
  void* userdata;

  int (*init)(struct SoundChip* self);
  void (*render)(struct SoundChip* self, int16_t* buffer, int samples);
  int (*cleanup)(struct SoundChip* self);
};

extern const struct AppSettings appSettings;

#endif
