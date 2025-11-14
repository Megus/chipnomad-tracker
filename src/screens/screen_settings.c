#include <screen_settings.h>
#include <common.h>
#include <corelib_gfx.h>
#include <corelib_mainloop.h>
#include <screens.h>

// Forward declarations
static int settingsColumnCount(int row);
static void settingsDrawStatic(void);
static void settingsDrawCursor(int col, int row);
static void settingsDrawRowHeader(int row, int state);
static void settingsDrawColHeader(int col, int state);
static void settingsDrawField(int col, int row, int state);
static int settingsOnEdit(int col, int row, enum CellEditAction action);

static struct ScreenData screenSettingsData = {
  .rows = 1,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = settingsColumnCount,
  .drawStatic = settingsDrawStatic,
  .drawCursor = settingsDrawCursor,
  .drawRowHeader = settingsDrawRowHeader,
  .drawColHeader = settingsDrawColHeader,
  .drawField = settingsDrawField,
  .onEdit = settingsOnEdit,
};

static void setup(int input) {
}

static void fullRedraw(void) {
  screenFullRedraw(&screenSettingsData);
}

static void draw(void) {
}

int settingsColumnCount(int row) {
  return 1; // Only one button
}

void settingsDrawStatic(void) {
  const struct ColorScheme cs = appSettings.colorScheme;
  
  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "SETTINGS");
  
  gfxSetFgColor(cs.textInfo);
  gfxPrint(12, 10, "Under construction");
}

void settingsDrawCursor(int col, int row) {
  if (row == 0 && col == 0) {
    gfxCursor(0, 2, 14); // "Quit ChipNomad"
  }
}

void settingsDrawRowHeader(int row, int state) {
}

void settingsDrawColHeader(int col, int state) {
}

void settingsDrawField(int col, int row, int state) {
  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
  
  if (row == 0 && col == 0) {
    gfxPrint(0, 2, "Quit ChipNomad");
  }
}

int settingsOnEdit(int col, int row, enum CellEditAction action) {
  if (row == 0 && col == 0 && action == editTap) {
    // Trigger exit event
    mainLoopTriggerQuit();
    return 1;
  }
  return 0;
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyUp | keyShift)) {
    screenSetup(&screenSong, 0);
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  screenInput(&screenSettingsData, keys, isDoubleTap);
}

const struct AppScreen screenSettings = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};