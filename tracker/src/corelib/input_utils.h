#ifndef __INPUT_UTILS_H__
#define __INPUT_UTILS_H__

#include <stdint.h>

enum class InputDeviceType : int {
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

// Key mapping: 8 buttons x 3 keys each
struct KeyMapping {
  InputCode keyUp[3];
  InputCode keyDown[3];
  InputCode keyLeft[3];
  InputCode keyRight[3];
  InputCode keyEdit[3];
  InputCode keyOpt[3];
  InputCode keyPlay[3];
  InputCode keyShift[3];
};

// Input utilities
class InputUtils {
  public:
    virtual ~InputUtils() = default;

    // Initialize default key mappings into the provided settings, based on platform/keyboard layout
    virtual void initDefaultKeyMapping(KeyMapping& mapping) = 0;

    // Convert an input code to a human-readable name
    virtual const char* getKeyName(InputCode input) = 0;

    // Convert input code to a key following a mapping
    int inputCodeToKey(InputCode input, KeyMapping& mapping);
};

#endif // __INPUT_UTILS_H__
