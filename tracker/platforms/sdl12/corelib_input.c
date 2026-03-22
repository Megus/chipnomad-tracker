#include "corelib_input.h"
#include "common.h"
#include <stdio.h>

// RG35xx SDL1.2 keycodes
#define RG_UP     119
#define RG_DOWN   115
#define RG_LEFT   113
#define RG_RIGHT  100
#define RG_A      97
#define RG_B      98
#define RG_Y      121
#define RG_L1     104
#define RG_R1     108
#define RG_SELECT 110
#define RG_START  109
#define RG_VOL_UP 114
#define RG_VOL_DN 116

void inputInitDefaultKeyMapping(void) {
  appSettings.keyMapping.keyUp[0] = (InputCode){inputKeyboard, RG_UP};
  appSettings.keyMapping.keyDown[0] = (InputCode){inputKeyboard, RG_DOWN};
  appSettings.keyMapping.keyLeft[0] = (InputCode){inputKeyboard, RG_LEFT};
  appSettings.keyMapping.keyRight[0] = (InputCode){inputKeyboard, RG_RIGHT};
  appSettings.keyMapping.keyEdit[0] = (InputCode){inputKeyboard, RG_A};
  appSettings.keyMapping.keyOpt[0] = (InputCode){inputKeyboard, RG_B};
  appSettings.keyMapping.keyPlay[0] = (InputCode){inputKeyboard, RG_START};
  appSettings.keyMapping.keyShift[0] = (InputCode){inputKeyboard, RG_SELECT};

  // Alternate mappings
  appSettings.keyMapping.keyOpt[1] = (InputCode){inputKeyboard, RG_Y};
  appSettings.keyMapping.keyShift[1] = (InputCode){inputKeyboard, RG_R1};

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
    case RG_UP: return "Up";
    case RG_DOWN: return "Down";
    case RG_LEFT: return "Left";
    case RG_RIGHT: return "Right";
    case RG_A: return "A";
    case RG_B: return "B";
    case RG_Y: return "Y";
    case RG_L1: return "L1";
    case RG_R1: return "R1";
    case RG_SELECT: return "Select";
    case RG_START: return "Start";
    case RG_VOL_UP: return "Vol+";
    case RG_VOL_DN: return "Vol-";
    default:
      snprintf(buf, sizeof(buf), "K%d", input.code);
      return buf;
  }
}
