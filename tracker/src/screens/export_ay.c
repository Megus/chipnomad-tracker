#include "screen_export.h"
#include "corelib_gfx.h"
#include "corelib/corelib_file.h"
#include "common.h"
#include "screens.h"
#include "export/export.h"
#include <string.h>

static int getColumnCount(int row) {
  // The first 3 rows come from the common export screen fields
  if (row < SCR_EXPORT_ROWS) return exportCommonColumnCount(row);

  return 1; // Default
}

static void drawStatic(void) {
  exportCommonDrawStatic();
  gfxSetFgColor(appSettings.colorScheme.textValue);
  gfxPrint(0, 8, "PSG");
}

static void drawCursor(int col, int row) {
  if (row < SCR_EXPORT_ROWS) return exportCommonDrawCursor(col, row);
  if (row == SCR_EXPORT_ROWS) {
    gfxCursor(13, 8, 6);
  }
}

static void drawField(int col, int row, int state) {
  if (row < SCR_EXPORT_ROWS) return exportCommonDrawField(col, row, state);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == SCR_EXPORT_ROWS) {
    gfxPrint(13, 8, "Export");
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < SCR_EXPORT_ROWS) return exportCommonOnEdit(col, row, action);

  int handled = 0;

  if (row == SCR_EXPORT_ROWS) {
    // PSG Export
    if (currentExporter) return 1; // Already exporting

    char exportPath[1024];
    generatePSGExportPath(exportPath, sizeof(exportPath));
    strcat(exportPath, ".psg");

    currentExporter = createPSGExporter(exportPath, &chipnomadState->project, startRow);
    if (currentExporter) {
      // Set mix volume from app settings
      currentExporter->chipnomadState->mixVolume = appSettings.mixVolume;
      if (chipnomadState->project.chipsCount > 1) {
        screenMessage(MESSAGE_TIME, "Starting export (%d files)...", chipnomadState->project.chipsCount);
      } else {
        screenMessage(MESSAGE_TIME, "Starting export...");
      }
    } else {
      screenMessage(MESSAGE_TIME, "Export failed to start");
    }
    handled = 1;
  }

  return handled;
}

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}
static void drawSelection(int col1, int row1, int col2, int row2) {}

ScreenData screenExportAY = {
  .rows = 5,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawSelection = drawSelection,
  .drawField = drawField,
  .onEdit = onEdit,
};