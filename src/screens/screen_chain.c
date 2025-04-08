#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

int chain;

static void setup(int input) {
  chain = input;
}

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "CHAIN %02x", chain);
}

static void draw(void) {

}

static void onKey(int keys, int isDoubleTap) {
  //printf("%d\n", event.data.key.keys);
  if (keys == (keyRight | keyShift)) {
    setupScreen(screenPhrase, 0);
  } else if (keys == (keyLeft | keyShift)) {
    setupScreen(screenSong, 0);
  }
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
