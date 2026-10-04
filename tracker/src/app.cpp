#include <string.h>
#include "assets.h"
#include "gfx.h"
#include "font_manager.h"
#include "file_system.h"
#include "tracker_state.h"
#include "audio_manager.h"
#include "app.h"
#include "screens.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "waveform_display.h"
#include "input_utils.h"

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


static void applyLoopRange(void) {
  LoopRange range = screenGetLoopRange(currentScreen);
  if (range.enabled) {
    chipnomadState->engine->player.setLoopRange(range);
  } else {
    chipnomadState->engine->player.clearLoopRange();
  }
}

/**
* @brief Handle play/stop key commands
*
* @param keys Pressed keys
* @param tapCount number of taps
* @return int 0 - input not handled, 1 - input handled
*/
static int inputPlayback(int keys, int tapCount) {
  if (!chipnomadState) return 0;

  Player& player = chipnomadState->engine->player;
  int isPlaying = player.isPlaying();
  ScreenPlaybackLevel playbackLevel = screenGetPlaybackLevel(currentScreen);

  // Play song/chain/phrase depending on the screen's playback level
  if (!isPlaying && keys == keyPlay) {
    if (playbackLevel == ScreenPlaybackLevel::none) {
      return 0; // This screen doesn't support playback
    }

    player.stop();
    LoopRange range = screenGetLoopRange(currentScreen);

    if (playbackLevel == ScreenPlaybackLevel::song) {
      int startRow = range.enabled ? range.startSongRow : *pSongRow;
      player.playSong(startRow, 0, 1);
      applyLoopRange();
    } else if (playbackLevel == ScreenPlaybackLevel::chain) {
      int startRow = range.enabled ? range.startChainRow : *pChainRow;
      player.playChain(*pSongTrack, *pSongRow, startRow, 1);
      applyLoopRange();
    } else if (playbackLevel == ScreenPlaybackLevel::phrase) {
      player.playPhrase(*pSongTrack, *pSongRow, *pChainRow, 1);
      applyLoopRange();
    }
    return 1;
  }
  // Play song from music screens (Shift+Play)
  else if (!isPlaying && keys == (keyPlay | keyShift)) {
    if (playbackLevel == ScreenPlaybackLevel::none) {
      return 0; // This screen doesn't support playback
    }

    player.stop();
    LoopRange range = screenGetLoopRange(currentScreen);

    if (playbackLevel == ScreenPlaybackLevel::song) {
      int startRow = range.enabled ? range.startSongRow : *pSongRow;
      player.playSong(startRow, 0, 1);
      applyLoopRange();
    } else if (playbackLevel == ScreenPlaybackLevel::chain || playbackLevel == ScreenPlaybackLevel::phrase) {
      int startChainRow = range.enabled ? range.startChainRow : *pChainRow;
      player.playSong(*pSongRow, startChainRow, 1);
      applyLoopRange();
    }
    return 1;
  }
  // Stop playback
  else if (isPlaying && keys == keyPlay) {
    player.stop();
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
  // Stop phrase row and preview
  if (chipnomadState->engine->player.tracks[*pSongTrack].mode == PlaybackMode::phraseRow && keys == 0) {
    chipnomadState->engine->player.stop();
  }
  // Let screen handle input first, then try global playback if not handled
  if (!currentScreen->onInput(isKeyDown, keys, tapCount)) {
    if (isKeyDown) {
      inputPlayback(keys, tapCount);
    }
  }
}


#define AUTOSAVE_INTERVAL_FRAMES (60 * 60) // 1 minute at 60 FPS

static int autosaveCounter = 0;


// Initialize the application: setup audio system, load auto-saved project, show the first screen
bool TrackerApp::setup() {
  // Copy bundled assets (mobile builds only)
  assets.copyAssets();

  // Load settings
  input.initDefaultKeyMapping(state.settings.keyMapping);
  state.settings.loadSettings(file.getSettingsPath());

  // Load custom font before gfx.setup so it uses the correct font
  if (state.settings.fontPath[0] != '\0') {
    Font* font = fontManager.load(state.settings.fontPath);
    if (font) {
      fontManager.setCurrent(font);
    } else {
      state.settings.fontPath[0] = '\0';
      fontManager.setCurrent(nullptr);
    }
  }

  if (!gfx.setup(&state.settings.screenWidth, &state.settings.screenHeight)) return false;

  // LOGD("--- ChipNomad started ---");

  // Keyboard input reset
  pressedButtons = 0;
  tapTimerCount = 0;
  tapButton = 0;
  tapCount = 0;
  keyRepeatCount = 0;

  // Clear screen
  gfx.setBgColor(state.settings.colorTheme.background);
  gfx.clear();

  // Initialize waveform display
  waveformDisplayInit();

  // Try to load an auto-saved project
  if (!projectLoad(&state.project, file.getAutosavePath())) {
    // Failed to load autosave, initialize empty project
    projectInitAY(&state.project);
  }

  // Initialize all screen states
  screensInitAll();

  // Create and configure the audio system
  audio = new AudioManager(audioDevice, state.settings.audioSampleRate, state.settings.audioBufferSize, nullptr);
  audio->engine.setProject(&state.project);

  // Set mix volume and dithering from settings
  audio->engine.mixVolume = state.settings.mixVolume;
  audio->engine.aySampleDithering = state.settings.aySampleDithering;
  audio->engine.setQuality(state.settings.quality);

  audio->resume();

  screenSetup(&screenSong, 0);

  return true;
}

// Release all resources before closing the application
void TrackerApp::teardown() {
  gfx.teardown();
  delete audio;
}

// Main draw function. Draws playback status
void TrackerApp::draw() {
  const ColorTheme cs = state.settings.colorTheme;

  screenDraw();

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < state.project.tracksCount; c++) {
    // Draw mute/solo indicator to the left of track number
    gfx.setFgColor(cs.textTitles);
    if (audio->trackStates[c] == TrackState::muted) {
      gfx.print(34, 3 + c, "M");
    } else if (audio->trackStates[c] == TrackState::solo) {
      gfx.print(34, 3 + c, "S");
    } else {
      gfx.print(34, 3 + c, " "); // Clear indicator
    }

    // Use warning color for track numbers if audio overload is active
    int useOverloadColor = (audio->engine.audioOverload > 0);
    gfx.setFgColor(useOverloadColor ? cs.warning :
      (*state.pSongTrack == c ? cs.textDefault : cs.textInfo));
    digit[0] = c + 49;
    gfx.print(35, 3 + c, digit);

    // Draw waveform between track number and note
    gfx.setFgColor(cs.textInfo);
    Bitmap* waveformBitmap = waveformDisplayGetBitmap(c);
    if (waveformBitmap) {
      gfx.drawBitmap(waveformBitmap, 36, 3 + c);
    }

    uint8_t note = audio->engine.player.tracks[c].note.pitchFinal;
    const char* noteStr = noteName(&state.project, note);

    // Use warning color if track warning is active
    int useWarningColor = (state.settings.pitchConflictWarning && audio->engine.trackWarnings[c] > 0);

    gfx.setFgColor(useWarningColor ? cs.warning :
      (noteStr[0] == '-' ? cs.textEmpty : cs.textValue));
      gfx.print(37, 3 + c, noteStr);
  }
}

// Event handler
void TrackerApp::onMainLoopEvent(MainLoopEvent event) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;
  static int doubleTapMask = keyEdit | keyOpt | keyUnmapped;

  switch (event.type) {
  case MainLoopEventType::keyDown: {
    int value = input.inputCodeToKey(event.data.input, state.settings.keyMapping);

    // Call raw input callback if set (for key mapping screen)
    if (inputRawCallback) {
      inputRawCallback(event.data.input, 1);
    }

    if (value == keyEdit || value == keyOpt || value == keyShift) {
      // Edit/Opt/Shift "override" d-pad buttons
      pressedButtons = (pressedButtons & (~dPadMask)) | value;
    } else {
      pressedButtons |= value;
    }

    // Multi-tap detection
    if (value & doubleTapMask) {
      if (value == tapButton && tapTimerCount > 0) {
        // Same button pressed again within timer - increment tap count
        tapCount++;
      } else {
        // First tap or different button - start new tap sequence
        tapButton = value;
        tapCount = 1;
      }
      tapTimerCount = appSettings.doubleTapFrames;
    } else {
      // Non-multi-tap button pressed - reset tap state
      tapButton = 0;
      tapCount = 1;
      tapTimerCount = 0;
    }

    if (value & dPadMask) {
      // Key repeats are only applicable to d-pad
      keyRepeatCount = appSettings.keyRepeatDelay;
      // As we don't support multiple d-pad keys, keep only the last pressed one
      pressedButtons = (pressedButtons & ~dPadMask) | value;
    }
    appInput(1, pressedButtons, tapCount);

    break;
  }
  case MainLoopEventType::keyUp: {
    int value = inputCodeToKey(eventData.data.input);

    // Call raw input callback if set (for key mapping screen)
    if (inputRawCallback) {
      inputRawCallback(eventData.data.input, 0);
    }

    pressedButtons &= ~value;

    appInput(0, pressedButtons, 0);

    if (pressedButtons == 0) {
      // Clean untimed screen message when all keys are released
      screenMessage(0, "");
    }

    break;
  }
  case MainLoopEventType::tick:
    // Autosave
    if (++autosaveCounter >= AUTOSAVE_INTERVAL_FRAMES) {
      autosaveCounter = 0;
      projectSave(&chipnomadState->project, getAutosavePath());
    }

    // Multi-tap timer handling
    if (tapTimerCount > 0) {
      tapTimerCount--;
      if (tapTimerCount == 0) {
        // Timer expired, reset tap count
        tapCount = 0;
        tapButton = 0;
      }
    }

    // Key repeat handling
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
  case MainLoopEventType::exit:
    // Auto-save the current project and settings on exit
    projectSave(&chipnomadState->project, getAutosavePath());
    settingsSave();
    break;
  case MainLoopEventType::sleep:
    // Pause audio when app goes to background
    audio.pause();
    if (chipnomadState) {
      // Stop playback to avoid state issues
      chipnomadState->engine->player.stop();
      // Auto-save project
      projectSave(&chipnomadState->project, getAutosavePath());
    }
    // Save settings
    settingsSave();
    break;
  case MainLoopEventType::wake:
    // Resume audio when app comes back to foreground
    audio.resume();
    break;
  case MainLoopEventType::fullRedraw:
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


void clearNotePreview(void) {
  // Clear the note preview area for all tracks (right side of screen)
  gfxClearRect(35, 3, 5, PROJECT_MAX_TRACKS);
}
