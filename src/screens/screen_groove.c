#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static int groove = 0;
static uint8_t lastValue = 0;

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static int onEdit(int col, int row, enum CellEditAction action);

static struct ScreenData screen = {
  .rows = 16,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .isSelectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

static void setup(int input) {
  groove = input;
}

static int getColumnCount(int row) {
  return 1;  // Only one column for groove values
}

static void drawStatic(void) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "GROOVE %02X", groove);
}

static void drawField(int col, int row, int state) {
  uint8_t value = project.grooves[groove].speed[row];
  setCellColor(state, value == EMPTY_VALUE_8, value != 0);
  gfxPrintf(3, 3 + row, byteToHexOrEmpty(value));
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrint(3, 2, "T");  // T for Timing
}

static void drawCursor(int col, int row) {
  gfxCursor(3, 3 + row, 2);
}

static int onEdit(int col, int row, enum CellEditAction action) {
  return edit8withLimit(action, &project.grooves[groove].speed[row], &lastValue, 16, EMPTY_VALUE_8 - 1);
}

static void fullRedraw(void) {
  screenFullRedraw(&screen);
}

static void draw(void) {
  // Clear the marker column
  gfxClearRect(2, 3, 1, 16);

  // Show play position if this groove is currently playing
  if (playback.tracks[*pSongTrack].grooveIdx == groove) {
    int row = playback.tracks[*pSongTrack].grooveRow;
    if (row >= 0 && row < 16) {
      gfxSetFgColor(appSettings.colorScheme.playMarkers);
      gfxPrint(2, 3 + row, ">");
    }
  }
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyDown | keyShift)) {
    // To Phrase scrren
    screenSetup(&screenPhrase, 0);
    return 1;
  } else if (keys == (keyLeft | keyOpt)) {
    // To previous groove
    if (groove > 0) {
      groove--;
      fullRedraw();
      return 1;
    }
  } else if (keys == (keyRight | keyOpt)) {
    // To next groove
    if (groove < PROJECT_MAX_GROOVES) {
      groove++;
      fullRedraw();
      return 1;
    }
  } else if (keys == (keyUp | keyOpt)) {
    // +16 grooves
    groove += 16;
    if (groove >= PROJECT_MAX_GROOVES) groove = PROJECT_MAX_GROOVES - 1;
    fullRedraw();
    return 1;
  } else if (keys == (keyDown | keyOpt)) {
    // -16 grooves
    groove -= 16;
    if (groove < 0) groove = 0;
    fullRedraw();
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {

  if (inputScreenNavigation(keys, isDoubleTap)) return;
  screenInput(&screen, keys, isDoubleTap);
}

const struct AppScreen screenGroove = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
