#include "common.h"
#include "corelib/corelib_file.h"
#include "corelib/corelib_gfx.h"
#include "keyboard_layout.h"
#include <string.h>

#ifdef MACOS_BUILD
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(DESKTOP_BUILD) || defined(PORTMASTER_BUILD)
#define DEFAULT_VOLUME (1.0)
#else
#define DEFAULT_VOLUME (0.5)
#endif

#define SETTINGS_FILENAME "settings.txt"

AppSettings appSettings = {
  .screenWidth = 0, // 0 to auto-detect resolution
  .screenHeight = 0, // 0 to auto-detect resolution
  .audioSampleRate = 44100,
  .audioBufferSize = 2048,
  .doubleTapFrames = 10,
  .keyRepeatDelay = 16,
  .keyRepeatSpeed = 2,
  .volume = DEFAULT_VOLUME,
  .mixVolume = 20000.0f / 32767.0f,
  .quality = CHIPNOMAD_QUALITY_MEDIUM,
  .pitchConflictWarning = 0,
  .gamepadSwapAB = 0,
  .keyboardLayout = KEYBOARD_LAYOUT_AUTO,
  .colorScheme = {
    .background = 0x000f1a,
    .textEmpty = 0x002638,
    .textInfo = 0x4878b0,
    .textDefault = 0xa0d0f0,
    .textValue = 0xe2ebf8,
    .textTitles = 0xbfdf50,
    .playMarkers = 0xefe000,
    .cursor = 0x7ddcff,
    .selection = 0x00d090,
    .warning = 0xff4040,
  },
  .themeName = "Default",
  .projectFilename = "",
  .projectPath = "",
  .pitchTablePath = "",
  .instrumentPath = ""
};

int* pSongRow;
int* pSongTrack;
int* pChainRow;
ChipNomadState* chipnomadState;

#ifdef MACOS_BUILD
static char userDataPath[PATH_LENGTH] = "";

static const char* getUserDataPath(void) {
  if (userDataPath[0] == '\0') {
    const char* home = getenv("HOME");
    if (home) {
      snprintf(userDataPath, PATH_LENGTH, "%s/Library/Application Support/ChipNomad", home);
      // Create directory if it doesn't exist
      mkdir(userDataPath, 0755);
    } else {
      strcpy(userDataPath, ".");
    }
  }
  return userDataPath;
}

static void getFullPath(const char* filename, char* fullPath, int maxLength) {
  snprintf(fullPath, maxLength, "%s/%s", getUserDataPath(), filename);
}
#endif

int settingsSave(void) {
  #ifdef MACOS_BUILD
  char fullPath[PATH_LENGTH];
  getFullPath(SETTINGS_FILENAME, fullPath, PATH_LENGTH);
  int fileId = fileOpen(fullPath, 1);
  #else
  int fileId = fileOpen(SETTINGS_FILENAME, 1);
  #endif
  if (fileId == -1) return 1;

  filePrintf(fileId, "screenWidth: %d\n", appSettings.screenWidth);
  filePrintf(fileId, "screenHeight: %d\n", appSettings.screenHeight);
  filePrintf(fileId, "audioSampleRate: %d\n", appSettings.audioSampleRate);
  filePrintf(fileId, "audioBufferSize: %d\n", appSettings.audioBufferSize);
  filePrintf(fileId, "doubleTapFrames: %d\n", appSettings.doubleTapFrames);
  filePrintf(fileId, "keyRepeatDelay: %d\n", appSettings.keyRepeatDelay);
  filePrintf(fileId, "keyRepeatSpeed: %d\n", appSettings.keyRepeatSpeed);
  filePrintf(fileId, "volume: %f\n", appSettings.volume);
  filePrintf(fileId, "mixVolume: %f\n", appSettings.mixVolume);
  filePrintf(fileId, "quality: %d\n", appSettings.quality);
  filePrintf(fileId, "pitchConflictWarning: %d\n", appSettings.pitchConflictWarning);
  filePrintf(fileId, "gamepadSwapAB: %d\n", appSettings.gamepadSwapAB);
  filePrintf(fileId, "keyboardLayout: %d\n", (int)appSettings.keyboardLayout);
  filePrintf(fileId, "colorBackground: 0x%06x\n", appSettings.colorScheme.background);
  filePrintf(fileId, "colorTextEmpty: 0x%06x\n", appSettings.colorScheme.textEmpty);
  filePrintf(fileId, "colorTextInfo: 0x%06x\n", appSettings.colorScheme.textInfo);
  filePrintf(fileId, "colorTextDefault: 0x%06x\n", appSettings.colorScheme.textDefault);
  filePrintf(fileId, "colorTextValue: 0x%06x\n", appSettings.colorScheme.textValue);
  filePrintf(fileId, "colorTextTitles: 0x%06x\n", appSettings.colorScheme.textTitles);
  filePrintf(fileId, "colorPlayMarkers: 0x%06x\n", appSettings.colorScheme.playMarkers);
  filePrintf(fileId, "colorCursor: 0x%06x\n", appSettings.colorScheme.cursor);
  filePrintf(fileId, "colorSelection: 0x%06x\n", appSettings.colorScheme.selection);
  filePrintf(fileId, "colorWarning: 0x%06x\n", appSettings.colorScheme.warning);
  filePrintf(fileId, "themeName: %s\n", appSettings.themeName);
  filePrintf(fileId, "projectFilename: %s\n", appSettings.projectFilename);
  filePrintf(fileId, "projectPath: %s\n", appSettings.projectPath);
  filePrintf(fileId, "pitchTablePath: %s\n", appSettings.pitchTablePath);
  filePrintf(fileId, "instrumentPath: %s\n", appSettings.instrumentPath);
  filePrintf(fileId, "themePath: %s\n", appSettings.themePath);

  fileClose(fileId);
  return 0;
}

int settingsLoad(void) {
  #ifdef MACOS_BUILD
  char fullPath[PATH_LENGTH];
  getFullPath(SETTINGS_FILENAME, fullPath, PATH_LENGTH);
  int fileId = fileOpen(fullPath, 0);
  #else
  int fileId = fileOpen(SETTINGS_FILENAME, 0);
  #endif
  if (fileId == -1) return 1;

  char* line;
  while ((line = fileReadString(fileId)) != NULL) {
    if (strncmp(line, "screenWidth: ", 13) == 0) {
      sscanf(line + 13, "%d", &appSettings.screenWidth);
    } else if (strncmp(line, "screenHeight: ", 14) == 0) {
      sscanf(line + 14, "%d", &appSettings.screenHeight);
    } else if (strncmp(line, "audioSampleRate: ", 17) == 0) {
      sscanf(line + 17, "%d", &appSettings.audioSampleRate);
    } else if (strncmp(line, "audioBufferSize: ", 17) == 0) {
      sscanf(line + 17, "%d", &appSettings.audioBufferSize);
    } else if (strncmp(line, "doubleTapFrames: ", 17) == 0) {
      sscanf(line + 17, "%d", &appSettings.doubleTapFrames);
    } else if (strncmp(line, "keyRepeatDelay: ", 16) == 0) {
      sscanf(line + 16, "%d", &appSettings.keyRepeatDelay);
    } else if (strncmp(line, "keyRepeatSpeed: ", 16) == 0) {
      sscanf(line + 16, "%d", &appSettings.keyRepeatSpeed);
    } else if (strncmp(line, "volume: ", 8) == 0) {
      sscanf(line + 8, "%f", &appSettings.volume);
    } else if (strncmp(line, "mixVolume: ", 11) == 0) {
      sscanf(line + 11, "%f", &appSettings.mixVolume);
    } else if (strncmp(line, "quality: ", 9) == 0) {
      sscanf(line + 9, "%d", &appSettings.quality);
    } else if (strncmp(line, "pitchConflictWarning: ", 22) == 0) {
      sscanf(line + 22, "%d", &appSettings.pitchConflictWarning);
    } else if (strncmp(line, "gamepadSwapAB: ", 15) == 0) {
      sscanf(line + 15, "%d", &appSettings.gamepadSwapAB);
    } else if (strncmp(line, "keyboardLayout: ", 16) == 0) {
      int layoutValue;
      sscanf(line + 16, "%d", &layoutValue);
      appSettings.keyboardLayout = (KeyboardLayout)layoutValue;
    } else if (strncmp(line, "colorBackground: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &appSettings.colorScheme.background);
    } else if (strncmp(line, "colorTextEmpty: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &appSettings.colorScheme.textEmpty);
    } else if (strncmp(line, "colorTextInfo: ", 15) == 0) {
      sscanf(line + 15, "0x%x", &appSettings.colorScheme.textInfo);
    } else if (strncmp(line, "colorTextDefault: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &appSettings.colorScheme.textDefault);
    } else if (strncmp(line, "colorTextValue: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &appSettings.colorScheme.textValue);
    } else if (strncmp(line, "colorTextTitles: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &appSettings.colorScheme.textTitles);
    } else if (strncmp(line, "colorPlayMarkers: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &appSettings.colorScheme.playMarkers);
    } else if (strncmp(line, "colorCursor: ", 13) == 0) {
      sscanf(line + 13, "0x%x", &appSettings.colorScheme.cursor);
    } else if (strncmp(line, "colorSelection: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &appSettings.colorScheme.selection);
    } else if (strncmp(line, "colorWarning: ", 14) == 0) {
      sscanf(line + 14, "0x%x", &appSettings.colorScheme.warning);
    } else if (strncmp(line, "themeName: ", 11) == 0) {
      strncpy(appSettings.themeName, line + 11, 16);
      appSettings.themeName[16] = 0;
    } else if (strncmp(line, "projectFilename: ", 17) == 0) {
      strncpy(appSettings.projectFilename, line + 17, FILENAME_LENGTH);
      appSettings.projectFilename[FILENAME_LENGTH] = 0;
    } else if (strncmp(line, "projectPath: ", 13) == 0) {
      strncpy(appSettings.projectPath, line + 13, PATH_LENGTH);
      appSettings.projectPath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "pitchTablePath: ", 16) == 0) {
      strncpy(appSettings.pitchTablePath, line + 16, PATH_LENGTH);
      appSettings.pitchTablePath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "instrumentPath: ", 16) == 0) {
      strncpy(appSettings.instrumentPath, line + 16, PATH_LENGTH);
      appSettings.instrumentPath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "themePath: ", 11) == 0) {
      strncpy(appSettings.themePath, line + 11, PATH_LENGTH);
      appSettings.themePath[PATH_LENGTH] = 0;
    }
  }

  fileClose(fileId);
  return 0;
}

void resetToDefaultColors(void) {
  appSettings.colorScheme.background = 0x000f1a;
  appSettings.colorScheme.textEmpty = 0x002638;
  appSettings.colorScheme.textInfo = 0x4878b0;
  appSettings.colorScheme.textDefault = 0xa0d0f0;
  appSettings.colorScheme.textValue = 0xe2ebf8;
  appSettings.colorScheme.textTitles = 0xbfdf50;
  appSettings.colorScheme.playMarkers = 0xefe000;
  appSettings.colorScheme.cursor = 0x7ddcff;
  appSettings.colorScheme.selection = 0x00d090;
  appSettings.colorScheme.warning = 0xff4040;
}

int saveTheme(const char* path) {
  int fileId = fileOpen(path, 1);
  if (fileId == -1) return 1;
  
  filePrintf(fileId, "colorBackground: 0x%06x\n", appSettings.colorScheme.background);
  filePrintf(fileId, "colorTextEmpty: 0x%06x\n", appSettings.colorScheme.textEmpty);
  filePrintf(fileId, "colorTextInfo: 0x%06x\n", appSettings.colorScheme.textInfo);
  filePrintf(fileId, "colorTextDefault: 0x%06x\n", appSettings.colorScheme.textDefault);
  filePrintf(fileId, "colorTextValue: 0x%06x\n", appSettings.colorScheme.textValue);
  filePrintf(fileId, "colorTextTitles: 0x%06x\n", appSettings.colorScheme.textTitles);
  filePrintf(fileId, "colorPlayMarkers: 0x%06x\n", appSettings.colorScheme.playMarkers);
  filePrintf(fileId, "colorCursor: 0x%06x\n", appSettings.colorScheme.cursor);
  filePrintf(fileId, "colorSelection: 0x%06x\n", appSettings.colorScheme.selection);
  filePrintf(fileId, "colorWarning: 0x%06x\n", appSettings.colorScheme.warning);
  
  fileClose(fileId);
  return 0;
}

int loadTheme(const char* path) {
  int fileId = fileOpen(path, 0);
  if (fileId == -1) return 1;
  
  // First reset to defaults to ensure all colors have valid values
  resetToDefaultColors();
  
  char* line;
  while ((line = fileReadString(fileId)) != NULL) {
    if (strncmp(line, "colorBackground: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &appSettings.colorScheme.background);
    } else if (strncmp(line, "colorTextEmpty: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &appSettings.colorScheme.textEmpty);
    } else if (strncmp(line, "colorTextInfo: ", 15) == 0) {
      sscanf(line + 15, "0x%x", &appSettings.colorScheme.textInfo);
    } else if (strncmp(line, "colorTextDefault: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &appSettings.colorScheme.textDefault);
    } else if (strncmp(line, "colorTextValue: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &appSettings.colorScheme.textValue);
    } else if (strncmp(line, "colorTextTitles: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &appSettings.colorScheme.textTitles);
    } else if (strncmp(line, "colorPlayMarkers: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &appSettings.colorScheme.playMarkers);
    } else if (strncmp(line, "colorCursor: ", 13) == 0) {
      sscanf(line + 13, "0x%x", &appSettings.colorScheme.cursor);
    } else if (strncmp(line, "colorSelection: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &appSettings.colorScheme.selection);
    } else if (strncmp(line, "colorWarning: ", 14) == 0) {
      sscanf(line + 14, "0x%x", &appSettings.colorScheme.warning);
    }
  }
  
  fileClose(fileId);
  return 0;
}

const char* getAutosavePath(void) {
  #ifdef MACOS_BUILD
  static char autosavePath[PATH_LENGTH];
  getFullPath(AUTOSAVE_FILENAME, autosavePath, PATH_LENGTH);
  return autosavePath;
  #else
  return AUTOSAVE_FILENAME;
  #endif
}

void extractFilenameWithoutExtension(const char* path, char* output, int maxLength) {
  // Extract filename from path
  const char* filename = strrchr(path, PATH_SEPARATOR);
  if (filename) {
    filename++; // Skip the separator
  } else {
    filename = path; // No path separator found
  }

  // Copy filename
  strncpy(output, filename, maxLength - 1);
  output[maxLength - 1] = 0;

  // Remove file extension
  char* dot = strrchr(output, '.');
  if (dot) {
    *dot = 0;
  }
}

void clearNotePreview(void) {
  // Clear the note preview area for all possible tracks (right side of screen)
  gfxClearRect(35, 3, 5, PROJECT_MAX_TRACKS);
}
