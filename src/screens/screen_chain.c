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
  gfxPrint(0, 0, "CHAIN");
}

static void draw(void) {

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
      break;
  }

  return 0;
}
