#include <string.h>
#include "app.h"
#include "tracker_ui.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "input_utils.h"

constexpr int kAutosaveInterval = 60 * 60; // 1 minute at 60 FPS

TrackerApp::TrackerApp(Gfx& gfx, FontManager& fontManager, AudioDevice& audio, FileSystem& file, Assets& assets, InputUtils& inputUtils):
  gfx(gfx),
  fontManager(fontManager),
  audioDevice(audio),
  file(file),
  assets(assets),
  input(inputUtils),
  state() {
};

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

  // Try to load an auto-saved project
  if (!projectLoad(&state.project, file.getAutosavePath())) {
    // Failed to load autosave, initialize empty project
    projectInitAY(&state.project);
  }

  // Create and configure the audio system
  audio = new AudioManager(audioDevice, state.settings.audioSampleRate, state.settings.audioBufferSize, nullptr);
  audio->engine.setProject(&state.project);

  // Set mix volume and dithering from settings
  audio->engine.mixVolume = state.settings.mixVolume;
  audio->engine.aySampleDithering = state.settings.aySampleDithering;
  audio->engine.setQuality(state.settings.quality);

  audio->resume();

  ui = new TrackerUI(gfx, fontManager, *audio, file, input, *this, state);

  return true;
}

// Release all resources before closing the application
void TrackerApp::teardown() {
  gfx.teardown();
  delete ui;
  delete audio;
}

// Main draw function. Draws playback status
void TrackerApp::draw() {
  ui->draw();
}

// Event handler
void TrackerApp::onMainLoopEvent(MainLoopEvent event) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;
  static int doubleTapMask = keyEdit | keyOpt | keyUnmapped;

  switch (event.type) {
  case MainLoopEventType::tick: {
    // Autosave
    if (++autosaveCounter >= kAutosaveInterval) {
      autosaveCounter = 0;
      chipnomad::projectSave(&state.project, file.getAutosavePath());
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
          keyRepeatCount = state.settings.keyRepeatSpeed;
          dispatch({.type = EventType::input, .input = {.isKeyDown = true, .keys = pressedButtons, .tapCount = 0}});
        }
      } else {
        keyRepeatCount = 0;
      }
    }
    break;

    // Send all queued events. Currently TrackerUI is the only receiver
    for (int c = 0; c < eventQueueLength; c++) {
      ui->onEvent(eventQueue[c]);
    }
    eventQueueLength = 0;
  }
  case MainLoopEventType::keyDown: {
    int value = input.inputCodeToKey(event.data.input, state.settings.keyMapping);

    dispatch({.type = EventType::rawInput, .rawInput = {.keyCode = event.data.input, .isKeyDown = true}});

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
      tapTimerCount = state.settings.doubleTapFrames;
    } else {
      // Non-multi-tap button pressed - reset tap state
      tapButton = 0;
      tapCount = 1;
      tapTimerCount = 0;
    }

    if (value & dPadMask) {
      // Key repeats are only applicable to d-pad
      keyRepeatCount = state.settings.keyRepeatDelay;
      // As we don't support multiple d-pad keys, keep only the last pressed one
      pressedButtons = (pressedButtons & ~dPadMask) | value;
    }
    dispatch({.type = EventType::input, .input = {.isKeyDown = true, .keys = pressedButtons, .tapCount = tapCount}});

    break;
  }
  case MainLoopEventType::keyUp: {
    int value = input.inputCodeToKey(event.data.input, state.settings.keyMapping);

    dispatch({.type = EventType::rawInput, .rawInput = {.keyCode = event.data.input, .isKeyDown = false}});

    pressedButtons &= ~value;

    dispatch({.type = EventType::input, .input = {.isKeyDown = false, .keys = pressedButtons, .tapCount = 0}});

    break;
  }
  case MainLoopEventType::exit:
    // Auto-save the current project and settings on exit
    chipnomad::projectSave(&state.project, file.getAutosavePath());
    state.settings.saveSettings(file.getSettingsPath());
    break;
  case MainLoopEventType::sleep:
    audio->pause();
    audio->engine.player.stop();
    chipnomad::projectSave(&state.project, file.getAutosavePath());
    state.settings.saveSettings(file.getSettingsPath());
    break;
  case MainLoopEventType::wake:
    // Resume audio when app comes back to foreground
    audio->resume();
    break;
  case MainLoopEventType::fullRedraw:
    dispatch({.type = EventType::redrawScreen});
    break;
  }
}

void TrackerApp::dispatch(Event event) {
  eventQueue[eventQueueLength] = event;
  eventQueueLength++;
}
