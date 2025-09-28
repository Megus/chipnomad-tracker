#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <corelib_mainloop.h>
#include <playback.h>

#define AUTOSAVE_FILENAME "autosave.cnm"
#define FILENAME_LENGTH (24)
#define PATH_LENGTH (4096)

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
  char projectFilename[FILENAME_LENGTH + 1];
  char projectPath[PATH_LENGTH + 1];
  char pitchTablePath[PATH_LENGTH + 1];
  char instrumentPath[PATH_LENGTH + 1];
};

extern struct AppSettings appSettings;
extern int* pSongRow;
extern int* pSongTrack;
extern int* pChainRow;

extern struct PlaybackState playback;

// Settings functions
int settingsSave(void);
int settingsLoad(void);

// Utility functions
void extractFilenameWithoutExtension(const char* path, char* output, int maxLength);

#endif
