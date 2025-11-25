#ifndef __SCREEN_CREATE_FOLDER_H__
#define __SCREEN_CREATE_FOLDER_H__

void createFolderSetup(const char* path, void (*createdCallback)(void), void (*cancelCallback)(void));

#endif