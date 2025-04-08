#include <stdarg.h>
#include <screens.h>
#include <project.h>
#include <corelib_gfx.h>


AppScreen currentScreen = NULL;

void drawScreenMap() {
  const static int smY = 15;

  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetBgColor(cs.background);
  gfxSetFgColor(cs.textInfo);
  gfxClearRect(35, smY, 5, 4);

  // Core screens
  gfxPrint(35, smY + 1, "SCPIT");

  // Additional screens
  if (currentScreen == screenSong || currentScreen == screenProject) {
    gfxPrint(35, smY, "P");
  } else if (currentScreen == screenPhrase || currentScreen == screenGroove) {
    gfxPrint(37, smY, "G");
  }

  // Highlight current screen
  gfxSetFgColor(cs.textDefault);
  if (currentScreen == screenSong) {
    gfxPrint(35, smY + 1, "S");
  } else if (currentScreen == screenChain) {
    gfxPrint(36, smY + 1, "C");
  } else if (currentScreen == screenPhrase) {
    gfxPrint(37, smY + 1, "P");
  } else if (currentScreen == screenInstrument) {
    gfxPrint(38, smY + 1, "I");
  } else if (currentScreen == screenTable) {
    gfxPrint(39, smY + 1, "T");
  } else if (currentScreen == screenProject) {
    gfxPrint(35, smY, "P");
  } else if (currentScreen == screenGroove) {
    gfxPrint(37, smY, "G");
  }
}

void setupScreen(const AppScreen screen, int input) {
  currentScreen = screen;
  currentScreen((struct AppEvent){appEventSetup, {.setup = {input}}});
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxSetSelectionColor(appSettings.colorScheme.selection);
  gfxSetCursorColor(appSettings.colorScheme.cursor);
  gfxClearRect(0, 0, 40, 20);
  currentScreen((struct AppEvent){appEventFullRedraw});
  drawScreenMap();
}

void screenMessage(const char* format, ...) {
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxClearRect(0, 19, 40, 1);

  va_list args;
  va_start(args, format);
  gfxPrintf(0, 19, format, args);
  va_end(args);
}
