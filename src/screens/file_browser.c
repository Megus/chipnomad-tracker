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
static void (*onFileSelected)(const char* path);
static void (*onCancelled)(void);

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

void fileBrowserSetup(const char* title, const char* extension, void (*fileCallback)(const char*), void (*cancelCallback)(void)) {
  strncpy(browserTitle, title, 31);
  browserTitle[31] = 0;
  strncpy(fileExtension, extension, 7);
  fileExtension[7] = 0;
  onFileSelected = fileCallback;
  onCancelled = cancelCallback;
  
  fileGetCurrentDirectory(currentPath, sizeof(currentPath));
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
  for (int i = 0; i < VISIBLE_ENTRIES && (topIndex + i) < entryCount; i++) {
    int entryIdx = topIndex + i;
    int y = 3 + i;
    
    if (entryIdx == selectedIndex) {
      gfxSetFgColor(appSettings.colorScheme.textValue);
      gfxPrint(0, y, ">");
    } else {
      gfxSetFgColor(appSettings.colorScheme.textDefault);
      gfxPrint(0, y, " ");
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
  if (keys == keyUp && selectedIndex > 0) {
    selectedIndex--;
    if (selectedIndex < topIndex) {
      topIndex = selectedIndex;
    }
    return 1;
  } else if (keys == keyDown && selectedIndex < entryCount - 1) {
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
    if (entries[selectedIndex].isDirectory) {
      // Enter directory
      if (strcmp(entries[selectedIndex].name, "..") == 0) {
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
        strcat(currentPath, entries[selectedIndex].name);
      }
      fileBrowserRefresh();
    } else {
      // Select file
      char fullPath[2048];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, entries[selectedIndex].name);
      if (onFileSelected) {
        onFileSelected(fullPath);
      }
    }
    return 1;
  } else if (keys == keyOpt) {
    // Cancel
    if (onCancelled) {
      onCancelled();
    }
    return 1;
  }
  
  return 0;
}