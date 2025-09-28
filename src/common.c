#include <common.h>
#include <corelib_file.h>
#include <string.h>

#ifdef DESKTOP_BUILD
#define DEFAULT_VOLUME (1.0)
#else
#define DEFAULT_VOLUME (0.5)
#endif

#define SETTINGS_FILENAME "settings.txt"

struct AppSettings appSettings = {
  .audioSampleRate = 44100,
  .audioBufferSize = 2048,
  .doubleTapFrames = 10,
  .keyRepeatDelay = 16,
  .keyRepeatSpeed = 2,
  .volume = DEFAULT_VOLUME,
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
  },
  .projectFilename = "",
  .projectPath = "",
  .instrumentPath = ""
};

int* pSongRow;
int* pSongTrack;
int* pChainRow;
struct PlaybackState playback;

int settingsSave(void) {
  int fileId = fileOpen(SETTINGS_FILENAME, 1);
  if (fileId == -1) return 1;

  filePrintf(fileId, "audioSampleRate: %d\n", appSettings.audioSampleRate);
  filePrintf(fileId, "audioBufferSize: %d\n", appSettings.audioBufferSize);
  filePrintf(fileId, "doubleTapFrames: %d\n", appSettings.doubleTapFrames);
  filePrintf(fileId, "keyRepeatDelay: %d\n", appSettings.keyRepeatDelay);
  filePrintf(fileId, "keyRepeatSpeed: %d\n", appSettings.keyRepeatSpeed);
  filePrintf(fileId, "volume: %f\n", appSettings.volume);
  filePrintf(fileId, "colorBackground: 0x%06x\n", appSettings.colorScheme.background);
  filePrintf(fileId, "colorTextEmpty: 0x%06x\n", appSettings.colorScheme.textEmpty);
  filePrintf(fileId, "colorTextInfo: 0x%06x\n", appSettings.colorScheme.textInfo);
  filePrintf(fileId, "colorTextDefault: 0x%06x\n", appSettings.colorScheme.textDefault);
  filePrintf(fileId, "colorTextValue: 0x%06x\n", appSettings.colorScheme.textValue);
  filePrintf(fileId, "colorTextTitles: 0x%06x\n", appSettings.colorScheme.textTitles);
  filePrintf(fileId, "colorPlayMarkers: 0x%06x\n", appSettings.colorScheme.playMarkers);
  filePrintf(fileId, "colorCursor: 0x%06x\n", appSettings.colorScheme.cursor);
  filePrintf(fileId, "colorSelection: 0x%06x\n", appSettings.colorScheme.selection);
  filePrintf(fileId, "projectFilename: %s\n", appSettings.projectFilename);
  filePrintf(fileId, "projectPath: %s\n", appSettings.projectPath);
  filePrintf(fileId, "instrumentPath: %s\n", appSettings.instrumentPath);

  fileClose(fileId);
  return 0;
}

int settingsLoad(void) {
  int fileId = fileOpen(SETTINGS_FILENAME, 0);
  if (fileId == -1) return 1;

  char* line;
  while ((line = fileReadString(fileId)) != NULL) {
    if (strncmp(line, "audioSampleRate: ", 17) == 0) {
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
    } else if (strncmp(line, "projectFilename: ", 17) == 0) {
      strncpy(appSettings.projectFilename, line + 17, FILENAME_LENGTH);
      appSettings.projectFilename[FILENAME_LENGTH] = 0;
    } else if (strncmp(line, "projectPath: ", 13) == 0) {
      strncpy(appSettings.projectPath, line + 13, PATH_LENGTH);
      appSettings.projectPath[PATH_LENGTH] = 0;
    } else if (strncmp(line, "instrumentPath: ", 16) == 0) {
      strncpy(appSettings.instrumentPath, line + 16, PATH_LENGTH);
      appSettings.instrumentPath[PATH_LENGTH] = 0;
    }
  }

  fileClose(fileId);
  return 0;
}

void extractFilenameWithoutExtension(const char* path, char* output, int maxLength) {
  // Extract filename from path
  const char* filename = strrchr(path, '/');
  if (filename) {
    filename++; // Skip the '/'
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

