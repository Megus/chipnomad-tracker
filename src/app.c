#include <corelib_gfx.h>
#include <common.h>
#include <audio_manager.h>
#include <app.h>
#include <screens.h>
#include <song.h>

#include <psg_play.h>

const struct AppScreen* currentScreen = NULL;

void setupScreen(const struct AppScreen* screen, int input) {
  currentScreen = screen;
  currentScreen->setup(0); // Song screen doesn't need special inputs
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClearRect(0, 0, 35, 20);
  currentScreen->fullRedraw();
}

void redrawCommonUI() {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetBgColor(cs.background);
  gfxClearRect(35, 0, 5, 20);

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < song.tracksCount; c++) {
    gfxSetFgColor(cs.textInfo);
    digit[0] = c + 49;
    gfxPrint(35, 3 + c, digit);

    gfxSetFgColor(cs.textEmpty);
    gfxPrint(37, 3 + c, "---");
  }


  // Screen map
  gfxSetFgColor(cs.textInfo);
  gfxPrint(35, 15, "SCPIT");


}


void appSetup(void) {
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  audioManager.start(appSettings.audioSampleRate, appSettings.audioBufferSize, 50.0);
  audioManager.initChips();

  //audioManager.setFrameCallback(psgFrameCallback, NULL);

  // Initialize song
  songInit();

  // Show the first screen
  setupScreen(&screenSong, 0);
  redrawCommonUI();
}

void appCleanup(void) {
  audioManager.stop();
}

void appDraw(void) {
  currentScreen->draw();
}

void appOnEvent(enum MainLoopEvent event, int value, void* userdata) {
  //currentScreen->onEvent(event, value,  userdata);
}
