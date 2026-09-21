#ifndef __INPUT_UTILS_H__
#define __INPUT_UTILS_H__

#include <stdint.h>

struct AppSettings;

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

// Input utilities
class InputUtils {
  public:
    virtual ~InputUtils() = default;

    // Initialize default key mappings into the provided settings, based on platform/keyboard layout
    virtual void initDefaultKeyMapping(AppSettings& settings) = 0;

    // Convert an input code to a human-readable name
    virtual const char* getKeyName(InputCode input) = 0;
};

#endif // __INPUT_UTILS_H__
