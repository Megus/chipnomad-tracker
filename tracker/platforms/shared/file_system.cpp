#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "file_system.h"
#include "misc.h"

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

bool FileSystem::fileExists(const char* path) {
  FILE* file = fopen(path, "r");
  if (file != NULL) {
    fclose(file);
    return true;
  } else {
    return false;
  }
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

std::vector<FileEntry> FileSystem::listDirectory(const char* path, const char* extension) {
  DIR* dir = opendir(path);
  if (!dir) {
    return std::vector<FileEntry>();
  }

  struct dirent* entry;
  std::vector<FileEntry> entries;

  // Check first entry - if it's neither "." nor "..", insert ".." (unless at root)
  entry = readdir(dir);
  if (entry && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(path, "/") != 0) {
    entries.push_back({.name = "..", .isDirectory = true});
  }

  // Process entries using do-while to handle first entry
  if (entry) {
    do {
      // Skip hidden files and current directory
      if (entry->d_name[0] == '.') {
        // Allow ".." for parent directory
        if (strcmp(entry->d_name, "..") != 0) continue;
      }

      char fullPath[kPathMaxLength + 1];
      snprintf(fullPath, sizeof(fullPath), "%s%s%s", path, kPathSeparatorStr, entry->d_name);

      struct stat statBuf;
      if (stat(fullPath, &statBuf) != 0) continue;

      int isDir = S_ISDIR(statBuf.st_mode);

      // Filter by extensions (comma-separated list of extensions)
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

      FileEntry fileEntry;
      copyString(fileEntry.name, entry->d_name, kFilenameMaxLength);
      fileEntry.isDirectory = isDir;
      entries.push_back(fileEntry);
    } while ((entry = readdir(dir)));
  }

  closedir(dir);
  return entries;
}

const char* FileSystem::getAutosavePath(void) {
  static const char* autosaveFilename = "autosave.cnm";
  static char autosavePath[kPathMaxLength + 1];

  char defaultDir[kPathMaxLength + 1];
  if (!getDefaultDirectory(defaultDir, kPathMaxLength + 1)) return autosaveFilename;
  snprintf(autosavePath, sizeof(autosavePath), "%s%s%s", defaultDir, kPathSeparatorStr, autosaveFilename);
  return autosavePath;
}

const char* FileSystem::getSettingsPath(void) {
  static const char* settingsFilename = "settings.txt";
  static char settingsPath[kPathMaxLength + 1];

  char defaultDir[kPathMaxLength + 1];
  if (!getDefaultDirectory(defaultDir, kPathMaxLength + 1)) return settingsFilename;
  snprintf(settingsPath, sizeof(settingsPath), "%s%s%s", defaultDir, kPathSeparatorStr, settingsFilename);
  return settingsPath;
}

void FileSystem::extractFilenameWithoutExtension(const char* path, char* output, int maxLength) {
  // Extract filename from path
  const char* filename = strrchr(path, kPathSeparatorChar);
  if (filename) {
    filename++; // Skip the separator
  } else {
    filename = path; // No path separator found
  }

  // Copy filename
  copyString(output, filename, maxLength);

  // Remove file extension
  char* dot = strrchr(output, '.');
  if (dot) {
    *dot = 0;
  }
}
