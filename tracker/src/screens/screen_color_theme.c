#include "screen_color_theme.h"
#include "common.h"
#include "corelib_gfx.h"
#include "corelib_mainloop.h"
#include "screens.h"
#include "file_browser.h"
#include "corelib/corelib_file.h"
#include <string.h>

static int colorThemeColumnCount(int row);
static void colorThemeDrawStatic(void);
static void colorThemeDrawCursor(int col, int row);
static void colorThemeDrawRowHeader(int row, int state);
static void colorThemeDrawColHeader(int col, int state);
static void colorThemeDrawField(int col, int row, int state);
static int colorThemeOnEdit(int col, int row, enum CellEditAction action);
static void fullRedraw(void);

static ScreenData screenColorThemeData = {
  .rows = 12,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = colorThemeColumnCount,
  .drawStatic = colorThemeDrawStatic,
  .drawCursor = colorThemeDrawCursor,
  .drawRowHeader = colorThemeDrawRowHeader,
  .drawColHeader = colorThemeDrawColHeader,
  .drawField = colorThemeDrawField,
  .onEdit = colorThemeOnEdit,
};

static int isCharEdit = 0;

static void onThemeLoaded(const char* path) {
  int result = loadTheme(path);
  if (result == 0) {
    screenMessage(MESSAGE_TIME, "Theme loaded");
    // Store the directory path
    char* lastSeparator = strrchr(path, PATH_SEPARATOR);
    if (lastSeparator) {
      int pathLen = lastSeparator - path;
      if (pathLen < PATH_LENGTH) {
        strncpy(appSettings.themePath, path, pathLen);
        appSettings.themePath[pathLen] = 0;
      }
    }
    fullRedraw();
  } else {
    screenMessage(MESSAGE_TIME, "Load failed");
  }
  screenSetup(&screenColorTheme, 0);
}

static void onThemeSaved(const char* folderPath) {
  char fullPath[2048];
  snprintf(fullPath, sizeof(fullPath), "%s%s%s.cth", folderPath, PATH_SEPARATOR_STR, appSettings.themeName);
  
  if (saveTheme(fullPath) == 0) {
    screenMessage(MESSAGE_TIME, "Theme saved");
    // Store the directory path
    strncpy(appSettings.themePath, folderPath, PATH_LENGTH);
    appSettings.themePath[PATH_LENGTH] = 0;
  } else {
    screenMessage(MESSAGE_TIME, "Save failed");
  }
  screenSetup(&screenColorTheme, 0);
}

static void onThemeCancelled(void) {
  screenSetup(&screenColorTheme, 0);
}

static const char* colorNames[] = {
  "Background",
  "Text Empty",
  "Text Info",
  "Text Default",
  "Text Value",
  "Text Titles",
  "Play Markers",
  "Cursor",
  "Selection",
  "Warning"
};

static int* getColorPtr(int row) {
  switch (row) {
    case 0: return &appSettings.colorScheme.background;
    case 1: return &appSettings.colorScheme.textEmpty;
    case 2: return &appSettings.colorScheme.textInfo;
    case 3: return &appSettings.colorScheme.textDefault;
    case 4: return &appSettings.colorScheme.textValue;
    case 5: return &appSettings.colorScheme.textTitles;
    case 6: return &appSettings.colorScheme.playMarkers;
    case 7: return &appSettings.colorScheme.cursor;
    case 8: return &appSettings.colorScheme.selection;
    case 9: return &appSettings.colorScheme.warning;
    default: return NULL;
  }
}

static void setup(int input) {
  isCharEdit = 0;
}

static void fullRedraw(void) {
  screenFullRedraw(&screenColorThemeData);
}

static void draw(void) {
}

int colorThemeColumnCount(int row) {
  if (row < 10) {
    return 3; // R, G, B
  } else if (row == 10) {
    return THEME_NAME_LENGTH; // Theme name field
  } else if (row == 11) {
    return 3; // Save, Load, Reset
  }
  return 1;
}

void colorThemeDrawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "COLOR THEME");

  gfxSetFgColor(cs.textDefault);
  gfxPrint(17, 1, "R  G  B");
  gfxPrint(0, 17, "Theme name");
}

void colorThemeDrawCursor(int col, int row) {
  if (row < 10) {
    if (col == 0) {
      gfxCursor(17, row + 2, 2); // R column
    } else if (col == 1) {
      gfxCursor(20, row + 2, 2); // G column
    } else if (col == 2) {
      gfxCursor(23, row + 2, 2); // B column
    }
  } else if (row == 10) {
    gfxCursor(17 + col, 17, 1); // Theme name field
  } else if (row == 11) {
    if (col == 0) {
      gfxCursor(17, 18, 4); // Save
    } else if (col == 1) {
      gfxCursor(22, 18, 4); // Load
    } else if (col == 2) {
      gfxCursor(27, 18, 5); // Reset
    }
  }
}

void colorThemeDrawRowHeader(int row, int state) {
  if (row < 10) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(0, row + 2, colorNames[row]);

    // Draw color preview as part of row header
    int* colorPtr = getColorPtr(row);
    if (colorPtr) {
      gfxSetBgColor(*colorPtr);
      gfxPrint(26, row + 2, "   ");
      gfxSetBgColor(appSettings.colorScheme.background);
    }
  }
}

void colorThemeDrawColHeader(int col, int state) {
}

void colorThemeDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row < 10) {
    int* colorPtr = getColorPtr(row);
    if (!colorPtr) return;

    int color = *colorPtr;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    if (col == 0) {
      gfxPrintf(17, row + 2, "%02X", r);
    } else if (col == 1) {
      gfxPrintf(20, row + 2, "%02X", g);
    } else if (col == 2) {
      gfxPrintf(23, row + 2, "%02X", b);
    }
  } else if (row == 10) {
    // Theme name field
    gfxClearRect(17, 17, THEME_NAME_LENGTH, 1);
    gfxPrintf(17, 17, "%s", appSettings.themeName);
  } else if (row == 11) {
    // Buttons
    if (col == 0) {
      gfxPrint(17, 18, "Save");
    } else if (col == 1) {
      gfxPrint(22, 18, "Load");
    } else if (col == 2) {
      gfxPrint(27, 18, "Reset");
    }
  }
}

int colorThemeOnEdit(int col, int row, enum CellEditAction action) {
  if (row < 10) {
    int* colorPtr = getColorPtr(row);
    if (!colorPtr) return 0;

    int color = *colorPtr;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    int handled = 0;

    if (col == 0) {
      // Edit R component
      handled = edit8noLast(action, &r, 16, 0, 255);
    } else if (col == 1) {
      // Edit G component
      handled = edit8noLast(action, &g, 16, 0, 255);
    } else if (col == 2) {
      // Edit B component
      handled = edit8noLast(action, &b, 16, 0, 255);
    }

    if (handled) {
      *colorPtr = (r << 16) | (g << 8) | b;
      // Redraw the row header to update color preview
      colorThemeDrawRowHeader(row, 0);
      // Redraw whole screen to preview color changes
      fullRedraw();
    }

    return handled;
  } else if (row == 10) {
    // Theme name editing
    int res = editCharacter(action, appSettings.themeName, col, THEME_NAME_LENGTH);
    if (res == 1) {
      isCharEdit = 1;
    } else if (res > 1) {
      return 1;
    }
  } else if (row == 11 && action == editTap) {
    // Buttons
    if (col == 0) {
      // Save
      if (strlen(appSettings.themeName) == 0) {
        screenMessage(MESSAGE_TIME, "Enter theme name");
      } else {
        fileBrowserSetupFolderMode("SAVE THEME", appSettings.themePath, appSettings.themeName, ".cth", onThemeSaved, onThemeCancelled);
        screenSetup(&screenFileBrowser, 0);
      }
      return 1;
    } else if (col == 1) {
      // Load
      fileBrowserSetup("LOAD THEME", ".cth", appSettings.themePath, onThemeLoaded, onThemeCancelled);
      screenSetup(&screenFileBrowser, 0);
      return 1;
    } else if (col == 2) {
      // Reset
      resetToDefaultColors();
      screenMessage(MESSAGE_TIME, "Reset to default");
      fullRedraw();
      return 1;
    }
  }

  return 0;
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == keyOpt && screenColorThemeData.cursorRow != 10) {
    screenSetup(&screenSettings, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (isCharEdit) {
    char result = charEditInput(keys, isDoubleTap, appSettings.themeName, screenColorThemeData.cursorCol, THEME_NAME_LENGTH);
    if (result) {
      isCharEdit = 0;
      if (screenColorThemeData.cursorCol < THEME_NAME_LENGTH - 1) screenColorThemeData.cursorCol++;
      fullRedraw();
    }
    return 1;
  }

  if (inputScreenNavigation(keys, isDoubleTap)) return 1;

  return screenInput(&screenColorThemeData, isKeyDown, keys, isDoubleTap);
}

const AppScreen screenColorTheme = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};