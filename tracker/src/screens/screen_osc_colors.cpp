#include "screen_osc_colors.h"
#include "common.h"
#include "corelib_gfx.h"
#include "screens.h"

static int oscColorsColumnCount(int row);
static void oscColorsDrawStatic(void);
static void oscColorsDrawCursor(int col, int row);
static void oscColorsDrawRowHeader(int row, CellState state);
static void oscColorsDrawColHeader(int col, CellState state);
static void oscColorsDrawField(int col, int row, CellState state);
static int oscColorsOnEdit(int col, int row, CellEditAction action);
static void fullRedraw(void);

static ScreenData screenOscColorsData = {
  .rows = 8,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = -1,
  .selectStartRow = 0,
  .selectStartCol = 0,
  .selectAnchorRow = 0,
  .selectAnchorCol = 0,
  .playbackLevel = ScreenPlaybackLevel::none,
  .getColumnCount = oscColorsColumnCount,
  .drawStatic = oscColorsDrawStatic,
  .drawCursor = oscColorsDrawCursor,
  .drawSelection = NULL,
  .drawRowHeader = oscColorsDrawRowHeader,
  .drawColHeader = oscColorsDrawColHeader,
  .drawField = oscColorsDrawField,
  .onEdit = oscColorsOnEdit,
  .onInput = NULL,
  .onRawInput = NULL,
  .isCellValid = NULL,
  .getLoopRange = NULL,
};

static const char* oscColorNames[] = {
  "Voice 1",
  "Voice 2",
  "Voice 3",
  "Voice 4",
  "Voice 5",
  "Voice 6",
  "Voice 7",
  "Voice 8"
};

static int* getColorPtr(int row) {
  if (row >= 0 && row < 8) return &appSettings.colorScheme.oscColors[row];
  return NULL;
}

static void setup(int input) {
  (void)input;
}

static void fullRedraw(void) {
  screenFullRedraw(&screenOscColorsData);
}

static void draw(void) {
}

int oscColorsColumnCount(int row) {
  (void)row;
  return 3; // R, G, B
}

void oscColorsDrawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "OSC COLORS");

  gfxSetFgColor(cs.textDefault);
  gfxPrint(17, 1, "R  G  B");
}

void oscColorsDrawCursor(int col, int row) {
  if (col == 0) {
    gfxCursor(17, row + 2, 2);
  } else if (col == 1) {
    gfxCursor(20, row + 2, 2);
  } else if (col == 2) {
    gfxCursor(23, row + 2, 2);
  }
}

void oscColorsDrawRowHeader(int row, CellState state) {
  (void)state;
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, row + 2, oscColorNames[row]);

  // Draw color preview (gfxClearRect ignores the transparent-text flag)
  int* colorPtr = getColorPtr(row);
  if (colorPtr) {
    gfxSetBgColor(*colorPtr);
    gfxClearRect(26, row + 2, 3, 1);
    gfxSetBgColor(appSettings.colorScheme.background);
  }
}

void oscColorsDrawColHeader(int col, CellState state) {
  (void)col;
  (void)state;
}

void oscColorsDrawField(int col, int row, CellState state) {
  gfxSetFgColor(state == CellState::focus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

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
}

int oscColorsOnEdit(int col, int row, CellEditAction action) {
  int* colorPtr = getColorPtr(row);
  if (!colorPtr) return 0;

  int color = *colorPtr;
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  int handled = 0;
  if (col == 0) {
    handled = edit8noLast(action, &r, 16, 0, 255);
  } else if (col == 1) {
    handled = edit8noLast(action, &g, 16, 0, 255);
  } else if (col == 2) {
    handled = edit8noLast(action, &b, 16, 0, 255);
  }

  if (handled) {
    *colorPtr = (r << 16) | (g << 8) | b;
    oscColorsDrawRowHeader(row, CellState::normal);
    fullRedraw();
  }

  return handled;
}

static int inputScreenNavigation(int keys, int tapCount) {
  (void)tapCount;
  if (keys == keyOpt) {
    screenSetup(&screenSettings, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (inputScreenNavigation(keys, tapCount)) return 1;
  return screenInput(&screenOscColorsData, isKeyDown, keys, tapCount);
}

const AppScreen screenOscColors = {
  .init = NULL,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .getPlaybackLevel = NULL,
};
