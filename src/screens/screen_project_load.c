#include <screens.h>
#include <corelib_gfx.h>
#include <file_browser.h>
#include <project.h>
#include <string.h>

static void onFileSelected(const char* path) {
  if (projectLoad(path) == 0) {
    extractFilenameWithoutExtension(path, appSettings.projectFilename, FILENAME_LENGTH + 1);
    
    // Save the directory path
    char* lastSlash = strrchr(path, '/');
    if (lastSlash) {
      int pathLen = lastSlash - path;
      strncpy(appSettings.projectPath, path, pathLen);
      appSettings.projectPath[pathLen] = 0;
    }
    
    screenSetup(&screenProject, 0);
  }
}

static void onCancelled(void) {
  screenSetup(&screenProject, 0);
}

static void setup(int input) {
  fileBrowserSetup("LOAD PROJECT", ".cnm", onFileSelected, onCancelled);
  
  // Set starting directory if projectPath is set
  if (strlen(appSettings.projectPath) > 0) {
    fileBrowserSetPath(appSettings.projectPath);
  }
}

static void fullRedraw(void) {
  fileBrowserDraw();
}

static void draw(void) {
}

static void onInput(int keys, int isDoubleTap) {
  if (fileBrowserInput(keys, isDoubleTap)) {
    fileBrowserDraw();
  }
}

const struct AppScreen screenProjectLoad = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};