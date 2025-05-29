#include <screen_project.h>
#include <corelib_gfx.h>

static char stereoModes[4][5] = {
  "ABC",
  "ACB",
  "BAC",
  "Mono",
};

static int getColumnCount(int row) {
  // The first 7 rows come from the common project screen fields
  if (row < 7) return projectCommonColumnCount(row);

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
  if (row < 7) return projectCommonDrawCursor(col, row);

}

static void drawField(int col, int row, int state) {
  if (row < 7) return projectCommonDrawField(col, row, state);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 7) {
    gfxPrint(13, 10, project.chipSetup.ay.isYM ? "YM2149F" : "AY-3-8910");
  } else if (row == 8) {
    // Panning scheme
    int stereo = 0;
    struct ChipSetupAY* ay = &project.chipSetup.ay;
    if (ay->panA == 128 && ay->panB == 128 && ay->panC == 128) {
      stereo = 3; // Mono
    } else if (ay->panA == 128) {
      stereo = 2; // BAC
    } else if (ay->panB == 128) {
      stereo = 0; // ABC
    } else if (ay->panC == 128) {
      stereo = 1; // ACB
    }

    gfxPrint(13, 11, stereoModes[stereo]);
  } else if (row == 9) {
    gfxPrintf(13, 12, "%d Hz", project.chipSetup.ay.clock);
  } else if (row == 10) {
    gfxPrint(13, 13, project.pitchTable.name);
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 7) return projectCommonOnEdit(col, row, action);

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
