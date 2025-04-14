#include <corelib_gfx.h>
#include <common.h>
#include <audio_manager.h>
#include <app.h>
#include <screens.h>
#include <project.h>

// Input handling vars
static int pressedButtons;
static int editDoubleTapCount;
static int keyRepeatCount;

void appSetup(void) {
  // Keyboard input reset
  pressedButtons = 0;
  editDoubleTapCount = 0;
  keyRepeatCount = 0;

  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  // Try to load an auto-saved project
  if (projectLoad(AUTOSAVE_FILENAME)) {
    projectInit(&project);
  }

  audioManager.start(appSettings.audioSampleRate, appSettings.audioBufferSize, 50.0);
  audioManager.initChips();
  //audioManager.setFrameCallback(psgFrameCallback, NULL);
  screenSetup(&screenSong, 0);
}

void appCleanup(void) {
  audioManager.stop();
}

void appDraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  currentScreen->draw();

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < project.tracksCount; c++) {
    gfxSetFgColor(cs.textInfo);
    digit[0] = c + 49;
    gfxPrint(35, 3 + c, digit);

    gfxSetFgColor(cs.textEmpty);
    gfxPrint(37, 3 + c, "---");
  }
}

void appOnEvent(enum MainLoopEvent event, int value, void* userdata) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;

  switch (event) {
    case eventKeyDown:
      pressedButtons |= value;
      // Double tap is only applicable to Edit button
      int isDoubleTap = (value == keyEdit && editDoubleTapCount > 0) ? 1 : 0;
      currentScreen->onInput(pressedButtons, isDoubleTap);
      editDoubleTapCount = 0;
      // Key repeats are only applicable to d-pad
      if (value & dPadMask) keyRepeatCount = appSettings.keyRepeatDelay;
      break;
    case eventKeyUp:
      pressedButtons &= ~value;
      // Double tap is only applicable to Edit button
      if (value == keyEdit) editDoubleTapCount = appSettings.doubleTapFrames;
      // Clean screen message when nothing is pressed. Consider clearing it on any key up
      if (pressedButtons == 0) screenMessage("");
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
            currentScreen->onInput(pressedButtons, 0);
          }
        } else {
          keyRepeatCount = 0;
        }
      }
      break;
    case eventExit:
      // Auto-save the current project on exit
      projectSave(AUTOSAVE_FILENAME);
      break;
  }
}
