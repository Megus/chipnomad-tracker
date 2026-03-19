#include <string.h>
#include "corelib_gfx.h"
#include "corelib_font.h"
#include "common.h"
#include "audio_manager.h"
#include "app.h"
#include "screens.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "waveform_display.h"
#include "corelib_input.h"

// Raw input callback for key mapping screen
void (*inputRawCallback)(InputCode input, int isDown) = NULL;

// Input handling vars:

/** Currently pressed buttons */
static int pressedButtons;
/** Frame counter for tap detection */
static int tapTimerCount;
/** Button that triggered tap timer */
static int tapButton;
/** Number of taps detected */
static int tapCount;
/** Frame counter for key repeats */
static int keyRepeatCount;

/**
* @brief Convert InputCode to Key enum value
*
* @param input Input code
* @return Key value or 0 if not recognized
*/
static int inputCodeToKey(InputCode input) {
  // Logical buttons are not remappable
  if (input.deviceType == inputLogical) {
    return input.code;
  }

  // Check key mapping for keyboard and gamepad inputs
  for (int i = 0; i < 3; i++) {
    if (appSettings.keyMapping.keyUp[i].deviceType == input.deviceType && appSettings.keyMapping.keyUp[i].code == input.code) return keyUp;
    if (appSettings.keyMapping.keyDown[i].deviceType == input.deviceType && appSettings.keyMapping.keyDown[i].code == input.code) return keyDown;
    if (appSettings.keyMapping.keyLeft[i].deviceType == input.deviceType && appSettings.keyMapping.keyLeft[i].code == input.code) return keyLeft;
    if (appSettings.keyMapping.keyRight[i].deviceType == input.deviceType && appSettings.keyMapping.keyRight[i].code == input.code) return keyRight;
    if (appSettings.keyMapping.keyEdit[i].deviceType == input.deviceType && appSettings.keyMapping.keyEdit[i].code == input.code) return keyEdit;
    if (appSettings.keyMapping.keyOpt[i].deviceType == input.deviceType && appSettings.keyMapping.keyOpt[i].code == input.code) return keyOpt;
    if (appSettings.keyMapping.keyPlay[i].deviceType == input.deviceType && appSettings.keyMapping.keyPlay[i].code == input.code) return keyPlay;
    if (appSettings.keyMapping.keyShift[i].deviceType == input.deviceType && appSettings.keyMapping.keyShift[i].code == input.code) return keyShift;
  }

  // Return keyUnmapped for any input that doesn't match a mapping
  return keyUnmapped;
}

/**
* @brief Handle play/stop key commands
*
* @param keys Pressed keys
* @param tapCount number of taps
* @return int 0 - input not handled, 1 - input handled
*/
static void applyLoopRange(void) {
  LoopRange range = screenGetLoopRange(currentScreen);
  if (range.enabled) {
    playbackSetLoopRange(&chipnomadState->playbackState, range);
  } else {
    playbackClearLoopRange(&chipnomadState->playbackState);
  }
}

static int inputPlayback(int keys, int tapCount) {
  if (!chipnomadState) return 0;

  int isPlaying = playbackIsPlaying(&chipnomadState->playbackState);

  // Play song/chain/phrase depending on the current screen
  if (!isPlaying && keys == keyPlay) {
    playbackStop(&chipnomadState->playbackState);
    LoopRange range = screenGetLoopRange(currentScreen);
    if (currentScreen == &screenSong || currentScreen == &screenProject) {
      int startRow = range.enabled ? range.startSongRow : *pSongRow;
      playbackStartSong(&chipnomadState->playbackState, startRow, 0, 1);
      applyLoopRange();
    } else if (currentScreen == &screenChain) {
      int startRow = range.enabled ? range.startChainRow : *pChainRow;
      playbackStartChain(&chipnomadState->playbackState, *pSongTrack, *pSongRow, startRow, 1);
      applyLoopRange();
    } else if (currentScreen == &screenPhrase || currentScreen == &screenTable || currentScreen == &screenInstrument) {
      playbackStartPhrase(&chipnomadState->playbackState, *pSongTrack, *pSongRow, *pChainRow, 1);
      applyLoopRange();
    }
    return 1;
  }
  // Play song from music screens
  else if (!isPlaying && keys == (keyPlay | keyShift)) {
    playbackStop(&chipnomadState->playbackState);
    LoopRange range = screenGetLoopRange(currentScreen);
    if (currentScreen == &screenSong || currentScreen == &screenProject) {
      int startRow = range.enabled ? range.startSongRow : *pSongRow;
      playbackStartSong(&chipnomadState->playbackState, startRow, 0, 1);
      applyLoopRange();
    } else if (currentScreen == &screenChain || currentScreen == &screenPhrase || currentScreen == &screenTable || currentScreen == &screenInstrument) {
      int startChainRow = range.enabled ? range.startChainRow : *pChainRow;
      playbackStartSong(&chipnomadState->playbackState, *pSongRow, startChainRow, 1);
      applyLoopRange();
    }
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
* @param tapCount number of taps
*/
static void appInput(int isKeyDown, int keys, int tapCount) {
  // Volume control (only on key down)
  if (isKeyDown) {
    if (keys == keyVolumeUp) {
      if (appSettings.mixVolume < 1.0) appSettings.mixVolume += 0.1;
      if (appSettings.mixVolume > 1.0) appSettings.mixVolume = 1.0;
      if (chipnomadState) chipnomadState->mixVolume = appSettings.mixVolume;
    } else if (keys == keyVolumeDown) {
      if (appSettings.mixVolume > 0.0) appSettings.mixVolume -= 0.1;
      if (appSettings.mixVolume < 0.0) appSettings.mixVolume = 0.0;
      if (chipnomadState) chipnomadState->mixVolume = appSettings.mixVolume;
    }
  }

  // Stop phrase row and preview
  if (chipnomadState->playbackState.tracks[*pSongTrack].mode == playbackModePhraseRow && keys == 0) {
    playbackStop(&chipnomadState->playbackState);
  }
  // Let screen handle input first, then try global playback if not handled
  if (!currentScreen->onInput(isKeyDown, keys, tapCount)) {
    if (isKeyDown) {
      inputPlayback(keys, tapCount);
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
  // Initialize default key mappings if not loaded from settings
  if (appSettings.keyMapping.keyUp[0].deviceType == inputNone) {
    inputInitDefaultKeyMapping();
  }

  // Keyboard input reset
  pressedButtons = 0;
  tapTimerCount = 0;
  tapButton = 0;
  tapCount = 0;
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
void appOnEvent(MainLoopEventData eventData) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;
  static int doubleTapMask = keyEdit | keyOpt | keyUnmapped;

  switch (eventData.type) {
  case eventKeyDown: {
    int value = inputCodeToKey(eventData.data.input);
    
    // Call raw input callback if set (for key mapping screen)
    if (inputRawCallback) {
      inputRawCallback(eventData.data.input, 1);
    }
    
    if (value == keyEdit || value == keyOpt || value == keyShift) {
      // Edit/Opt/Shift "override" d-pad buttons
      pressedButtons = (pressedButtons & (~dPadMask)) | value;
    } else {
      pressedButtons |= value;
    }

    // Tap detection
    int currentTapCount = 0;
    if ((value & doubleTapMask) && value == tapButton && tapTimerCount > 0) {
      tapCount++;
      currentTapCount = tapCount;
      tapTimerCount = appSettings.doubleTapFrames; // Reset timer for next tap
    }

    if (value & dPadMask) {
      // Key repeats are only applicable to d-pad
      keyRepeatCount = appSettings.keyRepeatDelay;
      // As we don't support multiple d-pad keys, keep only the last pressed one
      pressedButtons = (pressedButtons & ~dPadMask) | value;
    }
    appInput(1, pressedButtons, currentTapCount);
    break;
  }
  case eventKeyUp: {
    int value = inputCodeToKey(eventData.data.input);
    
    // Call raw input callback if set (for key mapping screen)
    if (inputRawCallback) {
      inputRawCallback(eventData.data.input, 0);
    }
    
    pressedButtons &= ~value;
    // Set tap timer
    if (value & doubleTapMask) {
      if (value == tapButton && tapTimerCount > 0) {
        // Same button released again within timer - keep current count
      } else {
        // First tap or different button
        tapButton = value;
        tapCount = 1;
      }
      tapTimerCount = appSettings.doubleTapFrames;
    }

    // Call appInput on key release
    appInput(0, pressedButtons, 0);

    if (pressedButtons == 0) {
      // Clean untimed screen message when all keys are released
      screenMessage(0, "");
    }
    break;
  }
  case eventTick:
    if (tapTimerCount > 0) {
      tapTimerCount--;
      if (tapTimerCount == 0) {
        // Timer expired, reset tap count
        tapCount = 0;
        tapButton = 0;
      }
    }
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
  case eventSleep:
    // Pause audio when app goes to background
    audioManager.pause();
    if (chipnomadState) {
      // Stop playback to avoid state issues
      playbackStop(&chipnomadState->playbackState);
      // Auto-save project
      projectSave(&chipnomadState->project, getAutosavePath());
    }
    // Save settings
    settingsSave();
    break;
  case eventWake:
    // Resume audio when app comes back to foreground
    audioManager.resume();
    break;
  case eventFullRedraw:
    // Force full screen redraw
    gfxSetBgColor(appSettings.colorScheme.background);
    gfxClear();
    if (currentScreen) {
      currentScreen->fullRedraw();
      drawScreenMap();
    }
    break;
  }
}
