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

// Key mapping: 8 buttons × 3 keys each
// Stored as SDL keycodes (keyboard) or button IDs (controller)
// 0 = unmapped
typedef struct KeyMapping {
  int32_t keyUp[3];
  int32_t keyDown[3];
  int32_t keyLeft[3];
  int32_t keyRight[3];
  int32_t keyEdit[3];   // A button
  int32_t keyOpt[3];    // B button
  int32_t keyPlay[3];   // Start
  int32_t keyShift[3];  // Select
} KeyMapping;

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
  KeyMapping keyMapping;
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
void initDefaultKeyMapping(void);
void resetKeyMappingToDefaults(void);

// Utility functions
void extractFilenameWithoutExtension(const char* path, char* output, int maxLength);
const char* getAutosavePath(void);
void clearNotePreview(void);

#endif
