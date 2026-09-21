#include <stdio.h>
#include "input_utils_sdl12.h"
#include "keymap.h"
#include "app_settings.h"

void InputUtilsSDL12::initDefaultKeyMapping(AppSettings& settings) {
  settings.keyMapping.keyUp[0] = (InputCode){InputDeviceType::keyboard, BTN_UP};
  settings.keyMapping.keyDown[0] = (InputCode){InputDeviceType::keyboard, BTN_DOWN};
  settings.keyMapping.keyLeft[0] = (InputCode){InputDeviceType::keyboard, BTN_LEFT};
  settings.keyMapping.keyRight[0] = (InputCode){InputDeviceType::keyboard, BTN_RIGHT};
  settings.keyMapping.keyEdit[0] = (InputCode){InputDeviceType::keyboard, BTN_A};
  settings.keyMapping.keyOpt[0] = (InputCode){InputDeviceType::keyboard, BTN_B};
  settings.keyMapping.keyPlay[0] = (InputCode){InputDeviceType::keyboard, BTN_START};
  settings.keyMapping.keyShift[0] = (InputCode){InputDeviceType::keyboard, BTN_SELECT};

  // Alternate mappings
  settings.keyMapping.keyOpt[1] = (InputCode){InputDeviceType::keyboard, BTN_Y};
  settings.keyMapping.keyShift[1] = (InputCode){InputDeviceType::keyboard, BTN_R1};

  // Clear remaining slots
  settings.keyMapping.keyUp[1] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyDown[1] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyLeft[1] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyRight[1] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyEdit[1] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyPlay[1] = (InputCode){InputDeviceType::none, 0};

  settings.keyMapping.keyUp[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyDown[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyLeft[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyRight[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyEdit[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyOpt[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyPlay[2] = (InputCode){InputDeviceType::none, 0};
  settings.keyMapping.keyShift[2] = (InputCode){InputDeviceType::none, 0};
}

const char* InputUtilsSDL12::getKeyName(InputCode input) {
  if (input.deviceType == InputDeviceType::none) return "---";

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
