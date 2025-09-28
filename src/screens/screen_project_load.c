#include <screens.h>
#include <corelib_gfx.h>
#include <file_browser.h>
#include <project.h>

static void onFileSelected(const char* path) {
  if (projectLoad(path) == 0) {
    screenSetup(&screenProject, 0);
  }
}

static void onCancelled(void) {
  screenSetup(&screenProject, 0);
}

static void setup(int input) {
  fileBrowserSetup("LOAD PROJECT", ".cnm", onFileSelected, onCancelled);
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

const struct AppScreen screenProjectLoad = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};