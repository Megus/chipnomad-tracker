#ifndef __SCREEN_CREATE_FOLDER_H__
#define __SCREEN_CREATE_FOLDER_H__

#ifdef __cplusplus
extern "C" {
#endif

void createFolderSetup(const char* path, void (*createdCallback)(void), void (*cancelCallback)(void));


#ifdef __cplusplus
}
#endif

#endif
