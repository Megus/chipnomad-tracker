#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <song.h>

int cursorY = 0;
int cursorX = 0;
int topY = 0;

static void setup(int input) {

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
  for (int c = 0; c < song.tracksCount; c++) {
    digit[0] = c + 49;
    gfxPrint(3 + c * 3, 2, digit);
  }

  // Chains
  for (int c = 0; c < 16; c++) {
    for (int d = 0; d < song.tracksCount; d++) {
      uint8_t chain = song.song[topY + c][d];
      gfxSetFgColor(chain == 0xff ? cs.textEmpty : cs.textValue);
      gfxPrint(3 + d * 3, 3 + c, chain == 0xff ? "--" : byteToHex(chain));
    }
  }
}

static void draw(void) {

}

static int onEvent(struct AppEvent event) {
  return 0;
}

const struct AppScreen screenSong = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onEvent = onEvent
};
