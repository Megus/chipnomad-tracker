#include <screens.h>
#include <corelib_file.h>
#include <corelib_gfx.h>
#include <string.h>
#include <stdlib.h>

#define VISIBLE_ENTRIES 16

static struct FileEntry* entries = NULL;
static int entryCount = 0;
static int selectedIndex = 0;
static int topIndex = 0;
static char currentPath[2048];
static char fileExtension[8];
static char browserTitle[32];
static char saveFilename[256];
static char saveExtension[8];
static int isFolderMode = 0;
static void (*onFileSelected)(const char* path);
static void (*onCancelled)(void);
static char pendingSavePath[2048];

static void doSave(void) {
  if (onFileSelected) {
    onFileSelected(pendingSavePath);
  }
}

static void cancelSave(void) {
  screenSetup(&screenFileBrowser, 0);
}

static int compareEntries(const void* a, const void* b) {
  const struct FileEntry* entryA = (const struct FileEntry*)a;
  const struct FileEntry* entryB = (const struct FileEntry*)b;

  // Directories first
  if (entryA->isDirectory && !entryB->isDirectory) return -1;
  if (!entryA->isDirectory && entryB->isDirectory) return 1;

  // Then alphabetical
  return strcmp(entryA->name, entryB->name);
}

static void fileBrowserRefreshWithSelection(const char* selectName) {
  if (entries) {
    free(entries);
    entries = NULL;
  }

  entries = fileListDirectory(currentPath, fileExtension, &entryCount);
  if (entries && entryCount > 0) {
    qsort(entries, entryCount, sizeof(struct FileEntry), compareEntries);
  }

  selectedIndex = 0;
  topIndex = 0;

  // Find the specified entry and select it
  if (selectName) {
    for (int i = 0; i < entryCount; i++) {
      if (strcmp(entries[i].name, selectName) == 0) {
        selectedIndex = i;
        if (selectedIndex >= VISIBLE_ENTRIES) {
          topIndex = selectedIndex - VISIBLE_ENTRIES + 1;
        }
        break;
      }
    }
  }
}

void fileBrowserRefresh(void) {
  fileBrowserRefreshWithSelection(NULL);
}

void fileBrowserSetup(const char* title, const char* extension, const char* startPath, void (*fileCallback)(const char*), void (*cancelCallback)(void)) {
  strncpy(browserTitle, title, 31);
  browserTitle[31] = 0;
  strncpy(fileExtension, extension, 7);
  fileExtension[7] = 0;
  isFolderMode = 0;
  onFileSelected = fileCallback;
  onCancelled = cancelCallback;

  if (startPath && strlen(startPath) > 0 && fileDirectoryExists(startPath)) {
    strncpy(currentPath, startPath, sizeof(currentPath) - 1);
    currentPath[sizeof(currentPath) - 1] = 0;
  } else {
    fileGetCurrentDirectory(currentPath, sizeof(currentPath));
  }
  fileBrowserRefresh();
}

void fileBrowserSetupFolderMode(const char* title, const char* startPath, const char* filename, const char* extension, void (*folderCallback)(const char*), void (*cancelCallback)(void)) {
  strncpy(browserTitle, title, 31);
  browserTitle[31] = 0;
  strncpy(saveFilename, filename ? filename : "", sizeof(saveFilename) - 1);
  saveFilename[sizeof(saveFilename) - 1] = 0;
  strncpy(saveExtension, extension ? extension : "", sizeof(saveExtension) - 1);
  saveExtension[sizeof(saveExtension) - 1] = 0;
  fileExtension[0] = 0;
  isFolderMode = 1;
  onFileSelected = folderCallback;
  onCancelled = cancelCallback;

  if (startPath && strlen(startPath) > 0 && fileDirectoryExists(startPath)) {
    strncpy(currentPath, startPath, sizeof(currentPath) - 1);
    currentPath[sizeof(currentPath) - 1] = 0;
  } else {
    fileGetCurrentDirectory(currentPath, sizeof(currentPath));
  }
  fileBrowserRefresh();
}

void fileBrowserDraw(void) {
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, browserTitle);

  gfxSetFgColor(appSettings.colorScheme.textInfo);
  int pathLen = strlen(currentPath);
  if (pathLen <= 80) {
    gfxPrint(0, 1, currentPath);
  } else {
    char displayPath[84];
    snprintf(displayPath, sizeof(displayPath), "...%s", currentPath + pathLen - 77);
    gfxPrint(0, 1, displayPath);
  }

  // Draw file list
  int startIdx = 0;
  int displayOffset = 0;
  
  // In folder mode, add "Save to [folder]" as first entry
  if (isFolderMode) {
    if (topIndex == 0) {
      int y = 3;
      if (selectedIndex == 0) {
        gfxSetFgColor(appSettings.colorScheme.textValue);
        gfxPrint(0, y, ">");
      } else {
        gfxSetFgColor(appSettings.colorScheme.textDefault);
        gfxPrint(0, y, " ");
      }
      
      char saveText[35];
      char* folderName = strrchr(currentPath, '/');
      if (folderName) {
        folderName++; // Skip the '/'
        if (*folderName == 0) folderName = "/"; // Root directory case
      } else {
        folderName = currentPath;
      }
      
      int nameLen = strlen(folderName);
      if (nameLen <= 25) {
        snprintf(saveText, sizeof(saveText), "Save to [%s]", folderName);
      } else {
        snprintf(saveText, sizeof(saveText), "Save to [...%.19s]", folderName + nameLen - 19);
      }
      saveText[34] = 0; // Ensure null termination
      gfxPrint(2, y, saveText);
      displayOffset = 1;
    } else {
      startIdx = 1;
    }
  }
  
  for (int i = startIdx; i < VISIBLE_ENTRIES - displayOffset && (topIndex + i - displayOffset) < entryCount; i++) {
    int entryIdx = topIndex + i - displayOffset;
    int y = 3 + i;

    if (entryIdx + displayOffset == selectedIndex) {
      gfxSetFgColor(appSettings.colorScheme.textValue);
      gfxPrint(0, y, ">");
    } else {
      gfxSetFgColor(appSettings.colorScheme.textDefault);
      gfxPrint(0, y, " ");
    }

    // Set color based on mode and entry type
    if (isFolderMode) {
      if (entries[entryIdx].isDirectory) {
        gfxSetFgColor(appSettings.colorScheme.textDefault);
      } else {
        gfxSetFgColor(appSettings.colorScheme.textInfo);
      }
    } else {
      gfxSetFgColor(appSettings.colorScheme.textDefault);
    }

    char displayName[35];
    if (entries[entryIdx].isDirectory) {
      snprintf(displayName, sizeof(displayName), "[%.31s]", entries[entryIdx].name);
    } else {
      snprintf(displayName, sizeof(displayName), "%.34s", entries[entryIdx].name);
    }
    gfxPrint(2, y, displayName);
  }


}

int fileBrowserInput(int keys, int isDoubleTap) {
  int maxIndex = entryCount - 1;
  if (isFolderMode) maxIndex++; // Add 1 for "Save to" option
  
  if (keys == keyUp && selectedIndex > 0) {
    selectedIndex--;
    if (selectedIndex < topIndex) {
      topIndex = selectedIndex;
    }
    return 1;
  } else if (keys == keyDown && selectedIndex < maxIndex) {
    selectedIndex++;
    if (selectedIndex >= topIndex + VISIBLE_ENTRIES) {
      topIndex = selectedIndex - VISIBLE_ENTRIES + 1;
    }
    return 1;
  } else if (keys == keyLeft) {
    // Page up
    selectedIndex -= VISIBLE_ENTRIES;
    if (selectedIndex < 0) selectedIndex = 0;
    if (selectedIndex < topIndex) {
      topIndex = selectedIndex;
    }
    return 1;
  } else if (keys == keyRight) {
    // Page down
    selectedIndex += VISIBLE_ENTRIES;
    if (selectedIndex >= entryCount) selectedIndex = entryCount - 1;
    if (selectedIndex >= topIndex + VISIBLE_ENTRIES) {
      topIndex = selectedIndex - VISIBLE_ENTRIES + 1;
    }
    return 1;
  } else if (keys == keyEdit) {
    // Handle "Save to" option in folder mode
    if (isFolderMode && selectedIndex == 0) {
      if (onFileSelected) {
        // Construct full path for overwrite check
        snprintf(pendingSavePath, sizeof(pendingSavePath), "%s/%s%s", currentPath, saveFilename, saveExtension);
        
        // Check if file exists
        int fileId = fileOpen(pendingSavePath, 0);
        if (fileId != -1) {
          // File exists, ask for confirmation
          fileClose(fileId);
          confirmSetup("Overwrite existing file?", doSave, cancelSave);
          screenSetup(&screenConfirm, 0);
        } else {
          // File doesn't exist, save directly
          onFileSelected(currentPath);
        }
        return 0;
      }
    }
    
    int entryIdx = isFolderMode ? selectedIndex - 1 : selectedIndex;
    if (entryIdx >= 0 && entries[entryIdx].isDirectory) {
      // Enter directory
      if (strcmp(entries[entryIdx].name, "..") == 0) {
        // Go up one level - remember current folder name
        char* lastSlash = strrchr(currentPath, '/');
        if (lastSlash && lastSlash != currentPath) {
          char folderName[256];
          strncpy(folderName, lastSlash + 1, 255);
          folderName[255] = 0;
          *lastSlash = 0;
          fileBrowserRefreshWithSelection(folderName);
          return 1;
        } else if (strlen(currentPath) > 1) {
          // Go to root
          strcpy(currentPath, "/");
          fileBrowserRefresh();
          return 1;
        }
      } else {
        // Enter subdirectory
        int len = strlen(currentPath);
        if (len > 0 && currentPath[len-1] != '/') {
          strcat(currentPath, "/");
        }
        strcat(currentPath, entries[entryIdx].name);
      }
      fileBrowserRefresh();
    } else if (!isFolderMode && entryIdx >= 0) {
      // Select file (only in file mode)
      char fullPath[2048];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, entries[entryIdx].name);
      if (onFileSelected) {
        onFileSelected(fullPath);
        return 0;
      }
    }
    return 1;
  } else if (keys == keyOpt) {
    // Cancel
    if (onCancelled) {
      onCancelled();
      return 0;
    }
    return 1;
  }

  return 0;
}

void fileBrowserSetPath(const char* path) {
  strncpy(currentPath, path, sizeof(currentPath) - 1);
  currentPath[sizeof(currentPath) - 1] = 0;
  fileBrowserRefresh();
}