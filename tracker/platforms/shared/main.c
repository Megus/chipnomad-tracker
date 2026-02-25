#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "corelib_gfx.h"
#include "corelib_font.h"
#include "corelib_mainloop.h"
#include "app.h"
#include "common.h"

#if defined(__ANDROID__) || defined(MACOS_BUILD)
extern void fileOpsInitPlatform(void);
#endif

int main(int argv, char** args) {
#if defined(__ANDROID__) || defined(MACOS_BUILD)
  fileOpsInitPlatform();
#endif

  settingsLoad();

  // Load custom font before gfxSetup so it uses the correct font
  if (appSettings.fontPath[0] != '\0') {
    Font* font = fontLoad(appSettings.fontPath);
    if (font) {
      fontSetCurrent(font);
    } else {
      appSettings.fontPath[0] = '\0';
      fontSetCurrent(NULL);
    }
  }

  if (gfxSetup(&appSettings.screenWidth, &appSettings.screenHeight) != 0) return 1;

  appSetup();
  mainLoopRun(appDraw, appOnEvent);
  appCleanup();
  gfxCleanup();
  mainLoopQuit();

  return 0;
}

int SDL_main(int argv, char** args) {
  return main(argv, args);
}
