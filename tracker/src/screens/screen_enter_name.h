#ifndef __SCREEN_ENTER_NAME_H__
#define __SCREEN_ENTER_NAME_H__

// Setup name entry screen with custom prompt and callbacks
// Returns trimmed name through callback
void enterNameSetup(const char* title, const char* prompt, const char* initialName, void (*confirmCallback)(const char* name), void (*cancelCallback)(void));

#endif
