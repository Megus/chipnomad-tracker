#ifndef __KEYBOARD_LAYOUT_H__
#define __KEYBOARD_LAYOUT_H__

#include "common.h"

// Keyboard layout detection and management functions
KeyboardLayout detectKeyboardLayout(void);
const char* getKeyboardLayoutName(KeyboardLayout layout);
int getKeyForLayout(int keyCode, KeyboardLayout layout);
void initKeyboardLayout(void);
void updateKeyboardLayout(void); // Re-evaluate keyboard layout after settings change

#endif