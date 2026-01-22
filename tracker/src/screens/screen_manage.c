#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}

static ScreenData screenManageData = {
  .rows = 3,
  .cursorRow = 1,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = manageColumnCount,
  .drawStatic = manageDrawStatic,
  .drawCursor = manageDrawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = manageDrawField,
  .onEdit = manageOnEdit,
};

static void init(void) {
  screenManageData.cursorRow = 1;
  screenManageData.cursorCol = 0;
}

static void setup(int input) {
}

static void fullRedraw(void) {
  screenFullRedraw(&screenManageData);
}

static void draw(void) {
}

int manageColumnCount(int row) {
  return 1;
}

void manageDrawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "MANAGE PROJECT");
  
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 2, "Clean unused/duplicate:");
}

void manageDrawCursor(int col, int row) {
  if (row == 1) {
    gfxCursor(2, 3, 18); // "Phrases and Chains"
  } else if (row == 2) {
    gfxCursor(2, 4, 22); // "Instruments and Tables"
  }
}

void manageDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
  
  if (row == 1) {
    gfxPrint(2, 3, "Phrases and Chains");
  } else if (row == 2) {
    gfxPrint(2, 4, "Instruments and Tables");
  }
}

int manageOnEdit(int col, int row, enum CellEditAction action) {
  if (action != editTap && action != editDoubleTap) return 0;
  
  if (row == 1) {
    // Cleanup phrases and chains
    int phrasesFreed, chainsFreed;
    cleanupPhrasesAndChains(&chipnomadState->project, &phrasesFreed, &chainsFreed);
    if (phrasesFreed > 0 || chainsFreed > 0) {
      screenMessage(MESSAGE_TIME, "Freed %d phrases, %d chains", phrasesFreed, chainsFreed);
    } else {
      screenMessage(MESSAGE_TIME, "Nothing to clean");
    }
  } else if (row == 2) {
    // Cleanup instruments and tables
    int instrumentsFreed, tablesFreed;
    cleanupInstrumentsAndTables(&chipnomadState->project, &instrumentsFreed, &tablesFreed);
    if (instrumentsFreed > 0 || tablesFreed > 0) {
      screenMessage(MESSAGE_TIME, "Freed %d instruments, %d tables", instrumentsFreed, tablesFreed);
    } else {
      screenMessage(MESSAGE_TIME, "Nothing to clean");
    }
  }
  
  return 1;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (keys == keyOpt) {
    screenSetup(&screenProject, 0);
    return 1;
  }
  
  return screenInput(&screenManageData, isKeyDown, keys, tapCount);
}

const AppScreen screenManage = {
  .init = init,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
