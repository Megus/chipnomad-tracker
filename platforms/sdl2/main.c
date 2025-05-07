#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <corelib_gfx.h>
#include <corelib_mainloop.h>
#include <app.h>

int main(int argv, char** args) {
  if (gfxSetup() != 0) return 1;

  appSetup();
  mainLoopRun(appDraw, appOnEvent);
  appCleanup();
  gfxCleanup();
  mainLoopQuit();

  return 0;
}
