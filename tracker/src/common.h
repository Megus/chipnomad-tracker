#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "chipnomad_lib.h"
#include "corelib_mainloop.h"

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

typedef enum {
  KEYBOARD_LAYOUT_AUTO = 0,
  KEYBOARD_LAYOUT_QWERTY = 1,
  KEYBOARD_LAYOUT_QWERTZ = 2,
  KEYBOARD_LAYOUT_AZERTY = 3,
  KEYBOARD_LAYOUT_DVORAK = 4
} KeyboardLayout;

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
  KeyboardLayout keyboardLayout;
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
