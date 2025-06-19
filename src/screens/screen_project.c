#include <screen_project.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <version.h>

static int isCharEdit = 0;

static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);

static struct ScreenData screenProjectCommon = {
  .rows = 7,
  .cursorRow = 0,
  .cursorCol = 0,
  .isSelectMode = -1,
  .getColumnCount = projectCommonColumnCount,
  .drawStatic = projectCommonDrawStatic,
  .drawCursor = projectCommonDrawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = projectCommonDrawField,
  .onEdit = projectCommonOnEdit,
};

static struct ScreenData* projectScreen(void) {
  struct ScreenData* data = &screenProjectCommon;
  if (project.chipType == chipAY) {
    data = &screenProjectAY;
  }
  data->drawRowHeader = drawRowHeader;
  data->drawColHeader = drawColHeader;

  return data;
}

static void setup(int input) {
  isCharEdit = 0;
}

static void fullRedraw(void) {
  struct ScreenData* screen = projectScreen();
  screenFullRedraw(screen);
}

static void draw(void) {
}

///////////////////////////////////////////////////////////////////////////////
//
// Common part of the form
//

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}

int projectCommonColumnCount(int row) {
  if (row == 0) {
    return 4; // Load, save, new, export
  } else if (row >= 1 && row <= 3) {
    return 24; // File, title, author
  } else if (row == 4) {
    return 2; // Tick rate
  } else if (row > 4) {
    return 1; // Chip type, chips count
  }
  return 1; // Default value
}

void projectCommonDrawStatic(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "PROJECT");

  gfxSetFgColor(cs.textDefault);
  gfxPrintf(8, 0, "%s v%s (%s)", appTitle, appVersion, appBuild);

  gfxPrint(0, 3, "File");
  gfxPrint(0, 4, "Title");
  gfxPrint(0, 5, "Author");

  gfxPrint(0, 7, "Tick rate");
  gfxPrint(0, 8, "Chip type");
  gfxPrint(0, 9, "Chips count");
}

void projectCommonDrawCursor(int col, int row) {
  if (row == 0) {
    if (col == 0) {
      gfxCursor(7, 2, 4); // Load
    } else if (col == 1) {
      gfxCursor(12, 2, 4); // Save
    } else if (col == 2) {
      gfxCursor(17, 2, 3); // New
    } else if (col == 3) {
      gfxCursor(21, 2, 6); // Export
    }
  } else if (row >= 1 && row <= 3) {
    gfxCursor(7 + col, 2 + row, 1);
  }
}

void projectCommonDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 0) {
    if (col == 0) {
      gfxPrint(7, 2, "Load");
    } else if (col == 1) {
      gfxPrint(12, 2, "Save");
    } else if (col == 2) {
      gfxPrint(17, 2, "New");
    } else if (col == 3) {
      gfxPrint(21, 2, "Export");
    }
  } else if (row == 1) {
    // File name
    gfxClearRect(7, 3, FILENAME_LENGTH, 1);
  } else if (row == 2) {
    // Title
    gfxClearRect(7, 4, PROJECT_TITLE_LENGTH, 1);
    gfxPrintf(7, 4, "%s", project.title);
  } else if (row == 3) {
    // Author
    gfxClearRect(7, 5, PROJECT_TITLE_LENGTH, 1);
    gfxPrintf(7, 5, "%s", project.author);
  } else if (row == 4) {
    //gfxPrintf(7, 7, "%d", project.frameRate);
  } else if (row == 5) {
    gfxPrint(13, 8, "AY/YM");
  } else if (row == 6) {
    gfxPrintf(13, 9, "%d", project.chipsCount);
  }
}

int projectCommonOnEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  return handled;
}


///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyDown | keyShift)) {
    screenSetup(&screenSong, 0);
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (isCharEdit) {
    /*struct ScreenData* screen = instrumentScreen();
    char result = charEditInput(keys, isDoubleTap, project.instruments[cInstrument].name, screen->cursorCol, PROJECT_INSTRUMENT_NAME_LENGTH);
    if (result) {
      isCharEdit = 0;
      if (screen->cursorCol < 15) screen->cursorCol++;
      fullRedraw();
    }*/
  } else {
    if (inputScreenNavigation(keys, isDoubleTap)) return;

    struct ScreenData* screen = projectScreen();
    if (screenInput(screen, keys, isDoubleTap)) return;
  }
}

const struct AppScreen screenProject = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};