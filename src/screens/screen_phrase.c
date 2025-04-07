#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static void setup(int input) {

}

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "PHRASE");
}

static void draw(void) {

}

static void onKey(int keys, int isDoubleTap) {
  //printf("%d\n", event.data.key.keys);
  if (keys == (keyRight | keyShift)) {
    setupScreen(screenInstrument, 0);
  } else if (keys == (keyLeft | keyShift)) {
    setupScreen(screenChain, 0);
  } else if (keys == (keyUp | keyShift)) {
    setupScreen(screenGroove, 0);
  }
}

int screenPhrase(struct AppEvent event) {
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
