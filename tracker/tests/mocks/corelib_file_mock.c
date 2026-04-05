#include "corelib_file.h"
#include <stdarg.h>

int fileOpen(const char* path, int isWriting) { return -1; }
int fileClose(int fileId) { return 0; }
int fileRead(int fileId, void* buffer, int maxLength) { return 0; }
char* fileReadString(int fileId) { return NULL; }
int fileWrite(int fileId, void* data, int length) { return 0; }
int filePrintf(int fileId, const char* format, ...) { return 0; }
int fileSeek(int fileId, long offset, int whence) { return 0; }
int fileDelete(const char* path) { return 0; }
int fileCreateDirectory(const char* path) { return 0; }
FileEntry* fileListDirectory(const char* path, const char* extension, int* entryCount) {
  *entryCount = 0;
  return NULL;
}
int fileGetDefaultDirectory(char* buffer, int bufferSize) { return 0; }
int fileDirectoryExists(const char* path) { return 0; }
