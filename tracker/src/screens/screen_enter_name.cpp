#include "screens.h"
#include "screen_enter_name.h"
#include "corelib_gfx.h"
#include <string.h>

static char enteredName[256] = "";
static char title[32] = "";
static char prompt[32] = "";
static int isCharEdit = 0;
static char* editingString = NULL;
static int editingStringLength = 0;
static void (*onConfirmed)(const char* name);
static void (*onCancelled)(void);

static void confirmName(void);
static void fullRedraw(void);

// Helper: Trim whitespace from both ends of a string
static void trimWhitespace(char* str) {
  if (!str) return;

  // Trim leading whitespace
  int start = 0;
  while (str[start] == ' ' || str[start] == '\t') {
    start++;
  }

  // Trim trailing whitespace
  int end = strlen(str + start);
  while (end > 0 && (str[start + end - 1] == ' ' || str[start + end - 1] == '\t')) {
    end--;
  }

  // Move trimmed string to start
  if (start > 0) {
    memmove(str, str + start, end);
  }
  str[end] = '\0';
}

static int getColumnCount(int row) {
  if (row == 0) return 24; // Name field (24 chars to fit on screen with label)
  if (row == 1) return 2;  // OK, Cancel buttons
  return 1;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, title);

  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 3, prompt);
}

static void drawCursor(int col, int row) {
  if (row == 0) {
    gfxCursor(col, 4, 1);  // Text field on line below prompt, starting at x=0
  } else if (row == 1) {
    if (col == 0) {
      gfxCursor(0, 6, 2);  // Buttons moved down one line
    } else {
      gfxCursor(4, 6, 6);
    }
  }
}

static void drawField(int col, int row, CellState state) {
  if (row == 0) {
    gfxSetFgColor(appSettings.colorScheme.textValue);
    gfxClearRect(0, 4, 24, 1);  // Text field on line below prompt, full 24 chars
    gfxPrint(0, 4, enteredName);
  } else if (row == 1) {
    gfxSetFgColor(state == CellState::focus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    if (col == 0) {
      gfxPrint(0, 6, "OK");  // Buttons moved down one line
    } else {
      gfxPrint(4, 6, "Cancel");
    }
  }
}

static int onEdit(int col, int row, CellEditAction action) {
  int handled = 0;
  if (row == 0) {
    int res = editCharacter(action, enteredName, col, 24);
    if (res == 1) {
      isCharEdit = 1;
      editingString = enteredName;
      editingStringLength = 24;
    } else if (res > 1) {
      handled = 1;
    }
    return handled;
  } else if (row == 1) {
    if (col == 0) {
      confirmName();
    } else {
      if (onCancelled) onCancelled();
    }
    return 0;
  }
  return 0;
}

static void drawRowHeader(int row, CellState state) {}
static void drawColHeader(int col, CellState state) {}
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

void enterNameSetup(const char* titleText, const char* promptText, const char* initialName, void (*confirmCallback)(const char* name), void (*cancelCallback)(void)) {
  strncpy(title, titleText, sizeof(title) - 1);
  title[sizeof(title) - 1] = 0;
  strncpy(prompt, promptText, sizeof(prompt) - 1);
  prompt[sizeof(prompt) - 1] = 0;

  if (initialName) {
    strncpy(enteredName, initialName, sizeof(enteredName) - 1);
    enteredName[sizeof(enteredName) - 1] = 0;
  } else {
    enteredName[0] = 0;
  }

  onConfirmed = confirmCallback;
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

static void confirmName(void) {
  trimWhitespace(enteredName);

  if (strlen(enteredName) == 0) return;

  if (onConfirmed) {
    onConfirmed(enteredName);
  }
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (isCharEdit) {
    char result = charEditInput(keys, tapCount, editingString, screenData.cursorCol, editingStringLength);
    if (result) {
      isCharEdit = 0;
      if (screenData.cursorCol < editingStringLength - 1) screenData.cursorCol++;
      editingString = NULL;
      editingStringLength = 0;
      fullRedraw();
    }
  } else {
    return screenInput(&screenData, isKeyDown, keys, tapCount);
  }
  return 0;
}

const AppScreen screenEnterName = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
