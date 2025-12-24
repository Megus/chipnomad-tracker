#ifndef __FILE_BROWSER_H__
#define __FILE_BROWSER_H__

// Setup file browser with extension filter and callbacks
void fileBrowserSetup(const char* title, const char* extension, const char* startPath, void (*fileCallback)(const char*), void (*cancelCallback)(void));
void fileBrowserSetupFolderMode(const char* title, const char* startPath, const char* filename, const char* extension, void (*folderCallback)(const char*), void (*cancelCallback)(void));

// Refresh the current directory listing
void fileBrowserRefresh(void);
void fileBrowserSetPath(const char* path);

// Draw the file browser
void fileBrowserDraw(void);

// Update file browser state (call every frame)
void fileBrowserUpdate(void);

// Handle input, returns 1 if handled
int fileBrowserInput(int keys, int isDoubleTap);

#endif