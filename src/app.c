#include <corelib_gfx.h>
#include <common.h>
#include <audio_manager.h>
#include <app.h>
#include <screens.h>
#include <project.h>

#include <psg_play.h>

AppScreen currentScreen = NULL;

// Input handling vars
static int pressedButtons;
static int editDoubleTapCount;
static int keyRepeatCount;

void redrawCommonUI() {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetBgColor(cs.background);
  gfxClearRect(35, 0, 5, 20);

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < project.tracksCount; c++) {
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

void setupScreen(const AppScreen screen, int input) {
  currentScreen = screen;
  currentScreen((struct AppEvent){appEventSetup, {.setup = {0}}});
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClearRect(0, 0, 35, 20);
  currentScreen((struct AppEvent){appEventFullRedraw});
  redrawCommonUI();
}

void appSetup(void) {
  // Keyboard input reset
  pressedButtons = 0;
  editDoubleTapCount = 0;
  keyRepeatCount = 0;

  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();
  audioManager.start(appSettings.audioSampleRate, appSettings.audioBufferSize, 50.0);
  audioManager.initChips();
  //audioManager.setFrameCallback(psgFrameCallback, NULL);
  projectInit();
  setupScreen(screenSong, 0);
}

void appCleanup(void) {
  audioManager.stop();
}

void appDraw(void) {
  currentScreen((struct AppEvent){appEventDraw});
}

void appOnEvent(enum MainLoopEvent event, int value, void* userdata) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;

  switch (event) {
    case eventKeyDown:
      pressedButtons |= value;
      // Double tap is only applicable to Edit button
      int isDoubleTap = (value == keyEdit && editDoubleTapCount > 0) ? 1 : 0;
      currentScreen((struct AppEvent){appEventKey, {.key = {pressedButtons, isDoubleTap}}});
      editDoubleTapCount = 0;
      // Key repeats are only applicable to d-pad
      if (value & dPadMask) keyRepeatCount = appSettings.keyRepeatDelay;
      break;
    case eventKeyUp:
      pressedButtons &= ~value;
      // Double tap is only applicable to Edit button
      if (value == keyEdit) editDoubleTapCount = appSettings.doubleTapFrames;
      break;
    case eventTick:
      if (editDoubleTapCount > 0) editDoubleTapCount--;
      if (keyRepeatCount > 0) {
        int maskedButtons = pressedButtons & dPadMask;
        // Only one d-pad button can be pressed for key repeats
        if (maskedButtons == keyLeft || maskedButtons == keyRight || maskedButtons == keyUp || maskedButtons == keyDown) {
          keyRepeatCount--;
          if (keyRepeatCount == 0) {
            keyRepeatCount = appSettings.keyRepeatSpeed;
            currentScreen((struct AppEvent){appEventKey, {.key = {pressedButtons, 0}}});
          }
        } else {
          keyRepeatCount = 0;
        }
      }
      break;
    case eventExit:
      break;
  }
}
