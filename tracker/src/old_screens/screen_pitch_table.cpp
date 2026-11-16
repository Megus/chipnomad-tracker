#include "screens.h"
#include "file_system.h"
#include "gfx.h"
#include "file_browser.h"
#include "chipnomad_lib.h"
#include <string.h>
#include "pitch_table_utils.h"

static int isCharEdit = 0;
static char* editingString = NULL;
static int editingStringLength = 0;

static void onLoadSelected(const char* path) {
  if (pitchTableLoadCSV(&chipnomadState->project, path)) {
    extractFilenameWithoutExtension(path, state.project->pitchTable.name, PROJECT_PITCH_TABLE_TITLE_LENGTH + 1);

    // Save the directory path
    const char* lastSeparator = strrchr(path, PATH_SEPARATOR);
    if (lastSeparator) {
      int pathLen = lastSeparator - path;
      copyString(appSettings.pitchTablePath, path, pathLen);
    }
    projectModified = 1;
  }
  screenSetup(&screenPitchTable, 0);
}

static void onSaveSelected(const char* folderPath) {
  pitchTableSaveCSV(&chipnomadState->project, folderPath, state.project->pitchTable.name);

  // Save the directory path
  copyString(appSettings.pitchTablePath, folderPath, PATH_LENGTH);

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
  gfx.setFgColor(theme().textTitles);
  gfx.print(0, 0, "PITCH TABLE");

  gfx.setFgColor(theme().textDefault);
  gfx.print(0, 2, "Name");
}

static void drawCursor(int col, int row) {
  if (row == 0) {
    gfx.cursor(7 + col, 2, 1);
  } else if (row == 1) {
    gfx.cursor(0, 4, 13);
  } else if (row == 2) {
    gfx.cursor(0, 5, 11);
  } else if (row == 3) {
    gfx.cursor(0, 6, 26);
  }
}

static void drawField(int col, int row, CellState state) {
  gfx.setFgColor(state == CellState::focus ? theme().textValue : theme().textDefault);

  if (row == 0) {
    gfx.clearRect(7, 2, PROJECT_PITCH_TABLE_TITLE_LENGTH, 1);
    gfx.printf(7, 2, "%s", state.project->pitchTable.name);
  } else if (row == 1) {
    gfx.print(0, 4, "Load from CSV");
  } else if (row == 2) {
    gfx.print(0, 5, "Save to CSV");
  } else if (row == 3) {
    gfx.print(0, 6, "Generate for current clock");
  }
}

static int onEdit(int col, int row, EditCellAction action) {
  int handled = 0;

  if (row == 0) {
    // Name editing
    int res = editCharacter(action, state.project->pitchTable.name, col, PROJECT_PITCH_TABLE_TITLE_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
      editingString = state.project->pitchTable.name;
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
    fileBrowserSetupFolderMode("SAVE PITCH TABLE", appSettings.pitchTablePath, state.project->pitchTable.name, ".csv", onSaveSelected, onCancelled);
    screenSetup(&screenFileBrowser, 0);
  } else if (row == 3) {
    // Generate for current clock
    if (state.project->linearPitch) {
      calculateLinearPitchTable12TET(&chipnomadState->project);
    } else {
      calculatePitchTableAY(&chipnomadState->project);
    }

    projectModified = 1;
    drawField(0, 0, CellState::normal); // Still needed to redraw the pitch table name
    handled = 1;
  }

  return handled;
}

static void drawRowHeader(int row, CellState state) {}
static void drawColHeader(int col, CellState state) {}
static void drawSelection(int col1, int row1, int col2, int row2) {}

static ScreenData screenPitchTableData = {
  .rows = 4,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = -1,
  .selectStartRow = 0,
  .selectStartCol = 0,
  .selectAnchorRow = 0,
  .selectAnchorCol = 0,
  .playbackLevel = ScreenPlaybackLevel::none,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawSelection = drawSelection,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
  .onInput = NULL,
  .onRawInput = NULL,
  .isCellValid = NULL,
  .getLoopRange = NULL,
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

static int inputScreenNavigation(int keys, int tapCount) {
  if (keys == keyOpt) {
    screenSetup(&screenProject, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (isCharEdit) {
    char result = charEditInput(keys, tapCount, editingString, screenPitchTableData.cursorCol, editingStringLength);
    if (result) {
      projectModified = 1;
      isCharEdit = 0;
      if (screenPitchTableData.cursorCol < editingStringLength - 1) screenPitchTableData.cursorCol++;
      editingString = NULL;
      editingStringLength = 0;
      fullRedraw();
    }
  } else {
    if (inputScreenNavigation(keys, tapCount)) return 1;
    if (screenInput(&screenPitchTableData, isKeyDown, keys, tapCount)) return 1;
  }
  return 0;
}

const AppScreen screenPitchTable = {
  .init = NULL,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .getPlaybackLevel = NULL,
};
