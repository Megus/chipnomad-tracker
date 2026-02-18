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

// File operations struct (OOP-like interface)
typedef struct FileOps {
  int (*open)(const char* path, int isWriting);
  int (*close)(int fileId);
  int (*read)(int fileId, void* buffer, int maxLength);
  char* (*readString)(int fileId);
  int (*write)(int fileId, void* data, int length);
  int (*printf)(int fileId, const char* format, ...);
  int (*seek)(int fileId, long offset, int whence);
  int (*delete)(const char* path);
  int (*createDirectory)(const char* path);
  FileEntry* (*listDirectory)(const char* path, const char* extension, int* entryCount);
  int (*getDefaultDirectory)(char* buffer, int bufferSize);
  int (*directoryExists)(const char* path);
} FileOps;

// Global file operations instance
extern FileOps fileOps;

// Convenience macros for calling file operations
#define fileOpen(path, isWriting) fileOps.open(path, isWriting)
#define fileClose(fileId) fileOps.close(fileId)
#define fileRead(fileId, buffer, maxLength) fileOps.read(fileId, buffer, maxLength)
#define fileReadString(fileId) fileOps.readString(fileId)
#define fileWrite(fileId, data, length) fileOps.write(fileId, data, length)
#define filePrintf(fileId, ...) fileOps.printf(fileId, __VA_ARGS__)
#define fileSeek(fileId, offset, whence) fileOps.seek(fileId, offset, whence)
#define fileDelete(path) fileOps.delete(path)
#define fileCreateDirectory(path) fileOps.createDirectory(path)
#define fileListDirectory(path, extension, entryCount) fileOps.listDirectory(path, extension, entryCount)
#define fileGetDefaultDirectory(buffer, bufferSize) fileOps.getDefaultDirectory(buffer, bufferSize)
#define fileDirectoryExists(path) fileOps.directoryExists(path)

#endif