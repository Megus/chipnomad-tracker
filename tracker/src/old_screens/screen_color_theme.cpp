#include "screen_color_theme.h"
#include "common.h"
#include "gfx.h"
#include "mainloop.h"
#include "screens.h"
#include "file_browser.h"
#include "file_system.h"
#include <string.h>

static int colorThemeColumnCount(int row);
static void colorThemeDrawStatic(void);
static void colorThemeDrawCursor(int col, int row);
static void colorThemeDrawRowHeader(int row, CellState state);
static void colorThemeDrawColHeader(int col, CellState state);
static void colorThemeDrawField(int col, int row, CellState state);
static int colorThemeOnEdit(int col, int row, EditCellAction action);
static void fullRedraw(void);

static ScreenData screenColorThemeData = {
  .rows = 12,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = -1,
  .selectStartRow = 0,
  .selectStartCol = 0,
  .selectAnchorRow = 0,
  .selectAnchorCol = 0,
  .playbackLevel = ScreenPlaybackLevel::none,
  .getColumnCount = colorThemeColumnCount,
  .drawStatic = colorThemeDrawStatic,
  .drawCursor = colorThemeDrawCursor,
  .drawSelection = NULL,
  .drawRowHeader = colorThemeDrawRowHeader,
  .drawColHeader = colorThemeDrawColHeader,
  .drawField = colorThemeDrawField,
  .onEdit = colorThemeOnEdit,
  .onInput = NULL,
  .onRawInput = NULL,
  .isCellValid = NULL,
  .getLoopRange = NULL,
};

static int isCharEdit = 0;

static void onThemeLoaded(const char* path) {
  if (loadTheme(path)) {
    screenMessage(MESSAGE_TIME, "Theme loaded");
    extractFilenameWithoutExtension(path, appSettings.themeName, THEME_NAME_LENGTH + 1);
    // Store the directory path
    const char* lastSeparator = strrchr(path, PATH_SEPARATOR);
    if (lastSeparator) {
      int pathLen = lastSeparator - path;
      if (pathLen < PATH_LENGTH) {
        copyString(appSettings.themePath, path, pathLen);
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

  if (saveTheme(fullPath)) {
    screenMessage(MESSAGE_TIME, "Theme saved");
    // Store the directory path
    copyString(appSettings.themePath, folderPath, PATH_LENGTH);
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
    case 0: return &theme().background;
    case 1: return &theme().textEmpty;
    case 2: return &theme().textInfo;
    case 3: return &theme().textDefault;
    case 4: return &theme().textValue;
    case 5: return &theme().textTitles;
    case 6: return &theme().playMarkers;
    case 7: return &theme().cursor;
    case 8: return &theme().selection;
    case 9: return &theme().warning;
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
  const ColorTheme t = theme();

  gfx.setFgColor(t.textTitles);
  gfx.print(0, 0, "COLOR THEME");

  gfx.setFgColor(t.textDefault);
  gfx.print(17, 1, "R  G  B");
  gfx.print(0, 17, "Theme name");
}

void colorThemeDrawCursor(int col, int row) {
  if (row < 10) {
    if (col == 0) {
      gfx.cursor(17, row + 2, 2); // R column
    } else if (col == 1) {
      gfx.cursor(20, row + 2, 2); // G column
    } else if (col == 2) {
      gfx.cursor(23, row + 2, 2); // B column
    }
  } else if (row == 10) {
    gfx.cursor(17 + col, 17, 1); // Theme name field
  } else if (row == 11) {
    if (col == 0) {
      gfx.cursor(17, 18, 4); // Save
    } else if (col == 1) {
      gfx.cursor(22, 18, 4); // Load
    } else if (col == 2) {
      gfx.cursor(27, 18, 5); // Reset
    }
  }
}

void colorThemeDrawRowHeader(int row, CellState state) {
  if (row < 10) {
    gfx.setFgColor(theme().textDefault);
    gfx.print(0, row + 2, colorNames[row]);

    // Draw color preview as part of row header
    int* colorPtr = getColorPtr(row);
    if (colorPtr) {
      gfxSetBgColor(*colorPtr);
      gfx.print(26, row + 2, "   ");
      gfxSetBgColor(theme().background);
    }
  }
}

void colorThemeDrawColHeader(int col, CellState state) {
}

void colorThemeDrawField(int col, int row, CellState state) {
  gfx.setFgColor(state == CellState::focus ? theme().textValue : theme().textDefault);

  if (row < 10) {
    int* colorPtr = getColorPtr(row);
    if (!colorPtr) return;

    int color = *colorPtr;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    if (col == 0) {
      gfx.printf(17, row + 2, "%02X", r);
    } else if (col == 1) {
      gfx.printf(20, row + 2, "%02X", g);
    } else if (col == 2) {
      gfx.printf(23, row + 2, "%02X", b);
    }
  } else if (row == 10) {
    // Theme name field
    gfx.clearRect(17, 17, THEME_NAME_LENGTH, 1);
    gfx.printf(17, 17, "%s", appSettings.themeName);
  } else if (row == 11) {
    // Buttons
    if (col == 0) {
      gfx.print(17, 18, "Save");
    } else if (col == 1) {
      gfx.print(22, 18, "Load");
    } else if (col == 2) {
      gfx.print(27, 18, "Reset");
    }
  }
}

int colorThemeOnEdit(int col, int row, EditCellAction action) {
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
      colorThemeDrawRowHeader(row, CellState::normal);
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
  } else if (row == 11 && action == EditCellAction::tap) {
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

static int inputScreenNavigation(int keys, int tapCount) {
  if (keys == keyOpt && screenColorThemeData.cursorRow != 10) {
    screenSetup(&screenSettings, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (isCharEdit) {
    char result = charEditInput(keys, tapCount, appSettings.themeName, screenColorThemeData.cursorCol, THEME_NAME_LENGTH);
    if (result) {
      isCharEdit = 0;
      if (screenColorThemeData.cursorCol < THEME_NAME_LENGTH - 1) screenColorThemeData.cursorCol++;
      fullRedraw();
    }
    return 1;
  }

  if (inputScreenNavigation(keys, tapCount)) return 1;

  return screenInput(&screenColorThemeData, isKeyDown, keys, tapCount);
}

const AppScreen screenColorTheme = {
  .init = NULL,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .getPlaybackLevel = NULL,
};