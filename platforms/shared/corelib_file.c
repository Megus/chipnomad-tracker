#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <corelib_file.h>

static FILE* files[CORELIB_MAX_OPEN_FILES];
static int currentFileId = 0;
static char stringBuffer[1024];

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

char* fileReadString(int fileId) {
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

struct FileEntry* fileListDirectory(const char* path, const char* extension, int* entryCount) {
  DIR* dir = opendir(path);
  if (!dir) {
    *entryCount = 0;
    return NULL;
  }
  
  struct dirent* entry;
  int capacity = 100;
  int count = 0;
  struct FileEntry* entries = malloc(capacity * sizeof(struct FileEntry));
  
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
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
    
    struct stat statBuf;
    if (stat(fullPath, &statBuf) != 0) continue;
    
    int isDir = S_ISDIR(statBuf.st_mode);
    
    // If it's a file, check extension
    if (!isDir && extension) {
      char* dot = strrchr(entry->d_name, '.');
      if (!dot || strcmp(dot, extension) != 0) continue;
    }
    
    // Resize array if needed
    if (count >= capacity) {
      capacity *= 2;
      struct FileEntry* newEntries = realloc(entries, capacity * sizeof(struct FileEntry));
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

int fileGetCurrentDirectory(char* buffer, int bufferSize) {
  return getcwd(buffer, bufferSize) ? 0 : -1;
}
