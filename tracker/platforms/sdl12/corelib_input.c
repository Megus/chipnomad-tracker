#include "corelib_input.h"
#include "corelib_keymap.h"
#include "common.h"
#include <stdio.h>

void inputInitDefaultKeyMapping(void) {
  appSettings.keyMapping.keyUp[0] = (InputCode){inputKeyboard, BTN_UP};
  appSettings.keyMapping.keyDown[0] = (InputCode){inputKeyboard, BTN_DOWN};
  appSettings.keyMapping.keyLeft[0] = (InputCode){inputKeyboard, BTN_LEFT};
  appSettings.keyMapping.keyRight[0] = (InputCode){inputKeyboard, BTN_RIGHT};
  appSettings.keyMapping.keyEdit[0] = (InputCode){inputKeyboard, BTN_A};
  appSettings.keyMapping.keyOpt[0] = (InputCode){inputKeyboard, BTN_B};
  appSettings.keyMapping.keyPlay[0] = (InputCode){inputKeyboard, BTN_START};
  appSettings.keyMapping.keyShift[0] = (InputCode){inputKeyboard, BTN_SELECT};

  // Alternate mappings
  appSettings.keyMapping.keyOpt[1] = (InputCode){inputKeyboard, BTN_Y};
  appSettings.keyMapping.keyShift[1] = (InputCode){inputKeyboard, BTN_R1};

  // Clear remaining slots
  appSettings.keyMapping.keyUp[1] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyDown[1] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyLeft[1] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyRight[1] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyEdit[1] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyPlay[1] = (InputCode){inputNone, 0};

  appSettings.keyMapping.keyUp[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyDown[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyLeft[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyRight[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyEdit[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyOpt[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyPlay[2] = (InputCode){inputNone, 0};
  appSettings.keyMapping.keyShift[2] = (InputCode){inputNone, 0};
}

const char* inputGetKeyName(InputCode input) {
  if (input.deviceType == inputNone) return "---";

  static char buf[8];
  switch (input.code) {
    case BTN_UP: return "Up";
    case BTN_DOWN: return "Down";
    case BTN_LEFT: return "Left";
    case BTN_RIGHT: return "Right";
    case BTN_A: return "A";
    case BTN_B: return "B";
    case BTN_X: return "X";
    case BTN_Y: return "Y";
    case BTN_L1: return "L1";
    case BTN_R1: return "R1";
    case BTN_L2: return "L2";
    case BTN_R2: return "R2";
    case BTN_SELECT: return "Select";
    case BTN_START: return "Start";
    case BTN_MENU: return "Menu";
    default:
      snprintf(buf, sizeof(buf), "K%d", input.code);
      return buf;
  }
}
