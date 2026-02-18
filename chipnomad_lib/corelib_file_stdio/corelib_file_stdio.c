#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../corelib/corelib_file.h"

static FILE* files[CORELIB_MAX_OPEN_FILES];
static int currentFileId = 0;
static char stringBuffer[1024];

static int fileOpenStdio(const char* path, int isWriting) {
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

static int fileCloseStdio(int fileId) {
  fclose(files[fileId]);
  return 0;
}

static int fileReadStdio(int fileId, void* buffer, int maxLength) {
  return fread(buffer, 1, maxLength, files[fileId]);
}

static char* fileReadStringStdio(int fileId) {
  char* r = fgets(stringBuffer, 1024, files[fileId]);

  if (r == NULL) return NULL;

  // Trim (if needed) EOL and space characters
  int idx = strlen(stringBuffer) - 1;
  while (idx >= 0 && isspace(stringBuffer[idx])) {
    stringBuffer[idx] = 0;
    idx--;
  }

  return stringBuffer;
}

static int fileWriteStdio(int fileId, void* data, int length) {
  return fwrite(data, 1, length, files[fileId]);
}

static int filePrintfStdio(int fileId, const char* format, ...) {
  static char writeBuffer[1024];

  va_list args;
  va_start(args, format);
  vsnprintf(writeBuffer, 1024, format, args);
  va_end(args);

  return fileWriteStdio(fileId, writeBuffer, strlen(writeBuffer));
}

static int fileSeekStdio(int fileId, long offset, int whence) {
  return fseek(files[fileId], offset, whence);
}

static FileEntry* fileListDirectoryStdio(const char* path, const char* extension, int* entryCount) {
  DIR* dir = opendir(path);
  if (!dir) {
    *entryCount = 0;
    return NULL;
  }

  struct dirent* entry;
  int capacity = 100;
  int count = 0;
  FileEntry* entries = malloc(capacity * sizeof(FileEntry));

  if (!entries) {
    closedir(dir);
    *entryCount = 0;
    return NULL;
  }

  while ((entry = readdir(dir))) {
    // Skip hidden files and current directory
    if (entry->d_name[0] == '.') {
      // Allow ".." for parent directory
      if (strcmp(entry->d_name, "..") != 0) continue;
    }

    char fullPath[2048];
    snprintf(fullPath, sizeof(fullPath), "%s%s%s", path, PATH_SEPARATOR_STR, entry->d_name);

    struct stat statBuf;
    if (stat(fullPath, &statBuf) != 0) continue;

    int isDir = S_ISDIR(statBuf.st_mode);

    if (!isDir && extension && extension[0] != '\0') {
      char* dot = strrchr(entry->d_name, '.');
      if (!dot) continue;

      int match = 0;
      const char* pos = extension;
      while (pos) {
        if (strcasecmp(pos, dot) == 0 ||
        (strncasecmp(pos, dot, strlen(dot)) == 0 && pos[strlen(dot)] == ',')) {
          match = 1;
          break;
        }
        pos = strchr(pos, ',');
        if (pos) pos++;
      }
      if (!match) continue;
    }

    // Resize array if needed
    if (count >= capacity) {
      capacity *= 2;
      FileEntry* newEntries = realloc(entries, capacity * sizeof(FileEntry));
      if (!newEntries) {
        free(entries);
        closedir(dir);
        *entryCount = 0;
        return NULL;
      }
      entries = newEntries;
    }

    strncpy(entries[count].name, entry->d_name, 255);
    entries[count].name[255] = 0;
    entries[count].isDirectory = isDir;
    count++;
  }

  closedir(dir);
  *entryCount = count;
  return entries;
}

static int fileGetDefaultDirectoryStdio(char* buffer, int bufferSize) {
  return getcwd(buffer, bufferSize) ? 0 : -1;
}

static int fileDirectoryExistsStdio(const char* path) {
  struct stat statBuf;
  return (stat(path, &statBuf) == 0 && S_ISDIR(statBuf.st_mode)) ? 1 : 0;
}

static int fileDeleteStdio(const char* path) {
  return remove(path) == 0 ? 0 : -1;
}

static int fileCreateDirectoryStdio(const char* path) {
  #ifdef _WIN32
  return mkdir(path) == 0 ? 0 : -1;
  #else
  return mkdir(path, 0755) == 0 ? 0 : -1;
  #endif
}

// Global file operations instance with stdio implementations
FileOps fileOps = {
  .open = fileOpenStdio,
  .close = fileCloseStdio,
  .read = fileReadStdio,
  .readString = fileReadStringStdio,
  .write = fileWriteStdio,
  .printf = filePrintfStdio,
  .seek = fileSeekStdio,
  .delete = fileDeleteStdio,
  .createDirectory = fileCreateDirectoryStdio,
  .listDirectory = fileListDirectoryStdio,
  .getDefaultDirectory = fileGetDefaultDirectoryStdio,
  .directoryExists = fileDirectoryExistsStdio
};