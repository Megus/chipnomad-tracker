#include <screens.h>
#include <file_browser.h>

static void setup(int input) {
}

static void fullRedraw(void) {
  fileBrowserDraw();
}

static void draw(void) {
}

static void onInput(int keys, int isDoubleTap) {
  if (fileBrowserInput(keys, isDoubleTap)) {
    fileBrowserDraw();
  }
}

const struct AppScreen screenFileBrowser = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};