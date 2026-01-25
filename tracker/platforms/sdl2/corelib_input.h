#ifndef __CORELIB_INPUT_H__
#define __CORELIB_INPUT_H__

#include <stdint.h>

// Initialize default key mappings based on platform/keyboard layout
void inputInitDefaultKeyMapping(void);

// Convert input code to human-readable name
const char* inputGetKeyName(int32_t keyCode);

// Callback for raw input capture (used by key mapping screen)
// Set to NULL when not capturing
extern void (*inputRawCallback)(int32_t keyCode, int isDown);

#endif
