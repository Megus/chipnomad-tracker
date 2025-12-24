#include "screens.h"
#include "corelib/corelib_file.h"
#include "corelib_gfx.h"
#include <string.h>

static char folderName[17] = "";
static char currentPath[2048];
static int isCharEdit = 0;
static char* editingString = NULL;
static int editingStringLength = 0;
static void (*onFolderCreated)(void);
static void (*onCancelled)(void);

static void createFolder(void);
static void fullRedraw(void);

static int getColumnCount(int row) {
  if (row == 0) return 16; // Folder name field
  if (row == 1) return 2;  // Create, Cancel buttons
  return 1;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "CREATE FOLDER");

  gfxSetFgColor(appSettings.colorScheme.textInfo);
  int pathLen = strlen(currentPath);
  if (pathLen <= 80) {
    gfxPrint(0, 1, currentPath);
  } else {
    char displayPath[84];
    snprintf(displayPath, sizeof(displayPath), "...%s", currentPath + pathLen - 77);
    gfxPrint(0, 1, displayPath);
  }

  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 3, "Folder name:");
}

static void drawCursor(int col, int row) {
  if (row == 0) {
    gfxCursor(13 + col, 3, 1);
  } else if (row == 1) {
    if (col == 0) {
      gfxCursor(0, 5, 6);
    } else {
      gfxCursor(8, 5, 6);
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row == 0) {
    gfxSetFgColor(appSettings.colorScheme.textValue);
    gfxClearRect(13, 3, 16, 1);
    gfxPrint(13, 3, folderName);
  } else if (row == 1) {
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    if (col == 0) {
      gfxPrint(0, 5, "Create");
    } else {
      gfxPrint(8, 5, "Cancel");
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;
  if (row == 0) {
    int res = editCharacter(action, folderName, col, 16);
    if (res == 1) {
      isCharEdit = 1;
      editingString = folderName;
      editingStringLength = 16;
    } else if (res > 1) {
      handled = 1;
    }
    return handled;
  } else if (row == 1) {
    if (col == 0) {
      createFolder();
    } else {
      if (onCancelled) onCancelled();
    }
    return 0;
  }
  return 0;
}

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}
static void drawSelection(int col1, int row1, int col2, int row2) {}

static ScreenData screenData = {
  .rows = 2,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawSelection = drawSelection,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

void createFolderSetup(const char* path, void (*createdCallback)(void), void (*cancelCallback)(void)) {
  strncpy(currentPath, path, sizeof(currentPath) - 1);
  currentPath[sizeof(currentPath) - 1] = 0;
  folderName[0] = 0;
  onFolderCreated = createdCallback;
  onCancelled = cancelCallback;

  screenData.cursorRow = 0;
  screenData.cursorCol = 0;
}

static void setup(int input) {
  isCharEdit = 0;
  editingString = NULL;
  editingStringLength = 0;
}

static void fullRedraw(void) {
  screenFullRedraw(&screenData);
}

static void draw(void) {
}

static void createFolder(void) {
  if (strlen(folderName) == 0) return;

  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s%s%s", currentPath, PATH_SEPARATOR_STR, folderName);

  if (fileCreateDirectory(fullPath) == 0) {
    if (onFolderCreated) {
      onFolderCreated();
    }
  }
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (isCharEdit) {
    char result = charEditInput(keys, isDoubleTap, editingString, screenData.cursorCol, editingStringLength);
    if (result) {
      isCharEdit = 0;
      if (screenData.cursorCol < editingStringLength - 1) screenData.cursorCol++;
      editingString = NULL;
      editingStringLength = 0;
      fullRedraw();
    }
  } else {
    return screenInput(&screenData, isKeyDown, keys, isDoubleTap);
  }
  return 0;
}

const AppScreen screenCreateFolder = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
