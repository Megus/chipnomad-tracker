#include <string.h>
#include <corelib_gfx.h>
#include <common.h>
#include <audio_manager.h>
#include <app.h>
#include <screens.h>
#include <project.h>
#include <project_utils.h>
#include <playback.h>

#define TRACK_WARNING_COOLDOWN_FRAMES 5

// Input handling vars:

/** Currently pressed buttons */
static int pressedButtons;
/** Frame counter for double tap detection */
static int editDoubleTapCount;
/** Frame counter for key repeats */
static int keyRepeatCount;

// Track warning vars:
/** Cooldown counters for each track */
static int trackWarningCooldown[PROJECT_MAX_TRACKS];

/**
 * @brief Update chip registers for the next frame
 *
 * @param userdata Arbitraty user data. Not used
 */
static void frameCallback(void* userdata) {
  playbackNextFrame(&playback, &audioManager.chips[0]);
}

/**
 * @brief Handle play/stop key commands
 *
 * @param keys Pressed keys
 * @param isDoubleTap is it a double tap?
 * @return int 0 - input not handled, 1 - input handled
 */
static int inputPlayback(int keys, int isDoubleTap) {
  int isPlaying = playbackIsPlaying(&playback);

  // Stop phrase row and preview
  if (playback.tracks[*pSongTrack].mode == playbackModePhraseRow && keys == 0) {
    playbackStop(&playback);
  }
  // Play song/chain/phrase depending on the current screen
  else if (!isPlaying && keys == keyPlay) {
    // Stop any preview first
    playbackStop(&playback);
    if (currentScreen == &screenSong || currentScreen == &screenProject) {
      playbackStartSong(&playback, *pSongRow, 0, 1);
    } else if (currentScreen == &screenChain) {
      playbackStartChain(&playback, *pSongTrack, *pSongRow, *pChainRow, 1);
    } else {
      playbackStartPhrase(&playback, *pSongTrack, *pSongRow, *pChainRow, 1);
    }
    return 1;
  }
  // Play song from any screen
  else if (!isPlaying && keys == (keyPlay | keyShift)) {
    // Stop any preview first
    playbackStop(&playback);
    if (currentScreen == &screenSong || currentScreen == &screenProject) {
      playbackStartSong(&playback, *pSongRow, 0, 1);
    } else {
      playbackStartSong(&playback, *pSongRow, *pChainRow, 1);
    }
    return 1;
  }
  // Stop playback
  else if (isPlaying && keys == keyPlay) {
    playbackStop(&playback);
    return 1;
  }
  return 0;
}

/**
 * @brief App input handler. Handles app-wide commands and then forwards the call to the current screen
 *
 * @param keys Pressed buttons
 * @param isDoubleTap is it a double tap?
 */
static void appInput(int keys, int isDoubleTap) {
  int volumeChanged = 0;

  // Volume control
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

  if (inputPlayback(keys, isDoubleTap)) return;
  currentScreen->onInput(keys, isDoubleTap);
}


///////////////////////////////////////////////////////////////////////////////
//
// High-level application functions
//

/**
 * @brief Initialize the application: setup audio system, load auto-saved project, show the first screen
 */
void appSetup(void) {
  // Keyboard input reset
  pressedButtons = 0;
  editDoubleTapCount = 0;
  keyRepeatCount = 0;

  // Clear screen
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  fillFXNames();

  // Try to load an auto-saved project
  if (projectLoad(getAutosavePath())) {
    // Failed to load autosave, initialize empty project
    projectInitAY();
  }

  // Initialize audio system
  playbackInit(&playback, &project);
  audioManager.start(appSettings.audioSampleRate, appSettings.audioBufferSize, project.tickRate);
  audioManager.initChips();
  audioManager.setFrameCallback(frameCallback, NULL);
  audioManager.resume();

  screenSetup(&screenSong, 0);
}

/**
 * @brief Release all resources before closing the application
 */
void appCleanup(void) {
  audioManager.stop();
}

/**
 * @brief Main draw function. Draws playback status
 */
void appDraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  screenDraw();

  // Check for track warnings if enabled
  if (appSettings.pitchConflictWarning && audioManager.chips[0].detectWarnings) {
    audioManager.chips[0].detectWarnings(&audioManager.chips[0], trackWarningCooldown, TRACK_WARNING_COOLDOWN_FRAMES);
  }
  
  // Decrease warning cooldowns
  for (int i = 0; i < project.tracksCount; i++) {
    if (trackWarningCooldown[i] > 0) {
      trackWarningCooldown[i]--;
    }
  }

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < project.tracksCount; c++) {
    gfxSetFgColor(*pSongTrack == c ? cs.textDefault : cs.textInfo);
    digit[0] = c + 49;
    gfxPrint(35, 3 + c, digit);

    uint8_t note = playback.tracks[c].note.noteFinal;
    char* noteStr = noteName(note);

    // Use warning color if track warning is active
    int useWarningColor = (trackWarningCooldown[c] > 0);
    
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
      appInput(pressedButtons, isDoubleTap);
      editDoubleTapCount = 0;
      break;
    case eventKeyUp:
      pressedButtons &= ~value;
      // Double tap is only applicable to Edit button
      if (value == keyEdit) editDoubleTapCount = appSettings.doubleTapFrames;

      if (pressedButtons == 0) {
        // Clean untimed screen message when all keys are released
        screenMessage(0, "");
        appInput(pressedButtons, 0);
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
            appInput(pressedButtons, 0);
          }
        } else {
          keyRepeatCount = 0;
        }
      }
      break;
    case eventExit:
      // Auto-save the current project on exit
      projectSave(getAutosavePath());
      // Save settings on exit
      settingsSave();
      break;
  }
}
