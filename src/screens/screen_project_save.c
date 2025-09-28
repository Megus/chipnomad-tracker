#include <screens.h>
#include <corelib_gfx.h>
#include <file_browser.h>
#include <project.h>
#include <string.h>
#include <corelib_file.h>

static char pendingPath[2048];
static char pendingFolderPath[2048];

static void doSave(void) {
  if (projectSave(pendingPath) == 0) {
    // Save the directory path
    strncpy(appSettings.projectPath, pendingFolderPath, PATH_LENGTH);
    appSettings.projectPath[PATH_LENGTH] = 0;
    
    screenSetup(&screenProject, 0);
  }
}

static void cancelSave(void) {
  screenSetup(&screenProjectSave, 0);
}

static void onFolderSelected(const char* folderPath) {
  snprintf(pendingPath, sizeof(pendingPath), "%s/%s.cnm", folderPath, appSettings.projectFilename);
  strncpy(pendingFolderPath, folderPath, sizeof(pendingFolderPath) - 1);
  pendingFolderPath[sizeof(pendingFolderPath) - 1] = 0;
  
  // Check if file exists
  int fileId = fileOpen(pendingPath, 0);
  if (fileId != -1) {
    // File exists, ask for confirmation
    fileClose(fileId);
    confirmSetup("Overwrite existing file?", doSave, cancelSave);
    screenSetup(&screenConfirm, 0);
  } else {
    // File doesn't exist, save directly
    doSave();
  }
}

static void onCancelled(void) {
  screenSetup(&screenProject, 0);
}

static void setup(int input) {
  fileBrowserSetupFolderMode("SAVE PROJECT", onFolderSelected, onCancelled);
  
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

const struct AppScreen screenProjectSave = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};