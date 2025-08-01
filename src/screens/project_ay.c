#include <screen_project.h>
#include <corelib_gfx.h>
#include <string.h>

int chipClockLength = 0;

static char stereoModes[4][5] = {
  "ABC",
  "ACB",
  "BAC",
  "Mono",
};

static int getColumnCount(int row) {
  // The first 7 rows come from the common project screen fields
  if (row < SCR_PROJECT_ROWS) return projectCommonColumnCount(row);

  return 1; // Default
}

static void drawStatic(void) {
  projectCommonDrawStatic();
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 10, "Subtype");
  gfxPrint(0, 11, "Stereo");
  gfxPrint(0, 12, "Chip clock");
  gfxPrint(0, 13, "Pitch table");
}

static void drawCursor(int col, int row) {
  if (row < SCR_PROJECT_ROWS) return projectCommonDrawCursor(col, row);
  if (row == SCR_PROJECT_ROWS) {
    // Chip type
    gfxCursor(13, 10, project.chipSetup.ay.isYM ? 7 : 9);
  } else if (row == SCR_PROJECT_ROWS + 1) {
    // Panning scheme
    gfxCursor(13, 11, 3);
  } else if (row == SCR_PROJECT_ROWS + 2) {
    // Chip clock
    gfxCursor(13, 12, chipClockLength);
  } else if (row == SCR_PROJECT_ROWS + 3) {
    // Pitch table
    gfxCursor(13, 13, strlen(project.pitchTable.name));
  }
}

static void drawField(int col, int row, int state) {
  if (row < SCR_PROJECT_ROWS) return projectCommonDrawField(col, row, state);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == SCR_PROJECT_ROWS) {
    gfxClearRect(13, 10, 9, 1);
    gfxPrint(13, 10, project.chipSetup.ay.isYM ? "YM2149F" : "AY-3-8910");
  } else if (row == SCR_PROJECT_ROWS + 1) {
    // Panning scheme
    gfxPrint(13, 11, stereoModes[project.chipSetup.ay.stereoMode]);
  } else if (row == SCR_PROJECT_ROWS + 2) {
    char buf[18];
    sprintf(buf, "%d Hz", project.chipSetup.ay.clock);
    chipClockLength = strlen(buf);
    gfxPrintf(13, 12, buf);
  } else if (row == SCR_PROJECT_ROWS + 3) {
    gfxClearRect(13, 13, PROJECT_PITCH_TABLE_TITLE_LENGTH, 1);
    gfxPrint(13, 13, project.pitchTable.name);
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < SCR_PROJECT_ROWS) return projectCommonOnEdit(col, row, action);

  int handled = 0;


  return handled;
}

struct ScreenData screenProjectAY = {
  .rows = 11,
  .cursorRow = 0,
  .cursorCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
};
