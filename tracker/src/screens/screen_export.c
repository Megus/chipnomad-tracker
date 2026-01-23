#include "screen_export.h"
#include "common.h"
#include "corelib_gfx.h"
#include "corelib/corelib_file.h"
#include "chipnomad_lib.h"
#include "screens.h"
#include "export/export.h"
#include <string.h>

// Export state
Exporter* currentExporter = NULL;

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}
static void drawSelection(int col1, int row1, int col2, int row2) {}

static int sampleRates[] = {44100, 48000, 88200, 96000};
static int bitDepths[] = {16, 24, 32};
static int currentSampleRateIndex = 0;
static int currentBitDepthIndex = 0;
int startRow = 0;

static ScreenData screenExportCommon = {
  .rows = 4,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = exportCommonColumnCount,
  .drawStatic = exportCommonDrawStatic,
  .drawCursor = exportCommonDrawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawSelection = drawSelection,
  .drawField = exportCommonDrawField,
  .onEdit = exportCommonOnEdit,
};

static ScreenData* exportScreen(void) {
  ScreenData* data = &screenExportCommon;
  if (chipnomadState->project.chipType == chipAY) {
    data = &screenExportAY;
  }
  return data;
}

static void setup(int input) {
  currentSampleRateIndex = 0;
  currentBitDepthIndex = 0;
  startRow = 0;
}

static void fullRedraw(void) {
  ScreenData* screen = exportScreen();
  screenFullRedraw(screen);
}

static void draw(void) {
  if (currentExporter) {
    int seconds = currentExporter->next(currentExporter);
    if (seconds == -1) {
      if (currentExporter->finish(currentExporter) == 0) {
        screenMessage(MESSAGE_TIME, "Export completed");
      } else {
        screenMessage(MESSAGE_TIME, "Export failed");
      }
      currentExporter = NULL;
    } else {
      screenMessage(MESSAGE_TIME, "Exporting... %ds. B to cancel", seconds);
    }
  }
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (currentExporter) {
    if (keys == keyOpt) {
      currentExporter->cancel(currentExporter);
      currentExporter = NULL;
      screenMessage(MESSAGE_TIME, "Export cancelled");
    }
    return 1; // Block all other input during export
  }

  if (keys == keyOpt) {
    screenSetup(&screenProject, 0);
    return 1;
  }

  ScreenData* screen = exportScreen();
  return screenInput(screen, isKeyDown, keys, tapCount);
}

const AppScreen screenExport = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};

///////////////////////////////////////////////////////////////////////////////
//
// Common part of the export screen
//

int exportCommonColumnCount(int row) {
  if (row == 0) {
    return 1;
  } else if (row == 1) {
    return 2;
  } else if (row == 2) {
    return 1;
  } else if (row == 3) {
    return 1;
  }
  return 0;
}

void exportCommonDrawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "EXPORT");

  gfxSetFgColor(cs.textDefault);
  gfxPrint(0, 2, "Start row");

  gfxSetFgColor(cs.textValue);
  gfxPrint(0, 4, "WAV");
  gfxSetFgColor(cs.textDefault);
  gfxPrint(0, 5, "Sample rate");
  gfxPrint(0, 6, "Bit depth");
}

void exportCommonDrawCursor(int col, int row) {
  if (row == 0) {
    gfxCursor(13, 2, 2);
  } else if (row == 1) {
    if (col == 0) {
      gfxCursor(13, 4, 6);
    } else {
      gfxCursor(20, 4, 5);
    }
  } else if (row == 2) {
    gfxCursor(13, 5, 5);
  } else if (row == 3) {
    gfxCursor(13, 6, 2);
  }
}

void exportCommonDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 0) {
    gfxClearRect(13, 2, 2, 1);
    gfxPrintf(13, 2, "%02X", startRow);
  } else if (row == 1) {
    if (col == 0) {
      gfxPrint(13, 4, "Export");
    } else {
      gfxPrint(20, 4, "Stems");
    }
  } else if (row == 2) {
    gfxClearRect(13, 5, 6, 1);
    gfxPrintf(13, 5, "%d", sampleRates[currentSampleRateIndex]);
  } else if (row == 3) {
    gfxClearRect(13, 6, 2, 1);
    gfxPrintf(13, 6, "%d", bitDepths[currentBitDepthIndex]);
  }
}

static int fileExists(const char* path) {
  int fileId = fileOpen(path, 0);
  if (fileId != -1) {
    fileClose(fileId);
    return 1;
  }
  return 0;
}

typedef int (*FileExistsCheckFunc)(const char* basePath, int count);

static int multiFileExists(const char* basePath, int count, const char* format) {
  for (int i = 0; i < count; i++) {
    char filename[1024];
    snprintf(filename, sizeof(filename), format, basePath, i + 1);
    if (fileExists(filename)) return 1;
  }
  return 0;
}

static int psgFilesExist(const char* basePath, int numChips) {
  if (numChips == 1) {
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s.psg", basePath);
    return fileExists(filename);
  }
  return multiFileExists(basePath, numChips, "%s-%d.psg");
}

static int stemsFilesExist(const char* basePath, int trackCount) {
  return multiFileExists(basePath, trackCount, "%s-%02d.wav");
}

static void generateMultiFileExportPath(char* outputPath, int maxLen, FileExistsCheckFunc checkFunc, int count) {
  char basePath[512];
  snprintf(basePath, sizeof(basePath), "%s%s%s",
    appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename);

  if (!checkFunc(basePath, count)) {
    strncpy(outputPath, basePath, maxLen - 1);
    outputPath[maxLen - 1] = 0;
    return;
  }

  for (int i = 1; i <= 999; i++) {
    snprintf(outputPath, maxLen, "%s%s%s_%03d",
      appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename, i);
    if (!checkFunc(outputPath, count)) {
      return;
    }
  }

  snprintf(outputPath, maxLen, "%s%s%s",
    appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename);
}

void generateStemsExportPath(char* outputPath, int maxLen) {
  int trackCount = chipnomadState->project.chipsCount * 3;
  generateMultiFileExportPath(outputPath, maxLen, stemsFilesExist, trackCount);
}

void generatePSGExportPath(char* outputPath, int maxLen) {
  generateMultiFileExportPath(outputPath, maxLen, psgFilesExist, chipnomadState->project.chipsCount);
}

void generateExportPath(char* outputPath, int maxLen, const char* extension) {
  char basePath[512];
  snprintf(basePath, sizeof(basePath), "%s%s%s.%s",
    appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename, extension);

  if (!fileExists(basePath)) {
    strncpy(outputPath, basePath, maxLen - 1);
    outputPath[maxLen - 1] = 0;
    return;
  }

  for (int i = 1; i <= 999; i++) {
    snprintf(outputPath, maxLen, "%s%s%s_%03d.%s",
      appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename, i, extension);
    if (!fileExists(outputPath)) {
      return;
    }
  }

  strncpy(outputPath, basePath, maxLen - 1);
  outputPath[maxLen - 1] = 0;
}

int exportCommonOnEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (row == 0) {
    handled = edit8noLast(action, (uint8_t*)&startRow, 16, 0, PROJECT_MAX_LENGTH - 1);
  } else if (row == 1 && col == 0) {
    if (currentExporter) return 1;

    char exportPath[1024];
    generateExportPath(exportPath, sizeof(exportPath), "wav");

    currentExporter = createWAVExporter(exportPath, &chipnomadState->project, startRow, sampleRates[currentSampleRateIndex], bitDepths[currentBitDepthIndex]);
    if (currentExporter) {
      currentExporter->chipnomadState->mixVolume = appSettings.mixVolume;
      screenMessage(MESSAGE_TIME, "Starting export...");
    } else {
      screenMessage(MESSAGE_TIME, "Export failed to start");
    }
    handled = 1;
  } else if (row == 1 && col == 1) {
    if (currentExporter) return 1;

    char basePath[512];
    generateStemsExportPath(basePath, sizeof(basePath));

    currentExporter = createWAVStemsExporter(basePath, &chipnomadState->project, startRow, sampleRates[currentSampleRateIndex], bitDepths[currentBitDepthIndex]);
    if (currentExporter) {
      currentExporter->chipnomadState->mixVolume = appSettings.mixVolume;
      int trackCount = chipnomadState->project.chipsCount * 3;
      screenMessage(MESSAGE_TIME, "Starting stems export (%d files)...", trackCount);
    } else {
      screenMessage(MESSAGE_TIME, "Export failed to start");
    }
    handled = 1;
  } else if (row == 2) {
    if (action == editIncrease) {
      currentSampleRateIndex = (currentSampleRateIndex + 1) % 4;
      handled = 1;
    } else if (action == editDecrease) {
      currentSampleRateIndex = (currentSampleRateIndex + 3) % 4;
      handled = 1;
    }
  } else if (row == 3) {
    if (action == editIncrease) {
      currentBitDepthIndex = (currentBitDepthIndex + 1) % 3;
      handled = 1;
    } else if (action == editDecrease) {
      currentBitDepthIndex = (currentBitDepthIndex + 2) % 3;
      handled = 1;
    }
  }

  return handled;
}
