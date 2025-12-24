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

static ScreenData screenExportCommon = {
  .rows = 3,
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

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
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
  return screenInput(screen, isKeyDown, keys, isDoubleTap);
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
    return 1; // WAV export
  } else if (row == 1) {
    return 1; // Sample rate
  } else if (row == 2) {
    return 1; // Bit depth
  }
  return 0;
}

void exportCommonDrawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "EXPORT");

  gfxSetFgColor(cs.textValue);
  gfxPrint(0, 2, "WAV");
  gfxSetFgColor(cs.textDefault);
  gfxPrint(0, 3, "Sample rate");
  gfxPrint(0, 4, "Bit depth");
}

void exportCommonDrawCursor(int col, int row) {
  if (row == 0) {
    gfxCursor(13, 2, 6); // Export
  } else if (row == 1) {
    gfxCursor(13, 3, 5); // Sample rate
  } else if (row == 2) {
    gfxCursor(13, 4, 2); // Bit depth
  }
}

void exportCommonDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 0) {
    gfxPrint(13, 2, "Export");
  } else if (row == 1) {
    gfxClearRect(13, 3, 6, 1);
    gfxPrintf(13, 3, "%d", sampleRates[currentSampleRateIndex]);
  } else if (row == 2) {
    gfxClearRect(13, 4, 2, 1);
    gfxPrintf(13, 4, "%d", bitDepths[currentBitDepthIndex]);
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

static int psgFilesExist(const char* basePath, int numChips) {
  for (int i = 0; i < numChips; i++) {
    char filename[1024];
    if (numChips > 1) {
      snprintf(filename, sizeof(filename), "%s-%d.psg", basePath, i + 1);
    } else {
      snprintf(filename, sizeof(filename), "%s.psg", basePath);
    }
    if (fileExists(filename)) return 1;
  }
  return 0;
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

  // Fallback if all numbers are taken
  strncpy(outputPath, basePath, maxLen - 1);
  outputPath[maxLen - 1] = 0;
}

void generatePSGExportPath(char* outputPath, int maxLen) {
  char basePath[512];
  snprintf(basePath, sizeof(basePath), "%s%s%s",
    appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename);

  if (!psgFilesExist(basePath, chipnomadState->project.chipsCount)) {
    strncpy(outputPath, basePath, maxLen - 1);
    outputPath[maxLen - 1] = 0;
    return;
  }

  for (int i = 1; i <= 999; i++) {
    snprintf(outputPath, maxLen, "%s%s%s_%03d",
      appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename, i);
    if (!psgFilesExist(outputPath, chipnomadState->project.chipsCount)) {
      return;
    }
  }

  // Fallback
  snprintf(outputPath, maxLen, "%s%s%s",
    appSettings.projectPath, PATH_SEPARATOR_STR, appSettings.projectFilename);
}

int exportCommonOnEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (row == 0) {
    // WAV Export
    if (currentExporter) return 1; // Already exporting

    char exportPath[1024];
    generateExportPath(exportPath, sizeof(exportPath), "wav");

    currentExporter = createWAVExporter(exportPath, &chipnomadState->project, 0, sampleRates[currentSampleRateIndex], bitDepths[currentBitDepthIndex]);
    if (currentExporter) {
      // Set mix volume from app settings
      currentExporter->chipnomadState->mixVolume = appSettings.mixVolume;
      screenMessage(MESSAGE_TIME, "Starting export...");
    } else {
      screenMessage(MESSAGE_TIME, "Export failed to start");
    }
    handled = 1;
  } else if (row == 1) {
    // Sample rate selection
    if (action == editIncrease) {
      currentSampleRateIndex = (currentSampleRateIndex + 1) % 4;
      handled = 1;
    } else if (action == editDecrease) {
      currentSampleRateIndex = (currentSampleRateIndex + 3) % 4;
      handled = 1;
    }
  } else if (row == 2) {
    // Bit depth selection
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
