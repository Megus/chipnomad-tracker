#include <string.h>
#include "app_settings.h"
#include "chipnomad_lib.h"

AppSettings::AppSettings() {
  screenWidth = 0; // 0 to auto-detect resolution
  screenHeight = 0;
  audioSampleRate = 44100;
  audioBufferSize = 2048;
  aySampleDithering = 1; // Default: ON
  doubleTapFrames = 20;
  keyRepeatDelay = 16;
  keyRepeatSpeed = 2;
  mixVolume = 1.0f;
  quality = chipnomad::EmulationQuality::medium;
  pitchConflictWarning = 0;

  // Zero out key mapping (platform-specific defaults applied later)
  memset(&keyMapping, 0, sizeof(KeyMapping));

  // Color scheme defaults
  colorTheme.background = 0x000f1a;
  colorTheme.textEmpty = 0x002638;
  colorTheme.textInfo = 0x4878b0;
  colorTheme.textDefault = 0xa0d0f0;
  colorTheme.textValue = 0xe2ebf8;
  colorTheme.textTitles = 0xbfdf50;
  colorTheme.playMarkers = 0xefe000;
  colorTheme.cursor = 0x7ddcff;
  colorTheme.selection = 0x00d090;
  colorTheme.warning = 0xff4040;

  // String defaults
  strncpy(themeName, "Default", kThemeNameLength);
  themeName[kThemeNameLength] = '\0';
  projectFilename[0] = '\0';
  projectPath[0] = '\0';
  pitchTablePath[0] = '\0';
  instrumentPath[0] = '\0';
  themePath[0] = '\0';
  fontPath[0] = '\0';
  fontFolderPath[0] = '\0';
  samplePath[0] = '\0';
  wavetablePath[0] = '\0';
}

bool AppSettings::saveSettings(const char* path) {
  FILE* file = fopen(path, "w");
  if (file == NULL) return false;

  fprintf(file, "screenWidth: %d\n", screenWidth);
  fprintf(file, "screenHeight: %d\n", screenHeight);
  fprintf(file, "audioSampleRate: %d\n", audioSampleRate);
  fprintf(file, "audioBufferSize: %d\n", audioBufferSize);
  fprintf(file, "aySampleDithering: %d\n", aySampleDithering);
  fprintf(file, "doubleTapFrames: %d\n", doubleTapFrames);
  fprintf(file, "keyRepeatDelay: %d\n", keyRepeatDelay);
  fprintf(file, "keyRepeatSpeed: %d\n", keyRepeatSpeed);
  fprintf(file, "mixVolume: %f\n", mixVolume);
  fprintf(file, "quality: %d\n", (int)quality);
  fprintf(file, "pitchConflictWarning: %d\n", pitchConflictWarning ? 1 : 0);

  // Save key mapping codes
  fprintf(file, "keyUp: %d,%d,%d\n", keyMapping.keyUp[0].code, keyMapping.keyUp[1].code, keyMapping.keyUp[2].code);
  fprintf(file, "keyDown: %d,%d,%d\n", keyMapping.keyDown[0].code, keyMapping.keyDown[1].code, keyMapping.keyDown[2].code);
  fprintf(file, "keyLeft: %d,%d,%d\n", keyMapping.keyLeft[0].code, keyMapping.keyLeft[1].code, keyMapping.keyLeft[2].code);
  fprintf(file, "keyRight: %d,%d,%d\n", keyMapping.keyRight[0].code, keyMapping.keyRight[1].code, keyMapping.keyRight[2].code);
  fprintf(file, "keyEdit: %d,%d,%d\n", keyMapping.keyEdit[0].code, keyMapping.keyEdit[1].code, keyMapping.keyEdit[2].code);
  fprintf(file, "keyOpt: %d,%d,%d\n", keyMapping.keyOpt[0].code, keyMapping.keyOpt[1].code, keyMapping.keyOpt[2].code);
  fprintf(file, "keyPlay: %d,%d,%d\n", keyMapping.keyPlay[0].code, keyMapping.keyPlay[1].code, keyMapping.keyPlay[2].code);
  fprintf(file, "keyShift: %d,%d,%d\n", keyMapping.keyShift[0].code, keyMapping.keyShift[1].code, keyMapping.keyShift[2].code);

  // Save key mapping device types
  fprintf(file, "keyUpType: %d,%d,%d\n", keyMapping.keyUp[0].deviceType, keyMapping.keyUp[1].deviceType, keyMapping.keyUp[2].deviceType);
  fprintf(file, "keyDownType: %d,%d,%d\n", keyMapping.keyDown[0].deviceType, keyMapping.keyDown[1].deviceType, keyMapping.keyDown[2].deviceType);
  fprintf(file, "keyLeftType: %d,%d,%d\n", keyMapping.keyLeft[0].deviceType, keyMapping.keyLeft[1].deviceType, keyMapping.keyLeft[2].deviceType);
  fprintf(file, "keyRightType: %d,%d,%d\n", keyMapping.keyRight[0].deviceType, keyMapping.keyRight[1].deviceType, keyMapping.keyRight[2].deviceType);
  fprintf(file, "keyEditType: %d,%d,%d\n", keyMapping.keyEdit[0].deviceType, keyMapping.keyEdit[1].deviceType, keyMapping.keyEdit[2].deviceType);
  fprintf(file, "keyOptType: %d,%d,%d\n", keyMapping.keyOpt[0].deviceType, keyMapping.keyOpt[1].deviceType, keyMapping.keyOpt[2].deviceType);
  fprintf(file, "keyPlayType: %d,%d,%d\n", keyMapping.keyPlay[0].deviceType, keyMapping.keyPlay[1].deviceType, keyMapping.keyPlay[2].deviceType);
  fprintf(file, "keyShiftType: %d,%d,%d\n", keyMapping.keyShift[0].deviceType, keyMapping.keyShift[1].deviceType, keyMapping.keyShift[2].deviceType);

  fprintf(file, "colorBackground: 0x%06x\n", colorTheme.background);
  fprintf(file, "colorTextEmpty: 0x%06x\n", colorTheme.textEmpty);
  fprintf(file, "colorTextInfo: 0x%06x\n", colorTheme.textInfo);
  fprintf(file, "colorTextDefault: 0x%06x\n", colorTheme.textDefault);
  fprintf(file, "colorTextValue: 0x%06x\n", colorTheme.textValue);
  fprintf(file, "colorTextTitles: 0x%06x\n", colorTheme.textTitles);
  fprintf(file, "colorPlayMarkers: 0x%06x\n", colorTheme.playMarkers);
  fprintf(file, "colorCursor: 0x%06x\n", colorTheme.cursor);
  fprintf(file, "colorSelection: 0x%06x\n", colorTheme.selection);
  fprintf(file, "colorWarning: 0x%06x\n", colorTheme.warning);
  fprintf(file, "themeName: %s\n", themeName);

  fprintf(file, "projectFilename: %s\n", projectFilename);
  fprintf(file, "projectPath: %s\n", projectPath);
  fprintf(file, "pitchTablePath: %s\n", pitchTablePath);
  fprintf(file, "instrumentPath: %s\n", instrumentPath);
  fprintf(file, "themePath: %s\n", themePath);
  fprintf(file, "fontPath: %s\n", fontPath);
  fprintf(file, "fontFolderPath: %s\n", fontFolderPath);
  fprintf(file, "samplePath: %s\n", samplePath);
  fprintf(file, "wavetablePath: %s\n", wavetablePath);

  fclose(file);
  return true;
}

bool AppSettings::loadSettings(const char* path) {
  char lineBuffer[1024];

  FILE* file = fopen(path, "r");
  if (file == NULL) return false;

  while (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL) {
    char* line = lineBuffer;
    // Strip newline characters from the end of the line
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[len - 1] = '\0';
      len--;
    }

    if (strncmp(line, "screenWidth: ", 13) == 0) {
      sscanf(line + 13, "%d", &screenWidth);
    } else if (strncmp(line, "screenHeight: ", 14) == 0) {
      sscanf(line + 14, "%d", &screenHeight);
    } else if (strncmp(line, "audioSampleRate: ", 17) == 0) {
      sscanf(line + 17, "%d", &audioSampleRate);
    } else if (strncmp(line, "audioBufferSize: ", 17) == 0) {
      sscanf(line + 17, "%d", &audioBufferSize);
    } else if (strncmp(line, "aySampleDithering: ", 19) == 0) {
      sscanf(line + 19, "%d", &aySampleDithering);
    } else if (strncmp(line, "doubleTapFrames: ", 17) == 0) {
      sscanf(line + 17, "%d", &doubleTapFrames);
    } else if (strncmp(line, "keyRepeatDelay: ", 16) == 0) {
      sscanf(line + 16, "%d", &keyRepeatDelay);
    } else if (strncmp(line, "keyRepeatSpeed: ", 16) == 0) {
      sscanf(line + 16, "%d", &keyRepeatSpeed);
    } else if (strncmp(line, "mixVolume: ", 11) == 0) {
      sscanf(line + 11, "%f", &mixVolume);
    } else if (strncmp(line, "quality: ", 9) == 0) {
      sscanf(line + 9, "%d", &quality);
    } else if (strncmp(line, "pitchConflictWarning: ", 22) == 0) {
      sscanf(line + 22, "%d", &pitchConflictWarning);
    } else if (strncmp(line, "keyUp: ", 7) == 0) {
      sscanf(line + 7, "%d,%d,%d", &keyMapping.keyUp[0].code, &keyMapping.keyUp[1].code, &keyMapping.keyUp[2].code);
    } else if (strncmp(line, "keyDown: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &keyMapping.keyDown[0].code, &keyMapping.keyDown[1].code, &keyMapping.keyDown[2].code);
    } else if (strncmp(line, "keyLeft: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &keyMapping.keyLeft[0].code, &keyMapping.keyLeft[1].code, &keyMapping.keyLeft[2].code);
    } else if (strncmp(line, "keyRight: ", 10) == 0) {
      sscanf(line + 10, "%d,%d,%d", &keyMapping.keyRight[0].code, &keyMapping.keyRight[1].code, &keyMapping.keyRight[2].code);
    } else if (strncmp(line, "keyEdit: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &keyMapping.keyEdit[0].code, &keyMapping.keyEdit[1].code, &keyMapping.keyEdit[2].code);
    } else if (strncmp(line, "keyOpt: ", 8) == 0) {
      sscanf(line + 8, "%d,%d,%d", &keyMapping.keyOpt[0].code, &keyMapping.keyOpt[1].code, &keyMapping.keyOpt[2].code);
    } else if (strncmp(line, "keyPlay: ", 9) == 0) {
      sscanf(line + 9, "%d,%d,%d", &keyMapping.keyPlay[0].code, &keyMapping.keyPlay[1].code, &keyMapping.keyPlay[2].code);
    } else if (strncmp(line, "keyShift: ", 10) == 0) {
      sscanf(line + 10, "%d,%d,%d", &keyMapping.keyShift[0].code, &keyMapping.keyShift[1].code, &keyMapping.keyShift[2].code);
    } else if (strncmp(line, "keyUpType: ", 11) == 0) {
      sscanf(line + 11, "%d,%d,%d", &keyMapping.keyUp[0].deviceType, &keyMapping.keyUp[1].deviceType, &keyMapping.keyUp[2].deviceType);
    } else if (strncmp(line, "keyDownType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", &keyMapping.keyDown[0].deviceType, &keyMapping.keyDown[1].deviceType, &keyMapping.keyDown[2].deviceType);
    } else if (strncmp(line, "keyLeftType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", &keyMapping.keyLeft[0].deviceType, &keyMapping.keyLeft[1].deviceType, &keyMapping.keyLeft[2].deviceType);
    } else if (strncmp(line, "keyRightType: ", 14) == 0) {
      sscanf(line + 14, "%d,%d,%d", &keyMapping.keyRight[0].deviceType, &keyMapping.keyRight[1].deviceType, &keyMapping.keyRight[2].deviceType);
    } else if (strncmp(line, "keyEditType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", &keyMapping.keyEdit[0].deviceType, &keyMapping.keyEdit[1].deviceType, &keyMapping.keyEdit[2].deviceType);
    } else if (strncmp(line, "keyOptType: ", 12) == 0) {
      sscanf(line + 12, "%d,%d,%d", &keyMapping.keyOpt[0].deviceType, &keyMapping.keyOpt[1].deviceType, &keyMapping.keyOpt[2].deviceType);
    } else if (strncmp(line, "keyPlayType: ", 13) == 0) {
      sscanf(line + 13, "%d,%d,%d", &keyMapping.keyPlay[0].deviceType, &keyMapping.keyPlay[1].deviceType, &keyMapping.keyPlay[2].deviceType);
    } else if (strncmp(line, "keyShiftType: ", 14) == 0) {
      sscanf(line + 14, "%d,%d,%d", &keyMapping.keyShift[0].deviceType, &keyMapping.keyShift[1].deviceType, &keyMapping.keyShift[2].deviceType);
    } else if (strncmp(line, "colorBackground: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &colorTheme.background);
    } else if (strncmp(line, "colorTextEmpty: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &colorTheme.textEmpty);
    } else if (strncmp(line, "colorTextInfo: ", 15) == 0) {
      sscanf(line + 15, "0x%x", &colorTheme.textInfo);
    } else if (strncmp(line, "colorTextDefault: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &colorTheme.textDefault);
    } else if (strncmp(line, "colorTextValue: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &colorTheme.textValue);
    } else if (strncmp(line, "colorTextTitles: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &colorTheme.textTitles);
    } else if (strncmp(line, "colorPlayMarkers: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &colorTheme.playMarkers);
    } else if (strncmp(line, "colorCursor: ", 13) == 0) {
      sscanf(line + 13, "0x%x", &colorTheme.cursor);
    } else if (strncmp(line, "colorSelection: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &colorTheme.selection);
    } else if (strncmp(line, "colorWarning: ", 14) == 0) {
      sscanf(line + 14, "0x%x", &colorTheme.warning);
    } else if (strncmp(line, "themeName: ", 11) == 0) {
      strncpy(themeName, line + 11, kThemeNameLength);
      themeName[kThemeNameLength] = 0;
    } else if (strncmp(line, "projectFilename: ", 17) == 0) {
      strncpy(projectFilename, line + 17, kFileNameLength);
      projectFilename[kFileNameLength] = 0;
    } else if (strncmp(line, "projectPath: ", 13) == 0) {
      strncpy(projectPath, line + 13, kPathMaxLength);
      projectPath[kPathMaxLength] = 0;
    } else if (strncmp(line, "pitchTablePath: ", 16) == 0) {
      strncpy(pitchTablePath, line + 16, kPathMaxLength);
      pitchTablePath[kPathMaxLength] = 0;
    } else if (strncmp(line, "instrumentPath: ", 16) == 0) {
      strncpy(instrumentPath, line + 16, kPathMaxLength);
      instrumentPath[kPathMaxLength] = 0;
    } else if (strncmp(line, "themePath: ", 11) == 0) {
      strncpy(themePath, line + 11, kPathMaxLength);
      themePath[kPathMaxLength] = 0;
    } else if (strncmp(line, "fontPath: ", 10) == 0) {
      strncpy(fontPath, line + 10, kPathMaxLength);
      fontPath[kPathMaxLength] = 0;
    } else if (strncmp(line, "fontFolderPath: ", 16) == 0) {
      strncpy(fontFolderPath, line + 16, kPathMaxLength);
      fontFolderPath[kPathMaxLength] = 0;
    } else if (strncmp(line, "samplePath: ", 12) == 0) {
      strncpy(samplePath, line + 12, kPathMaxLength);
      samplePath[kPathMaxLength] = 0;
    } else if (strncmp(line, "wavetablePath: ", 15) == 0) {
      strncpy(wavetablePath, line + 15, kPathMaxLength);
      wavetablePath[kPathMaxLength] = 0;
    }
  }

  fclose(file);
  return true;
}

void AppSettings::resetColorTheme() {
  colorTheme.background = 0x000f1a;
  colorTheme.textEmpty = 0x002638;
  colorTheme.textInfo = 0x4878b0;
  colorTheme.textDefault = 0xa0d0f0;
  colorTheme.textValue = 0xe2ebf8;
  colorTheme.textTitles = 0xbfdf50;
  colorTheme.playMarkers = 0xefe000;
  colorTheme.cursor = 0x7ddcff;
  colorTheme.selection = 0x00d090;
  colorTheme.warning = 0xff4040;
}

bool AppSettings::saveColorTheme(const char* path) {
  FILE* file = fopen(path, "w");
  if (file == NULL) return false;

  fprintf(file, "colorBackground: 0x%06x\n", colorTheme.background);
  fprintf(file, "colorTextEmpty: 0x%06x\n", colorTheme.textEmpty);
  fprintf(file, "colorTextInfo: 0x%06x\n", colorTheme.textInfo);
  fprintf(file, "colorTextDefault: 0x%06x\n", colorTheme.textDefault);
  fprintf(file, "colorTextValue: 0x%06x\n", colorTheme.textValue);
  fprintf(file, "colorTextTitles: 0x%06x\n", colorTheme.textTitles);
  fprintf(file, "colorPlayMarkers: 0x%06x\n", colorTheme.playMarkers);
  fprintf(file, "colorCursor: 0x%06x\n", colorTheme.cursor);
  fprintf(file, "colorSelection: 0x%06x\n", colorTheme.selection);
  fprintf(file, "colorWarning: 0x%06x\n", colorTheme.warning);

  fclose(file);
  return true;
}

bool AppSettings::loadColorTheme(const char* path) {
  FILE* file = fopen(path, "r");
  if (file == NULL) return false;

  resetColorTheme();

  char lineBuffer[1024];

  while (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL) {
    char* line = lineBuffer;
    if (strncmp(line, "colorBackground: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &colorTheme.background);
    } else if (strncmp(line, "colorTextEmpty: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &colorTheme.textEmpty);
    } else if (strncmp(line, "colorTextInfo: ", 15) == 0) {
      sscanf(line + 15, "0x%x", &colorTheme.textInfo);
    } else if (strncmp(line, "colorTextDefault: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &colorTheme.textDefault);
    } else if (strncmp(line, "colorTextValue: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &colorTheme.textValue);
    } else if (strncmp(line, "colorTextTitles: ", 17) == 0) {
      sscanf(line + 17, "0x%x", &colorTheme.textTitles);
    } else if (strncmp(line, "colorPlayMarkers: ", 18) == 0) {
      sscanf(line + 18, "0x%x", &colorTheme.playMarkers);
    } else if (strncmp(line, "colorCursor: ", 13) == 0) {
      sscanf(line + 13, "0x%x", &colorTheme.cursor);
    } else if (strncmp(line, "colorSelection: ", 16) == 0) {
      sscanf(line + 16, "0x%x", &colorTheme.selection);
    } else if (strncmp(line, "colorWarning: ", 14) == 0) {
      sscanf(line + 14, "0x%x", &colorTheme.warning);
    }
  }

  fclose(file);
  return true;
}
