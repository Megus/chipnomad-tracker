#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "corelib_gfx.h"
#include "corelib_mainloop.h"
#include "app.h"
#include "common.h"
#include "keyboard_layout.h"

int main(int argv, char** args) {
  // Load settings
  settingsLoad();
  updateKeyboardLayout();

  if (gfxSetup(&appSettings.screenWidth, &appSettings.screenHeight) != 0) return 1;

  appSetup();
  mainLoopRun(appDraw, appOnEvent);
  appCleanup();
  gfxCleanup();
  mainLoopQuit();

  return 0;
}
