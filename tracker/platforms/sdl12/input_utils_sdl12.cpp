#include <stdio.h>
#include "input_utils_sdl12.h"
#include "keymap.h"
#include "app_settings.h"

void InputUtilsSDL12::initDefaultKeyMapping(KeyMapping& mapping) {
  mapping.keyUp[0] = (InputCode){InputDeviceType::keyboard, BTN_UP};
  mapping.keyDown[0] = (InputCode){InputDeviceType::keyboard, BTN_DOWN};
  mapping.keyLeft[0] = (InputCode){InputDeviceType::keyboard, BTN_LEFT};
  mapping.keyRight[0] = (InputCode){InputDeviceType::keyboard, BTN_RIGHT};
  mapping.keyEdit[0] = (InputCode){InputDeviceType::keyboard, BTN_A};
  mapping.keyOpt[0] = (InputCode){InputDeviceType::keyboard, BTN_B};
  mapping.keyPlay[0] = (InputCode){InputDeviceType::keyboard, BTN_START};
  mapping.keyShift[0] = (InputCode){InputDeviceType::keyboard, BTN_SELECT};

  // Alternate mappings
  mapping.keyOpt[1] = (InputCode){InputDeviceType::keyboard, BTN_Y};
  mapping.keyShift[1] = (InputCode){InputDeviceType::keyboard, BTN_R1};

  // Clear remaining slots
  mapping.keyUp[1] = (InputCode){InputDeviceType::none, 0};
  mapping.keyDown[1] = (InputCode){InputDeviceType::none, 0};
  mapping.keyLeft[1] = (InputCode){InputDeviceType::none, 0};
  mapping.keyRight[1] = (InputCode){InputDeviceType::none, 0};
  mapping.keyEdit[1] = (InputCode){InputDeviceType::none, 0};
  mapping.keyPlay[1] = (InputCode){InputDeviceType::none, 0};

  mapping.keyUp[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyDown[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyLeft[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyRight[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyEdit[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyOpt[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyPlay[2] = (InputCode){InputDeviceType::none, 0};
  mapping.keyShift[2] = (InputCode){InputDeviceType::none, 0};
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
