#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

int chain = 0;
int cursorY = 0;
int cursorX = 0;

static void setup(int input) {
  if (input != chain) {
    cursorY = 0;
    cursorX = 0;
  }
  chain = input;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void drawRow(int row) {
  const struct ColorScheme cs = appSettings.colorScheme;

  // Phrase
  uint16_t phrase = project.chains[chain].phrases[row];

  if (row == cursorY && cursorX == 0) {
    gfxSetFgColor(cs.textDefault);
  } else if (phrase == EMPTY_VALUE_16) {
    gfxSetFgColor(cs.textEmpty);
  } else if (project.phrases[phrase].hasNoNotes) {
    gfxSetFgColor(cs.textInfo);
  } else {
    gfxSetFgColor(cs.textValue);
  }

  if (phrase == EMPTY_VALUE_16) {
    gfxPrint(2, 3 + row, "---");
  } else {
    gfxPrintf(2, 3 + row, "%03X", phrase);
  }

  // Transpose
  if (row == cursorY && cursorX == 1) {
    gfxSetFgColor(cs.textDefault);
  } else if (project.phrases[phrase].hasNoNotes) {
    gfxSetFgColor(cs.textInfo);
  } else {
    gfxSetFgColor(cs.textValue);
  }

  gfxPrint(6, 3 + row, byteToHex(project.chains[chain].transpose[row]));
}

static void drawCursor() {
  const struct ColorScheme cs = appSettings.colorScheme;

  // Row numbers
  for (int c = 0; c < 16; c++) {
    gfxSetFgColor(c == cursorY ? cs.textDefault : cs.textInfo);
    gfxPrintf(0, 3 + c, "%X", c);
  }

  // Columns
  gfxSetFgColor(cursorX == 0 ? cs.textDefault : cs.textInfo);
  gfxPrint(2, 2, "P");
  gfxSetFgColor(cursorX == 1 ? cs.textDefault : cs.textInfo);
  gfxPrint(6, 2, "T");

}

static void fullRedraw(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "CHAIN %02x", chain);

  for (int c = 0; c < 16; c++) {
    drawRow(c);
  }
  drawCursor();
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    setupScreen(screenPhrase, 0);
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    setupScreen(screenSong, 0);
    return 1;
  }
  return 0;
}

static int inputCursor(int keys, int isDoubleTap) {

  return 0;
}

static int inputEdit(int keys, int isDoubleTap) {

  return 0;
}


static void onKey(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (inputCursor(keys, isDoubleTap)) return;
  if (inputEdit(keys, isDoubleTap)) return;
}

int screenChain(struct AppEvent event) {
  switch (event.type) {
    case appEventSetup:
      setup(event.data.setup.input);
      break;
    case appEventFullRedraw:
      fullRedraw();
      break;
    case appEventDraw:
      draw();
      break;
    case appEventKey:
      onKey(event.data.key.keys, event.data.key.isDoubleTap);
      break;
  }

  return 0;
}
