#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>
#include <corelib_mainloop.h>

struct ColorScheme {
  int background;
  int textEmpty;
  int textInfo;
  int textDefault;
  int textValue;
  int textTitles;
  int playMarkers;
  int cursor;
  int selection;
};

struct AppSettings {
  int audioSampleRate;
  int audioBufferSize;
  struct ColorScheme colorScheme;
};

struct SoundChip {
  void* userdata;

  int (*init)(struct SoundChip* self);
  void (*render)(struct SoundChip* self, int16_t* buffer, int samples);
  int (*cleanup)(struct SoundChip* self);
};

struct AppScreen {
  void (*setup)(int input);
  void (*fullRedraw)(void);
  void (*draw)(void);
  void (*onEvent)(enum MainLoopEvent event, int value, void* userdata);
};

extern struct AppSettings appSettings;

#endif
