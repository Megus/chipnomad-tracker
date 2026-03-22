#include "screen_keymapping.h"
#include "screen_settings.h"
#include "corelib_gfx.h"
#include "corelib_file.h"
#include "common.h"
#include "screens.h"
#include "corelib_input.h"
#include "app.h"
#include <string.h>

typedef enum {
  STATE_NAVIGATION,
  STATE_CAPTURING,
  STATE_CAPTURE_DONE
} CaptureState;

static int captureRow = 0;
static int captureCol = 0;
static CaptureState captureState = STATE_NAVIGATION;
static InputCode pendingCapture = {inputNone, 0};
static void fullRedraw(void);

static const char* buttonNames[] = {
  "Up", "Down", "Left", "Right", "Edit (A)", "Opt (B)", "Play", "Shift"
};

static InputCode* getKeySlot(int row, int col) {
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

static void onRawInput(InputCode input, int isDown) {
  if (!isDown) return;

  // Logical buttons (e.g. touch vpad) are not remappable
  if (input.deviceType == inputLogical) {
    inputRawCallback = NULL;
    captureState = STATE_CAPTURE_DONE;
    return;
  }

  pendingCapture = input;
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
  } else {
    gfxSetFgColor(appSettings.colorScheme.textInfo);
    gfxPrint(0, 14, "Tap unmapped key 5x");
    gfxPrint(0, 15, "to reset defaults");
  }
}

static void drawCursor(int col, int row) {
  if (row < 8) {
    InputCode* slot = getKeySlot(row, col);
    int width = slot ? strlen(inputGetKeyName(*slot)) : 3;
    gfxCursor(11 + col * 8, row + 2, width);
  } else if (row == 8) {
    gfxCursor(0, 11, 4);
  }
}

static void drawField(int col, int row, int state) {
  if (row < 8) {
    gfxClearRect(11 + col * 8, row + 2, 7, 1);
    InputCode* slot = getKeySlot(row, col);
    if (slot) {
      int isEmpty = (slot->deviceType == inputNone);
      if (state == stateFocus) {
        gfxSetFgColor(appSettings.colorScheme.textValue);
      } else if (isEmpty) {
        gfxSetFgColor(appSettings.colorScheme.textEmpty);
      } else {
        gfxSetFgColor(appSettings.colorScheme.textDefault);
      }
      const char* keyName = inputGetKeyName(*slot);
      char trimmed[8];
      strncpy(trimmed, keyName, 7);
      trimmed[7] = '\0';
      gfxPrint(11 + col * 8, row + 2, trimmed);
    }
  } else if (row == 8) {
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    gfxPrint(0, 11, "Done");
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
    } else if (action == editClear && col > 0) {
      InputCode* slot = getKeySlot(row, col);
      if (slot) {
        slot->deviceType = inputNone;
        slot->code = 0;
      }
      return 1;
    }
  } else if (row == 8 && action == editTap) {
    settingsSave();
    screenSetup(&screenSettings, 0);
    return 1;
  }

  return 0;
}

static ScreenData screenKeyMappingData = {
  .rows = 9,
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

static void applyPendingCapture(void) {
  InputCode* slot = getKeySlot(captureRow, captureCol);
  if (slot && pendingCapture.deviceType != inputNone) {
    InputCode previous = *slot;

    // Find conflicting slot and swap
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 3; col++) {
        if (row == captureRow && col == captureCol) continue;
        InputCode* other = getKeySlot(row, col);
        if (other && other->deviceType == pendingCapture.deviceType && other->code == pendingCapture.code) {
          *other = previous;
        }
      }
    }

    *slot = pendingCapture;
  }
  pendingCapture = (InputCode){inputNone, 0};
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  LOGD("KeyMapping onInput: isKeyDown=%d, keys=%d, tapCount=%d, captureState=%d", isKeyDown, keys, tapCount, captureState);
  if (captureState == STATE_CAPTURING) return 1;
  if (captureState == STATE_CAPTURE_DONE) {
    if (!isKeyDown) {
      applyPendingCapture();
      captureState = STATE_NAVIGATION;
      fullRedraw();
    }
    return 1;
  }

  // 5 taps on any unmapped key resets to defaults
  if (isKeyDown && keys == keyUnmapped && tapCount >= 5) {
    inputInitDefaultKeyMapping();
    fullRedraw();
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
