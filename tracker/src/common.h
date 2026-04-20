#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "chipnomad_lib.h"
#include "corelib/corelib_input.h"
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

// Key mapping: 8 buttons × 3 keys each
typedef struct KeyMapping {
  InputCode keyUp[3];
  InputCode keyDown[3];
  InputCode keyLeft[3];
  InputCode keyRight[3];
  InputCode keyEdit[3];
  InputCode keyOpt[3];
  InputCode keyPlay[3];
  InputCode keyShift[3];
} KeyMapping;

typedef struct AppSettings {
  int screenWidth;
  int screenHeight;
  int audioSampleRate;
  int audioBufferSize;
  int doubleTapFrames;
  int keyRepeatDelay;
  int keyRepeatSpeed;
  float mixVolume;
  int quality;
  int pitchConflictWarning;
  KeyMapping keyMapping;
  ColorScheme colorScheme;
  char themeName[THEME_NAME_LENGTH + 1];
  char projectFilename[FILENAME_LENGTH + 1];
  char projectPath[PATH_LENGTH + 1];
  char pitchTablePath[PATH_LENGTH + 1];
  char instrumentPath[PATH_LENGTH + 1];
  char themePath[PATH_LENGTH + 1];
  char fontPath[PATH_LENGTH + 1];
  char fontFolderPath[PATH_LENGTH + 1];
} AppSettings;

extern AppSettings appSettings;
extern int* pSongRow;
extern int* pSongTrack;
extern int* pChainRow;

extern ChipNomadState* chipnomadState;

extern int projectModified; // Flag to track if the project has unsaved changes

// Settings functions
int settingsSave(void);
int settingsLoad(void);
int saveTheme(const char* path);
int loadTheme(const char* path);
void resetToDefaultColors(void);
void initDefaultKeyMapping(void);
void resetKeyMappingToDefaults(void);

// Utility functions
void extractFilenameWithoutExtension(const char* path, char* output, int maxLength);
const char* getAutosavePath(void);
void clearNotePreview(void);

#endif
