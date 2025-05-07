#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <corelib_mainloop.h>
#include <playback.h>

#define AUTOSAVE_FILENAME "autosave.cnm"

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
  int doubleTapFrames;
  int keyRepeatDelay;
  int keyRepeatSpeed;
  float volume;
  struct ColorScheme colorScheme;
};

extern struct AppSettings appSettings;
extern int* pSongRow;
extern int* pSongTrack;
extern int* pChainRow;

extern struct PlaybackState playback;

#endif
