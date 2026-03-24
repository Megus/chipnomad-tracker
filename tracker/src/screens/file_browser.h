#ifndef __FILE_BROWSER_H__
#define __FILE_BROWSER_H__

// Setup file browser with extension filter and callbacks
void fileBrowserSetup(const char* title, const char* extension, const char* startPath, void (*fileCallback)(const char*), void (*cancelCallback)(void));
void fileBrowserSetupFolderMode(const char* title, const char* startPath, const char* filename, const char* extension, void (*folderCallback)(const char*), void (*cancelCallback)(void));

void fileBrowserSetPath(const char* path);

#endif
