#include <string.h>
#include "corelib_gfx.h"
#include "common.h"
#include "audio_manager.h"
#include "app.h"
#include "screens.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "keyboard_layout.h"
#include "waveform_display.h"

// Input handling vars:

/** Currently pressed buttons */
static int pressedButtons;
/** Frame counter for double tap detection */
static int editDoubleTapCount;
/** Frame counter for key repeats */
static int keyRepeatCount;

/**
* @brief Handle play/stop key commands
*
* @param keys Pressed keys
* @param isDoubleTap is it a double tap?
* @return int 0 - input not handled, 1 - input handled
*/
static int inputPlayback(int keys, int isDoubleTap) {
  if (!chipnomadState) return 0;

  int isPlaying = playbackIsPlaying(&chipnomadState->playbackState);

  // Play song/chain/phrase depending on the current screen
  if (!isPlaying && keys == keyPlay) {
    // Stop any preview first
    playbackStop(&chipnomadState->playbackState);
    if (currentScreen == &screenSong || currentScreen == &screenProject) {
      playbackStartSong(&chipnomadState->playbackState, *pSongRow, 0, 1);
    } else if (currentScreen == &screenChain) {
      playbackStartChain(&chipnomadState->playbackState, *pSongTrack, *pSongRow, *pChainRow, 1);
    } else if (currentScreen == &screenPhrase || currentScreen == &screenTable || currentScreen == &screenInstrument) {
      playbackStartPhrase(&chipnomadState->playbackState, *pSongTrack, *pSongRow, *pChainRow, 1);
    }
    // Other screens don't support playback
    return 1;
  }
  // Play song from music screens
  else if (!isPlaying && keys == (keyPlay | keyShift)) {
    // Stop any preview first
    playbackStop(&chipnomadState->playbackState);
    if (currentScreen == &screenSong || currentScreen == &screenProject) {
      playbackStartSong(&chipnomadState->playbackState, *pSongRow, 0, 1);
    } else if (currentScreen == &screenChain || currentScreen == &screenPhrase || currentScreen == &screenTable || currentScreen == &screenInstrument) {
      playbackStartSong(&chipnomadState->playbackState, *pSongRow, *pChainRow, 1);
    }
    // Other screens don't support playback
    return 1;
  }
  // Stop playback
  else if (isPlaying && keys == keyPlay) {
    playbackStop(&chipnomadState->playbackState);
    return 1;
  }
  return 0;
}

/**
* @brief App input handler. Handles app-wide commands and then forwards the call to the current screen
*
* @param isKeyDown whether this is a key press (1) or key release (0)
* @param keys Pressed buttons
* @param isDoubleTap is it a double tap?
*/
static void appInput(int isKeyDown, int keys, int isDoubleTap) {
  int volumeChanged = 0;

  // Volume control (only on key down)
  if (isKeyDown) {
    if (keys == keyVolumeUp) {
      if (appSettings.volume < 1.0) appSettings.volume += 0.1;
      volumeChanged = 1;
    } else if (keys == keyVolumeDown) {
      if (appSettings.volume > 0.0) appSettings.volume -= 0.1;
      volumeChanged = 1;
    }
    if (volumeChanged) {
      if (appSettings.volume < 0.0) appSettings.volume = 0;
      if (appSettings.volume > 1.0) appSettings.volume = 1.0;
    }
  }

  // Stop phrase row and preview
  if (chipnomadState->playbackState.tracks[*pSongTrack].mode == playbackModePhraseRow && keys == 0) {
    playbackStop(&chipnomadState->playbackState);
  }
  // Let screen handle input first, then try global playback if not handled
  if (!currentScreen->onInput(isKeyDown, keys, isDoubleTap)) {
    if (isKeyDown) {
      inputPlayback(keys, isDoubleTap);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
//
// High-level application functions
//

/**
* @brief Initialize the application: setup audio system, load auto-saved project, show the first screen
*/
void appSetup(void) {
  // Initialize keyboard layout system
  initKeyboardLayout();
  
  // Keyboard input reset
  pressedButtons = 0;
  editDoubleTapCount = 0;
  keyRepeatCount = 0;

  // Clear screen
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  // Initialize waveform display
  waveformDisplayInit();

  // Create ChipNomad state
  chipnomadState = chipnomadCreate();
  if (!chipnomadState) {
    // Handle error - for now just exit
    return;
  }

  // Try to load an auto-saved project
  if (projectLoad(&chipnomadState->project, getAutosavePath())) {
    // Failed to load autosave, initialize empty project
    projectInitAY(&chipnomadState->project);
  }

  // Initialize all screen states
  screensInitAll();

  playbackInit(&chipnomadState->playbackState, &chipnomadState->project);

  // Set mix volume from settings
  chipnomadState->mixVolume = appSettings.mixVolume;

  // Initialize audio system
  chipnomadInitChips(chipnomadState, appSettings.audioSampleRate, NULL);
  chipnomadSetQuality(chipnomadState, appSettings.quality);
  audioManager.start(appSettings.audioSampleRate, appSettings.audioBufferSize);
  audioManager.resume();

  screenSetup(&screenSong, 0);
}

/**
* @brief Release all resources before closing the application
*/
void appCleanup(void) {
  audioManager.stop();
  chipnomadDestroy(chipnomadState);
  chipnomadState = NULL;
}

/**
* @brief Main draw function. Draws playback status
*/
void appDraw(void) {
  const ColorScheme cs = appSettings.colorScheme;

  screenDraw();

  if (!chipnomadState) return;

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < chipnomadState->project.tracksCount; c++) {
    // Draw mute/solo indicator to the left of track number
    gfxSetFgColor(cs.textTitles);
    if (audioManager.trackStates[c] == TRACK_MUTED) {
      gfxPrint(34, 3 + c, "M");
    } else if (audioManager.trackStates[c] == TRACK_SOLO) {
      gfxPrint(34, 3 + c, "S");
    } else {
      gfxPrint(34, 3 + c, " "); // Clear indicator
    }

    // Use warning color for track numbers if audio overload is active
    int useOverloadColor = (chipnomadState->audioOverload > 0);
    gfxSetFgColor(useOverloadColor ? cs.warning :
      (*pSongTrack == c ? cs.textDefault : cs.textInfo));
    digit[0] = c + 49;
    gfxPrint(35, 3 + c, digit);

    // Draw waveform between track number and note
    gfxSetFgColor(cs.textInfo);
    uint8_t* waveformBitmap = waveformDisplayGetBitmap(c);
    if (waveformBitmap) {
      gfxDrawCharBitmap(waveformBitmap, 36, 3 + c);
    }

    uint8_t note = chipnomadState->playbackState.tracks[c].note.noteFinal;
    char* noteStr = noteName(&chipnomadState->project, note);

    // Use warning color if track warning is active
    int useWarningColor = (appSettings.pitchConflictWarning && chipnomadState->trackWarnings[c] > 0);

    gfxSetFgColor(useWarningColor ? cs.warning :
      (noteStr[0] == '-' ? cs.textEmpty : cs.textValue));
      gfxPrint(37, 3 + c, noteStr);
  }
}

/**
* @brief Main event handler
*
* @param event Event
* @param value Event value
* @param userdata Arbitraty event data
*/
void appOnEvent(enum MainLoopEvent event, int value, void* userdata) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;
  static int cachedGamepadSwapAB = 0;

  // Update gamepad swap setting only when no keys are pressed
  if (pressedButtons == 0) {
    cachedGamepadSwapAB = appSettings.gamepadSwapAB;
  }

  // Handle gamepad A/B button swap
  if (cachedGamepadSwapAB) {
    if (value == keyEdit) {
      value = keyOpt;
    } else if (value == keyOpt) {
      value = keyEdit;
    }
  }

  switch (event) {
  case eventKeyDown:
    if (value == keyEdit || value == keyOpt || value == keyShift) {
      // Edit/Opt/Shift "override" d-pad buttons
      pressedButtons = (pressedButtons & (~dPadMask)) | value;
    } else {
      pressedButtons |= value;
    }

    // Double tap is only applicable to Edit button
    int isDoubleTap = (value == keyEdit && editDoubleTapCount > 0) ? 1 : 0;

    if (value & dPadMask) {
      // Key repeats are only applicable to d-pad
      keyRepeatCount = appSettings.keyRepeatDelay;
      // As we don't support multiple d-pad keys, keep only the last pressed one
      pressedButtons = (pressedButtons & ~dPadMask) | value;
    }
    appInput(1, pressedButtons, isDoubleTap);
    editDoubleTapCount = 0;
    break;
  case eventKeyUp:
    pressedButtons &= ~value;
    // Double tap is only applicable to Edit button
    if (value == keyEdit) editDoubleTapCount = appSettings.doubleTapFrames;

    // Call appInput on key release
    appInput(0, pressedButtons, 0);

    if (pressedButtons == 0) {
      // Clean untimed screen message when all keys are released
      screenMessage(0, "");
    }
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
          appInput(1, pressedButtons, 0);
        }
      } else {
        keyRepeatCount = 0;
      }
    }
    break;
  case eventExit:
    // Auto-save the current project on exit
    projectSave(&chipnomadState->project, getAutosavePath());
    // Save settings on exit
    settingsSave();
    break;
  }
}
