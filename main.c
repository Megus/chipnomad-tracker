#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_manager.h"
#include "corelib_mainloop.h"
#include "corelib_gfx.h"
#include "common.h"

#include <psg_play.h>

void draw() {
}

void onEvent(enum MainLoopEvent event, int value, void* userdata) {

}

int main(int argv, char** args) {
  if (gfxSetup() != 0) {
    return 1;
  }

  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  audioManager.start(appSettings.audioSampleRate, appSettings.audioBufferSize, 50.0);
  audioManager.initChips();

  psgReadFile("test.psg");

  audioManager.setFrameCallback(psgFrameCallback, NULL);

  mainLoopRun(draw, onEvent);

  audioManager.stop();

  gfxCleanup();
  mainLoopQuit();

  return 0;
}
