#ifndef __FILE_BROWSER_H__
#define __FILE_BROWSER_H__

// Setup file browser with extension filter and callbacks
void fileBrowserSetup(const char* title, const char* extension, void (*fileCallback)(const char*), void (*cancelCallback)(void));

// Refresh the current directory listing
void fileBrowserRefresh(void);

// Draw the file browser
void fileBrowserDraw(void);

// Handle input, returns 1 if handled
int fileBrowserInput(int keys, int isDoubleTap);

#endif