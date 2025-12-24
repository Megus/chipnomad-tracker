#include "screens.h"
#include "corelib/corelib_file.h"
#include "corelib_gfx.h"
#include "file_browser.h"
#include "chipnomad_lib.h"
#include <string.h>
#include "pitch_table_utils.h"

static int isCharEdit = 0;
static char* editingString = NULL;
static int editingStringLength = 0;

static void onLoadSelected(const char* path) {
  if (pitchTableLoadCSV(&chipnomadState->project, path) == 0) {
    extractFilenameWithoutExtension(path, chipnomadState->project.pitchTable.name, PROJECT_PITCH_TABLE_TITLE_LENGTH + 1);

    // Save the directory path
    char* lastSeparator = strrchr(path, PATH_SEPARATOR);
    if (lastSeparator) {
      int pathLen = lastSeparator - path;
      strncpy(appSettings.pitchTablePath, path, pathLen);
      appSettings.pitchTablePath[pathLen] = 0;
    }
  }
  screenSetup(&screenPitchTable, 0);
}

static void onSaveSelected(const char* folderPath) {
  pitchTableSaveCSV(&chipnomadState->project, folderPath, chipnomadState->project.pitchTable.name);

  // Save the directory path
  strncpy(appSettings.pitchTablePath, folderPath, PATH_LENGTH);
  appSettings.pitchTablePath[PATH_LENGTH] = 0;

  screenSetup(&screenPitchTable, 0);
}

static void onCancelled(void) {
  screenSetup(&screenPitchTable, 0);
}

static int getColumnCount(int row) {
  if (row == 0) return PROJECT_PITCH_TABLE_TITLE_LENGTH; // Name field
  return 1; // Options
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "PITCH TABLE");

  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 2, "Name");
}

static void drawCursor(int col, int row) {
  if (row == 0) {
    gfxCursor(7 + col, 2, 1);
  } else if (row == 1) {
    gfxCursor(0, 4, 13);
  } else if (row == 2) {
    gfxCursor(0, 5, 11);
  } else if (row == 3) {
    gfxCursor(0, 6, 26);
  }
}

static void drawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 0) {
    gfxClearRect(7, 2, PROJECT_PITCH_TABLE_TITLE_LENGTH, 1);
    gfxPrintf(7, 2, "%s", chipnomadState->project.pitchTable.name);
  } else if (row == 1) {
    gfxPrint(0, 4, "Load from CSV");
  } else if (row == 2) {
    gfxPrint(0, 5, "Save to CSV");
  } else if (row == 3) {
    gfxPrint(0, 6, "Generate for current clock");
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (row == 0) {
    // Name editing
    int res = editCharacter(action, chipnomadState->project.pitchTable.name, col, PROJECT_PITCH_TABLE_TITLE_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
      editingString = chipnomadState->project.pitchTable.name;
      editingStringLength = PROJECT_PITCH_TABLE_TITLE_LENGTH;
    } else if (res > 1) {
      handled = 1;
    }
  } else if (row == 1) {
    // Load from CSV
    fileBrowserSetup("LOAD PITCH TABLE", ".csv", appSettings.pitchTablePath, onLoadSelected, onCancelled);
    screenSetup(&screenFileBrowser, 0);
  } else if (row == 2) {
    // Save to CSV
    fileBrowserSetupFolderMode("SAVE PITCH TABLE", appSettings.pitchTablePath, chipnomadState->project.pitchTable.name, ".csv", onSaveSelected, onCancelled);
    screenSetup(&screenFileBrowser, 0);
  } else if (row == 3) {
    // Generate for current clock
    calculatePitchTableAY(&chipnomadState->project);
    drawField(0, 0, 0); // Still needed to redraw the pitch table name
    handled = 1;
  }

  return handled;
}

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}
static void drawSelection(int col1, int row1, int col2, int row2) {}

static ScreenData screenPitchTableData = {
  .rows = 4,
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

static void setup(int input) {
  isCharEdit = 0;
  editingString = NULL;
  editingStringLength = 0;
}

static void fullRedraw(void) {
  screenFullRedraw(&screenPitchTableData);
}

static void draw(void) {
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == keyOpt) {
    screenSetup(&screenProject, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (isCharEdit) {
    char result = charEditInput(keys, isDoubleTap, editingString, screenPitchTableData.cursorCol, editingStringLength);
    if (result) {
      isCharEdit = 0;
      if (screenPitchTableData.cursorCol < editingStringLength - 1) screenPitchTableData.cursorCol++;
      editingString = NULL;
      editingStringLength = 0;
      fullRedraw();
    }
  } else {
    if (inputScreenNavigation(keys, isDoubleTap)) return 1;
    if (screenInput(&screenPitchTableData, isKeyDown, keys, isDoubleTap)) return 1;
  }
  return 0;
}

const AppScreen screenPitchTable = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
