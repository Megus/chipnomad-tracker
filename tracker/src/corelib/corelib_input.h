#ifndef __CORELIB_INPUT_H__
#define __CORELIB_INPUT_H__

#include <stdint.h>

enum class InputDeviceType {
  none = 0,
  logical = 1,
  keyboard = 2,
  gamepad = 3,
};

struct InputCode {
  InputDeviceType deviceType;
  int32_t code;
};

enum Key {
  keyLeft = 0x1,
  keyRight = 0x2,
  keyUp = 0x4,
  keyDown = 0x8,
  keyEdit = 0x10,
  keyOpt = 0x20,
  keyPlay = 0x40,
  keyShift = 0x80,
  keyUnmapped = 0x400,
};

// Initialize default key mappings based on platform/keyboard layout
void inputInitDefaultKeyMapping(void);

// Convert input code to human-readable name
const char* inputGetKeyName(InputCode input);

#endif
