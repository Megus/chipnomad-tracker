#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "corelib_mainloop.h"
#include "chipnomad_lib.h"

#define AUTOSAVE_FILENAME "autosave.cnm"
#define FILENAME_LENGTH (24)
#define PATH_LENGTH (4096)
#define THEME_NAME_LENGTH (16)

typedef struct ColorScheme {
  int background;
  int textEmpty;
  int textInfo;
  int textDefault;
  int textValue;
  int textTitles;
  int playMarkers;
  int cursor;
  int selection;
  int warning;
} ColorScheme;

typedef struct AppSettings {
  int screenWidth;
  int screenHeight;
  int audioSampleRate;
  int audioBufferSize;
  int doubleTapFrames;
  int keyRepeatDelay;
  int keyRepeatSpeed;
  float volume;
  float mixVolume;
  int quality;
  int pitchConflictWarning;
  int gamepadSwapAB;
  ColorScheme colorScheme;
  char themeName[THEME_NAME_LENGTH + 1];
  char projectFilename[FILENAME_LENGTH + 1];
  char projectPath[PATH_LENGTH + 1];
  char pitchTablePath[PATH_LENGTH + 1];
  char instrumentPath[PATH_LENGTH + 1];
  char themePath[PATH_LENGTH + 1];
} AppSettings;

extern AppSettings appSettings;
extern int* pSongRow;
extern int* pSongTrack;
extern int* pChainRow;

extern ChipNomadState* chipnomadState;

// Settings functions
int settingsSave(void);
int settingsLoad(void);
int saveTheme(const char* path);
int loadTheme(const char* path);
void resetToDefaultColors(void);

// Utility functions
void extractFilenameWithoutExtension(const char* path, char* output, int maxLength);
const char* getAutosavePath(void);
void clearNotePreview(void);

#endif
