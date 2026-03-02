#ifndef __CORELIB_INPUT_H__
#define __CORELIB_INPUT_H__

#include <stdint.h>

enum InputDeviceType {
  inputNone = 0,
  inputKeyboard = 1,
  inputGamepad = 2,
};

typedef struct {
  enum InputDeviceType deviceType;
  int32_t code;
} InputCode;

// Initialize default key mappings based on platform/keyboard layout
void inputInitDefaultKeyMapping(void);

// Convert input code to human-readable name
const char* inputGetKeyName(InputCode input);

// Callback for raw input capture (used by key mapping screen)
// Set to NULL when not capturing
extern void (*inputRawCallback)(InputCode input, int isDown);

#endif
