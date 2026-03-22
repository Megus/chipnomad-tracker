#ifndef __CORELIB_FILE_H__
#define __CORELIB_FILE_H__

#define CORELIB_MAX_OPEN_FILES (16)

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

// Debug logging macro
#ifdef ANDROID_BUILD
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ChipNomad", __VA_ARGS__)
#else
#include <stdio.h>
#define LOGD(...) printf(__VA_ARGS__); printf("\n")
#endif

// File browser functions
typedef struct FileEntry {
  char name[256];
  int isDirectory;
} FileEntry;

// File operations API
int fileOpen(const char* path, int isWriting);
int fileClose(int fileId);
int fileRead(int fileId, void* buffer, int maxLength);
char* fileReadString(int fileId);
int fileWrite(int fileId, void* data, int length);
int filePrintf(int fileId, const char* format, ...);
int fileSeek(int fileId, long offset, int whence);
int fileDelete(const char* path);
int fileCreateDirectory(const char* path);
FileEntry* fileListDirectory(const char* path, const char* extension, int* entryCount);
int fileGetDefaultDirectory(char* buffer, int bufferSize);
int fileDirectoryExists(const char* path);

#endif