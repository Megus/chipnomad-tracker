#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "file_system.h"

#if defined(ANDROID_BUILD) || defined(MACOS_BUILD)
// Helper: Create directory recursively
static void createDirectoryRecursive(const char* path) {
  char tmp[4096];
  char* p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') tmp[len - 1] = 0;

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      #ifdef _WIN32
      mkdir(tmp);
      #else
      mkdir(tmp, 0755);
      #endif
      *p = '/';
    }
  }
  #ifdef _WIN32
  mkdir(tmp);
  #else
  mkdir(tmp, 0755);
  #endif
}
#endif

bool FileSystem::getDefaultDirectory(char* buffer, int bufferSize) {
#ifdef ANDROID_BUILD
  const char* dataPath = "/storage/emulated/0/Documents/ChipNomad";
  createDirectoryRecursive(dataPath);
  snprintf(buffer, bufferSize, "%s", dataPath);
  return true;
#elif defined(MACOS_BUILD)
  const char* home = getenv("HOME");
  if (home) {
    snprintf(buffer, bufferSize, "%s/Library/Application Support/ChipNomad", home);
    createDirectoryRecursive(buffer);
  } else {
    snprintf(buffer, bufferSize, ".");
  }
  return true;
#else
  return getcwd(buffer, bufferSize) ? true : false;
#endif
}

bool FileSystem::directoryExists(const char* path) {
  struct stat statBuf;
  return (stat(path, &statBuf) == 0 && S_ISDIR(statBuf.st_mode)) ? true : false;
}

bool FileSystem::createDirectory(const char* path) {
  #ifdef _WIN32
  return mkdir(path) == 0 ? true : false;
  #else
  return mkdir(path, 0755) == 0 ? true : false;
  #endif
}

bool FileSystem::deleteFile(const char* path) {
  return remove(path) == 0 ? true : false;
}

FileEntry* FileSystem::listDirectory(const char* path, const char* extension, int* entryCount) {
  DIR* dir = opendir(path);
  if (!dir) {
    *entryCount = 0;
    return nullptr;
  }

  struct dirent* entry;
  int capacity = 100;
  int count = 0;
  FileEntry* entries = (FileEntry*)malloc(capacity * sizeof(FileEntry));

  if (!entries) {
    closedir(dir);
    *entryCount = 0;
    return nullptr;
  }

  // Check first entry - if it's neither "." nor "..", insert ".." (unless at root)
  entry = readdir(dir);
  if (entry && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(path, "/") != 0) {
    strcpy(entries[0].name, "..");
    entries[0].isDirectory = 1;
    count = 1;
  }

  // Process entries using do-while to handle first entry
  if (entry) {
    do {
      // Skip hidden files and current directory
      if (entry->d_name[0] == '.') {
        // Allow ".." for parent directory
        if (strcmp(entry->d_name, "..") != 0) continue;
      }

      char fullPath[2048];
      snprintf(fullPath, sizeof(fullPath), "%s%s%s", path, kPathSeparatorStr, entry->d_name);

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
        FileEntry* newEntries = (FileEntry*)realloc(entries, capacity * sizeof(FileEntry));
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
    } while ((entry = readdir(dir)));
  }

  closedir(dir);
  *entryCount = count;
  return entries;
}

const char* FileSystem::getAutosavePath(void) {
  static const char* autosaveFilename = "autosave.cnm";

  char defaultDir[kPathMaxLength];
  char autosavePath[kPathMaxLength];
  if (!getDefaultDirectory(defaultDir, kPathMaxLength)) return autosaveFilename;
  snprintf(autosavePath, sizeof(autosavePath), "%s%s%s", defaultDir, kPathSeparatorStr, autosaveFilename);
  return autosavePath;
}

const char* FileSystem::getSettingsPath(void) {
  static const char* settingsFilename = "settings.txt";

  char defaultDir[kPathMaxLength];
  char settingsPath[kPathMaxLength];
  if (!getDefaultDirectory(defaultDir, kPathMaxLength)) return settingsFilename;
  snprintf(settingsPath, sizeof(settingsPath), "%s%s%s", defaultDir, kPathSeparatorStr, settingsFilename);
  return settingsPath;
}

void extractFilenameWithoutExtension(const char* path, char* output, int maxLength) {
  // Extract filename from path
  const char* filename = strrchr(path, kPathSeparator);
  if (filename) {
    filename++; // Skip the separator
  } else {
    filename = path; // No path separator found
  }

  // Copy filename
  strncpy(output, filename, maxLength - 1);
  output[maxLength - 1] = 0;

  // Remove file extension
  char* dot = strrchr(output, '.');
  if (dot) {
    *dot = 0;
  }
}
