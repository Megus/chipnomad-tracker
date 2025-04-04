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

extern struct AppSettings appSettings;

#endif
