#include "screens.h"
#include <string.h>
#include "corelib_gfx.h"

static char confirmMessage[128];
static int selectedOption = 0; // 0 = OK, 1 = Cancel
static void (*onConfirm)(void);
static void (*onCancel)(void);

void confirmSetup(const char* message, void (*confirmCallback)(void), void (*cancelCallback)(void)) {
  strncpy(confirmMessage, message, 127);
  confirmMessage[127] = 0;
  selectedOption = 0;
  onConfirm = confirmCallback;
  onCancel = cancelCallback;
}

static void setup(int input) {
}

static void fullRedraw(void) {
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  gfxSetFgColor(appSettings.colorScheme.selection);
  gfxPrint(0, 8, confirmMessage);

  // Draw OK button
  if (selectedOption == 0) {
    gfxSetFgColor(appSettings.colorScheme.textValue);
    gfxPrint(0, 10, "> OK");
  } else {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(0, 10, "  OK");
  }

  // Draw Cancel button
  if (selectedOption == 1) {
    gfxSetFgColor(appSettings.colorScheme.textValue);
    gfxPrint(6, 10, "> Cancel");
  } else {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(6, 10, "  Cancel");
  }
}

static void draw(void) {
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (keys == keyLeft && selectedOption > 0) {
    selectedOption--;
    fullRedraw();
    return 1;
  } else if (keys == keyRight && selectedOption < 1) {
    selectedOption++;
    fullRedraw();
    return 1;
  } else if (keys == keyEdit) {
    if (selectedOption == 0 && onConfirm) {
      onConfirm();
    } else if (selectedOption == 1 && onCancel) {
      onCancel();
    }
    return 1;
  } else if (keys == keyOpt && onCancel) {
    onCancel();
    return 1;
  }
  return 0;
}

const AppScreen screenConfirm = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
