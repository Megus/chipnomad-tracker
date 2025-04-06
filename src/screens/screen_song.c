#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

int cursorY = 0;
int cursorX = 0;
int topY = 0;

static void setup() {

}

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "SONG");

  // Positions list
  gfxSetFgColor(cs.textInfo);
  for (int c = 0; c < 16; c++) {
    gfxPrint(0, 3 + c, byteToHex(topY + c));
  }

  // Track names
  char digit[2] = "0";
  for (int c = 0; c < project.tracksCount; c++) {
    digit[0] = c + 49;
    gfxPrint(3 + c * 3, 2, digit);
  }

  // Chains
  for (int c = 0; c < 16; c++) {
    for (int d = 0; d < project.tracksCount; d++) {
      uint8_t chain = project.song[topY + c][d];
      gfxSetFgColor(chain == 0xff ? cs.textEmpty : cs.textValue);
      gfxPrint(3 + d * 3, 3 + c, chain == 0xff ? "--" : byteToHex(chain));
    }
  }
}

static void draw(void) {

}

int screenSong(struct AppEvent event) {
  switch (event.type) {
    case appEventSetup:
      setup();
      break;
    case appEventFullRedraw:
      fullRedraw();
      break;
    case appEventDraw:
      draw();
      break;
    case appEventKey:
      printf("%d\n", event.data.key.keys);
      break;
  }

  return 0;
}
