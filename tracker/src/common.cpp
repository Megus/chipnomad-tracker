#include "common.h"
#include "corelib/corelib_file.h"
#include "corelib/corelib_gfx.h"
#include <stdio.h>
#include <string.h>
#include <chipnomad_lib.h>

static char settingsPath[PATH_LENGTH + 32];
static char autosavePath[PATH_LENGTH + 32];
static char lineBuffer[1024];

AppSettings appSettings;

void initDefaultAppSettings(void) {
  appSettings.screenWidth = 0; // 0 to auto-detect resolution
  appSettings.screenHeight = 0;
  appSettings.audioSampleRate = 44100;
  appSettings.audioBufferSize = 2048;
  appSettings.aySampleDithering = 1; // Default: ON
  appSettings.doubleTapFrames = 20;
  appSettings.keyRepeatDelay = 16;
  appSettings.keyRepeatSpeed = 2;
  appSettings.mixVolume = 20000.0f / 32767.0f;
  appSettings.quality = (int)ChipNomadQuality::medium;
  appSettings.pitchConflictWarning = 0;

  // Zero out key mapping (platform-specific defaults applied later)
  memset(&appSettings.keyMapping, 0, sizeof(KeyMapping));

  // Color scheme defaults
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

  // String defaults
  strncpy(appSettings.themeName, "Default", THEME_NAME_LENGTH);
  appSettings.themeName[THEME_NAME_LENGTH] = '\0';
  appSettings.projectFilename[0] = '\0';
  appSettings.projectPath[0] = '\0';
  appSettings.pitchTablePath[0] = '\0';
  appSettings.instrumentPath[0] = '\0';
  appSettings.themePath[0] = '\0';
  appSettings.fontPath[0] = '\0';
  appSettings.fontFolderPath[0] = '\0';
  appSettings.samplePath[0] = '\0';
  appSettings.wavetablePath[0] = '\0';
}

int* pSongRow;
int* pSongTrack;
int* pChainRow;
ChipNomadState* chipnomadState;
int projectModified = 0;

int settingsSave(void) {
  char defaultDir[PATH_LENGTH];
  if (fileGetDefaultDirectory(defaultDir, PATH_LENGTH) != 0) return 1;
  snprintf(settingsPath, sizeof(settingsPath), "%s%ssettings.txt", defaultDir, PATH_SEPARATOR_STR);

  FILE* file = fopen(settingsPath, "w");
  if (file == NULL) return 1;

  fprintf(file, "screenWidth: %d\n", appSettings.screenWidth);
  fprintf(file, "screenHeight: %d\n", appSettings.screenHeight);
  fprintf(file, "audioSampleRate: %d\n", appSettings.audioSampleRate);
  fprintf(file, "audioBufferSize: %d\n", appSettings.audioBufferSize);
  fprintf(file, "aySampleDithering: %d\n", appSettings.aySampleDithering);
  fprintf(file, "doubleTapFrames: %d\n", appSettings.doubleTapFrames);
  fprintf(file, "keyRepeatDelay: %d\n", appSettings.keyRepeatDelay);
  fprintf(file, "keyRepeatSpeed: %d\n", appSettings.keyRepeatSpeed);
  fprintf(file, "mixVolume: %f\n", appSettings.mixVolume);
  fprintf(file, "quality: %d\n", appSettings.quality);
  fprintf(file, "pitchConflictWarning: %d\n", appSettings.pitchConflictWarning);

  // Save key mapping codes
  fprintf(file, "keyUp: %d,%d,%d\n", appSettings.keyMapping.keyUp[0].code, appSettings.keyMapping.keyUp[1].code, appSettings.keyMapping.keyUp[2].code);
  fprintf(file, "keyDown: %d,%d,%d\n", appSettings.keyMapping.keyDown[0].code, appSettings.keyMapping.keyDown[1].code, appSettings.keyMapping.keyDown[2].code);
  fprintf(file, "keyLeft: %d,%d,%d\n", appSettings.keyMapping.keyLeft[0].code, appSettings.keyMapping.keyLeft[1].code, appSettings.keyMapping.keyLeft[2].code);
  fprintf(file, "keyRight: %d,%d,%d\n", appSettings.keyMapping.keyRight[0].code, appSettings.keyMapping.keyRight[1].code, appSettings.keyMapping.keyRight[2].code);
  fprintf(file, "keyEdit: %d,%d,%d\n", appSettings.keyMapping.keyEdit[0].code, appSettings.keyMapping.keyEdit[1].code, appSettings.keyMapping.keyEdit[2].code);
  fprintf(file, "keyOpt: %d,%d,%d\n", appSettings.keyMapping.keyOpt[0].code, appSettings.keyMapping.keyOpt[1].code, appSettings.keyMapping.keyOpt[2].code);
  fprintf(file, "keyPlay: %d,%d,%d\n", appSettings.keyMapping.keyPlay[0].code, appSettings.keyMapping.keyPlay[1].code, appSettings.keyMapping.keyPlay[2].code);
  fprintf(file, "keyShift: %d,%d,%d\n", appSettings.keyMapping.keyShift[0].code, appSettings.keyMapping.keyShift[1].code, appSettings.keyMapping.keyShift[2].code);

  // Save key mapping device types
  fprintf(file, "keyUpType: %d,%d,%d\n", appSettings.keyMapping.keyUp[0].deviceType, appSettings.keyMapping.keyUp[1].deviceType, appSettings.keyMapping.keyUp[2].deviceType);
  fprintf(file, "keyDownType: %d,%d,%d\n", appSettings.keyMapping.keyDown[0].deviceType, appSettings.keyMapping.keyDown[1].deviceType, appSettings.keyMapping.keyDown[2].deviceType);
  fprintf(file, "keyLeftType: %d,%d,%d\n", appSettings.keyMapping.keyLeft[0].deviceType, appSettings.keyMapping.keyLeft[1].deviceType, appSettings.keyMapping.keyLeft[2].deviceType);
  fprintf(file, "keyRightType: %d,%d,%d\n", appSettings.keyMapping.keyRight[0].deviceType, appSettings.keyMapping.keyRight[1].deviceType, appSettings.keyMapping.keyRight[2].deviceType);
  fprintf(file, "keyEditType: %d,%d,%d\n", appSettings.keyMapping.keyEdit[0].deviceType, appSettings.keyMapping.keyEdit[1].deviceType, appSettings.keyMapping.keyEdit[2].deviceType);
  fprintf(file, "keyOptType: %d,%d,%d\n", appSettings.keyMapping.keyOpt[0].deviceType, appSettings.keyMapping.keyOpt[1].deviceType, appSettings.keyMapping.keyOpt[2].deviceType);
  fprintf(file, "keyPlayType: %d,%d,%d\n", appSettings.keyMapping.keyPlay[0].deviceType, appSettings.keyMapping.keyPlay[1].deviceType, appSettings.keyMapping.keyPlay[2].deviceType);
  fprintf(file, "keyShiftType: %d,%d,%d\n", appSettings.keyMapping.keyShift[0].deviceType, appSettings.keyMapping.keyShift[1].deviceType, appSettings.keyMapping.keyShift[2].deviceType);

  fprintf(file, "colorBackground: 0x%06x\n", appSettings.colorScheme.background);
  fprintf(file, "colorTextEmpty: 0x%06x\n", appSettings.colorScheme.textEmpty);
  fprintf(file, "colorTextInfo: 0x%06x\n", appSettings.colorScheme.textInfo);
  fprintf(file, "colorTextDefault: 0x%06x\n", appSettings.colorScheme.textDefault);
  fprintf(file, "colorTextValue: 0x%06x\n", appSettings.colorScheme.textValue);
  fprintf(file, "colorTextTitles: 0x%06x\n", appSettings.colorScheme.textTitles);
  fprintf(file, "colorPlayMarkers: 0x%06x\n", appSettings.colorScheme.playMarkers);
  fprintf(file, "colorCursor: 0x%06x\n", appSettings.colorScheme.cursor);
  fprintf(file, "colorSelection: 0x%06x\n", appSettings.colorScheme.selection);
  fprintf(file, "colorWarning: 0x%06x\n", appSettings.colorScheme.warning);
  fprintf(file, "themeName: %s\n", appSettings.themeName);
  fprintf(file, "projectFilename: %s\n", appSettings.projectFilename);
  fprintf(file, "projectPath: %s\n", appSettings.projectPath);
  fprintf(file, "pitchTablePath: %s\n", appSettings.pitchTablePath);
  fprintf(file, "instrumentPath: %s\n", appSettings.instrumentPath);
  fprintf(file, "themePath: %s\n", appSettings.themePath);
  fprintf(file, "fontPath: %s\n", appSettings.fontPath);
  fprintf(file, "fontFolderPath: %s\n", appSettings.fontFolderPath);
  fprintf(file, "samplePath: %s\n", appSettings.samplePath);
  fprintf(file, "wavetablePath: %s\n", appSettings.wavetablePath);

  fclose(file);
  return 0;
}

int settingsLoad(void) {
  // Always initialize defaults first
  initDefaultAppSettings();

  char defaultDir[PATH_LENGTH];
  if (fileGetDefaultDirectory(defaultDir, PATH_LENGTH) != 0) return 1;
  snprintf(settingsPath, sizeof(settingsPath), "%s%ssettings.txt", defaultDir, PATH_SEPARATOR_STR);

  FILE* file = fopen(settingsPath, "r");
  if (file == NULL) return 1;

  while (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL) {
    char* line = lineBuffer;
    // Strip newline characters from the end of the line
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[len - 1] = '\0';
      len--;
    }

    if (strncmp(line, "screenWidth: ", 13) == 0) {
      sscanf(line + 13, "%d", &appSettings.screenWidth);
    } else if (strncmp(line, "screenHeight: ", 14) == 0) {
      sscanf(line + 14, "%d", &appSettings.screenHeight);
    } else if (strncmp(line, "audioSampleRate: ", 17) == 0) {
      sscanf(line + 17, "%d", &appSettings.audioSampleRate);
    } else if (strncmp(line, "audioBufferSize: ", 17) == 0) {
      sscanf(line + 17, "%d", &appSettings.audioBufferSize);
    } else if (strncmp(line, "aySampleDithering: ", 19) == 0) {
      sscanf(line + 19, "%d", &appSettings.aySampleDithering);
    } else if (strncmp(line, "doubleTapFrames: ", 17) == 0) {
      sscanf(line + 17, "%d", &appSettings.doubleTapFrames);
    } else if (strncmp(line, "keyRepeatDelay: ", 16) == 0) {
      sscanf(line + 16, "%d", &appSettings.keyRepeatDelay);
    } else if (strncmp(line, "keyRepeatSpeed: ", 16) == 0) {
      sscanf(line + 16, "%d", &appSettings.keyRepeatSpeed);
    } else if (strncmp(line, "mixVolume: ", 11) == 0) {
      sscanf(line + 11, "%f", &appSettings.mixVolume);
    } else if (strncmp(line, "quality: ", 9) == 0) {
      sscanf(line + 9, "%d", &appSettings.quality);
    } else if (strncmp(line, "pitchConflictWarning: ", 22) == 0) {
      sscanf(line + 22, "%d", &appSettings.pitchConflictWarning);
    } else if (strncmp(line, "keyUp: ", 7) == 0) {
      sscanf(line + 7, "%d,%d,%d", &appSettings.keyMapping.keyUp[0].code, &appSettings.keyMapping.keyUp[1].code, &appSettings.keyMapping.keyUp[2].code);
    } else if (strncmp(line, "keyDown: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &appSettings.keyMapping.keyDown[0].code, &appSettings.keyMapping.keyDown[1].code, &appSettings.keyMapping.keyDown[2].code);
    } else if (strncmp(line, "keyLeft: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &appSettings.keyMapping.keyLeft[0].code, &appSettings.keyMapping.keyLeft[1].code, &appSettings.keyMapping.keyLeft[2].code);
    } else if (strncmp(line, "keyRight: ", 10) == 0) {
      sscanf(line + 10, "%d,%d,%d", &appSettings.keyMapping.keyRight[0].code, &appSettings.keyMapping.keyRight[1].code, &appSettings.keyMapping.keyRight[2].code);
    } else if (strncmp(line, "keyEdit: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &appSettings.keyMapping.keyEdit[0].code, &appSettings.keyMapping.keyEdit[1].code, &appSettings.keyMapping.keyEdit[2].code);
    } else if (strncmp(line, "keyOpt: ", 8) == 0) {
      sscanf(line + 8, "%d,%d,%d", &appSettings.keyMapping.keyOpt[0].code, &appSettings.keyMapping.keyOpt[1].code, &appSettings.keyMapping.keyOpt[2].code);
    } else if (strncmp(line, "keyPlay: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &appSettings.keyMapping.keyPlay[0].code, &appSettings.keyMapping.keyPlay[1].code, &appSettings.keyMapping.keyPlay[2].code);
    } else if (strncmp(line, "keyShift: ", 10) == 0) {
      sscanf(line + 10, "%d,%d,%d", &appSettings.keyMapping.keyShift[0].code, &appSettings.keyMapping.keyShift[1].code, &appSettings.keyMapping.keyShift[2].code);
    } else if (strncmp(line, "keyUpType: ", 11) == 0) {
      sscanf(line + 11, "%d,%d,%d", (int *)&appSettings.keyMapping.keyUp[0].deviceType, (int *)&appSettings.keyMapping.keyUp[1].deviceType, (int *)&appSettings.keyMapping.keyUp[2].deviceType);
    } else if (strncmp(line, "keyDownType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", (int *)&appSettings.keyMapping.keyDown[0].deviceType, (int *)&appSettings.keyMapping.keyDown[1].deviceType, (int *)&appSettings.keyMapping.keyDown[2].deviceType);
    } else if (strncmp(line, "keyLeftType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", (int *)&appSettings.keyMapping.keyLeft[0].deviceType, (int *)&appSettings.keyMapping.keyLeft[1].deviceType, (int *)&appSettings.keyMapping.keyLeft[2].deviceType);
    } else if (strncmp(line, "keyRightType: ", 14) == 0) {
      sscanf(line + 14, "%d,%d,%d", (int *)&appSettings.keyMapping.keyRight[0].deviceType, (int *)&appSettings.keyMapping.keyRight[1].deviceType, (int *)&appSettings.keyMapping.keyRight[2].deviceType);
    } else if (strncmp(line, "keyEditType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", (int *)&appSettings.keyMapping.keyEdit[0].deviceType, (int *)&appSettings.keyMapping.keyEdit[1].deviceType, (int *)&appSettings.keyMapping.keyEdit[2].deviceType);
    } else if (strncmp(line, "keyOptType: ", 12) == 0) {
      sscanf(line + 12, "%d,%d,%d", (int *)&appSettings.keyMapping.keyOpt[0].deviceType, (int *)&appSettings.keyMapping.keyOpt[1].deviceType, (int *)&appSettings.keyMapping.keyOpt[2].deviceType);
    } else if (strncmp(line, "keyPlayType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", (int *)&appSettings.keyMapping.keyPlay[0].deviceType, (int *)&appSettings.keyMapping.keyPlay[1].deviceType, (int *)&appSettings.keyMapping.keyPlay[2].deviceType);
    } else if (strncmp(line, "keyShiftType: ", 14) == 0) {
      sscanf(line + 14, "%d,%d,%d", (int *)&appSettings.keyMapping.keyShift[0].deviceType, (int *)&appSettings.keyMapping.keyShift[1].deviceType, (int *)&appSettings.keyMapping.keyShift[2].deviceType);
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
    } else if (strncmp(line, "fontPath: ", 10) == 0) {
      strncpy(appSettings.fontPath, line + 10, PATH_LENGTH);
      appSettings.fontPath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "fontFolderPath: ", 16) == 0) {
      strncpy(appSettings.fontFolderPath, line + 16, PATH_LENGTH);
      appSettings.fontFolderPath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "samplePath: ", 12) == 0) {
      strncpy(appSettings.samplePath, line + 12, PATH_LENGTH);
      appSettings.samplePath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "wavetablePath: ", 15) == 0) {
      strncpy(appSettings.wavetablePath, line + 15, PATH_LENGTH);
      appSettings.wavetablePath[PATH_LENGTH] = 0;
    }
  }

  fclose(file);
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
  FILE* file = fopen(path, "w");
  if (file == NULL) return 1;

  fprintf(file, "colorBackground: 0x%06x\n", appSettings.colorScheme.background);
  fprintf(file, "colorTextEmpty: 0x%06x\n", appSettings.colorScheme.textEmpty);
  fprintf(file, "colorTextInfo: 0x%06x\n", appSettings.colorScheme.textInfo);
  fprintf(file, "colorTextDefault: 0x%06x\n", appSettings.colorScheme.textDefault);
  fprintf(file, "colorTextValue: 0x%06x\n", appSettings.colorScheme.textValue);
  fprintf(file, "colorTextTitles: 0x%06x\n", appSettings.colorScheme.textTitles);
  fprintf(file, "colorPlayMarkers: 0x%06x\n", appSettings.colorScheme.playMarkers);
  fprintf(file, "colorCursor: 0x%06x\n", appSettings.colorScheme.cursor);
  fprintf(file, "colorSelection: 0x%06x\n", appSettings.colorScheme.selection);
  fprintf(file, "colorWarning: 0x%06x\n", appSettings.colorScheme.warning);

  fclose(file);
  return 0;
}

int loadTheme(const char* path) {
  FILE* file = fopen(path, "r");
  if (file == NULL) return 1;

  // First reset to defaults to ensure all colors have valid values
  resetToDefaultColors();

  while (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL) {
    char* line = lineBuffer;
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

  fclose(file);
  return 0;
}

const char* getAutosavePath(void) {
  char defaultDir[PATH_LENGTH];
  if (fileGetDefaultDirectory(defaultDir, PATH_LENGTH) != 0) return AUTOSAVE_FILENAME;
  snprintf(autosavePath, sizeof(autosavePath), "%s%s%s", defaultDir, PATH_SEPARATOR_STR, AUTOSAVE_FILENAME);
  return autosavePath;
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
  // Clear the note preview area for all tracks (right side of screen)
  gfxClearRect(35, 3, 5, PROJECT_MAX_TRACKS);
}

void initDefaultKeyMapping(void) {
  // Platform-specific initialization is done in corelib_input
  // This function is kept for API compatibility
  memset(&appSettings.keyMapping, 0, sizeof(KeyMapping));
}

void resetKeyMappingToDefaults(void) {
  // Platform-specific reset is done in corelib_input
  // This function is kept for API compatibility
  memset(&appSettings.keyMapping, 0, sizeof(KeyMapping));
}
