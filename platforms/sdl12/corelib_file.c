#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <corelib_file.h>

static FILE* files[CORELIB_MAX_OPEN_FILES];
static int currentFileId = 0;

int fileOpen(const char* path, int isWriting) {
  int fileId = currentFileId;

  FILE* file = fopen(path, isWriting ? "wb" : "rb");
  if (file == NULL) {
    // TODO: Return error details?
    return -1;
  }
  files[fileId] = file;

  currentFileId++;
  if (currentFileId == CORELIB_MAX_OPEN_FILES) {
    currentFileId = 0;
  }

  return fileId;
}

int fileClose(int fileId) {
  fclose(files[fileId]);
  return 0;
}

int fileRead(int fileId, void* buffer, int maxLength) {
  return fread(buffer, 1, maxLength, files[fileId]);
}

int fileWrite(int fileId, void* data, int length) {
  return fwrite(data, 1, length, files[fileId]);
}

int filePrintf(int fileId, const char* format, ...) {
  static char writeBuffer[1024];

  va_list args;
  va_start(args, format);
  vsnprintf(writeBuffer, 1024, format, args);
  va_end(args);

  return fileWrite(fileId, writeBuffer, strlen(writeBuffer));
}
