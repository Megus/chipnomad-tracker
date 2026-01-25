#include "screen_keymapping.h"
#include "screen_settings.h"
#include "corelib_gfx.h"
#include "common.h"
#include "screens.h"
#include "../platforms/sdl2/corelib_input.h"
#include <string.h>

typedef enum {
  STATE_NAVIGATION,
  STATE_CAPTURING,
  STATE_CAPTURE_DONE
} CaptureState;

static int captureRow = 0;
static int captureCol = 0;
static CaptureState captureState = STATE_NAVIGATION;
static void fullRedraw(void);

static const char* buttonNames[] = {
  "Up", "Down", "Left", "Right", "Edit (A)", "Opt (B)", "Play", "Shift"
};

static int32_t* getKeySlot(int row, int col) {
  switch (row) {
    case 0: return &appSettings.keyMapping.keyUp[col];
    case 1: return &appSettings.keyMapping.keyDown[col];
    case 2: return &appSettings.keyMapping.keyLeft[col];
    case 3: return &appSettings.keyMapping.keyRight[col];
    case 4: return &appSettings.keyMapping.keyEdit[col];
    case 5: return &appSettings.keyMapping.keyOpt[col];
    case 6: return &appSettings.keyMapping.keyPlay[col];
    case 7: return &appSettings.keyMapping.keyShift[col];
  }
  return NULL;
}

static void clearConflicts(int32_t keyCode, int skipRow, int skipCol) {
  if (keyCode == 0) return;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 3; col++) {
      if (row == skipRow && col == skipCol) continue;
      int32_t* slot = getKeySlot(row, col);
      if (slot && *slot == keyCode) {
        *slot = 0;
      }
    }
  }
}

static void onRawInput(int32_t keyCode, int isDown) {
  if (!isDown) return;

  int32_t* slot = getKeySlot(captureRow, captureCol);
  if (slot) {
    clearConflicts(keyCode, captureRow, captureCol);
    *slot = keyCode;
  }

  inputRawCallback = NULL;
  captureState = STATE_CAPTURE_DONE;
  fullRedraw();
}

static int getColumnCount(int row) {
  if (row < 8) return 3;
  return 1;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "Key Mapping");

  gfxSetFgColor(appSettings.colorScheme.textDefault);
  for (int i = 0; i < 8; i++) {
    gfxPrint(0, i + 2, buttonNames[i]);
  }

  if (inputRawCallback) {
    gfxSetFgColor(appSettings.colorScheme.textValue);
    gfxPrint(0, 14, "Press any key...");
  }
}

static void drawCursor(int col, int row) {
  if (row < 8) {
    int32_t* slot = getKeySlot(row, col);
    int width = slot ? strlen(inputGetKeyName(*slot)) : 3;
    gfxCursor(11 + col * 8, row + 2, width);
  } else if (row == 8) {
    gfxCursor(0, 11, 17);
  } else if (row == 9) {
    gfxCursor(0, 12, 4);
  }
}

static void drawField(int col, int row, int state) {
  if (row < 8) {
    gfxClearRect(11 + col * 8, row + 2, 7, 1);
    int32_t* slot = getKeySlot(row, col);
    if (slot) {
      int isEmpty = (*slot == 0);
      if (state == stateFocus) {
        gfxSetFgColor(appSettings.colorScheme.textValue);
      } else if (isEmpty) {
        gfxSetFgColor(appSettings.colorScheme.textEmpty);
      } else {
        gfxSetFgColor(appSettings.colorScheme.textDefault);
      }
      gfxPrint(11 + col * 8, row + 2, inputGetKeyName(*slot));
    }
  } else if (row == 8) {
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    gfxPrint(0, 11, "Reset to defaults");
  } else if (row == 9) {
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    gfxPrint(0, 12, "Done");
  }
}

static void drawRowHeader(int row, int state) {
}

static void drawColHeader(int col, int state) {
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 8) {
    if (action == editTap) {
      captureRow = row;
      captureCol = col;
      captureState = STATE_CAPTURING;
      inputRawCallback = onRawInput;
      fullRedraw();
      return 1;
    } else if (action == editClear) {
      int32_t* slot = getKeySlot(row, col);
      if (slot) *slot = 0;
      return 1;
    }
  } else if (row == 8 && action == editTap) {
    inputInitDefaultKeyMapping();
    return 1;
  } else if (row == 9) {
    settingsSave();
    screenSetup(&screenSettings, 0);
    return 1;
  }

  return 0;
}

static ScreenData screenKeyMappingData = {
  .rows = 10,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

static void setup(int input) {
}

static void fullRedraw(void) {
  screenFullRedraw(&screenKeyMappingData);
}

static void draw(void) {
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (captureState == STATE_CAPTURING) return 1;
  if (captureState == STATE_CAPTURE_DONE) {
    captureState = STATE_NAVIGATION;
    return 1;
  }
  return screenInput(&screenKeyMappingData, isKeyDown, keys, tapCount);
}

const AppScreen screenKeyMapping = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
