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
  inputUtils(inputUtils),
  state() {
};

// Initialize the application: setup audio system, load auto-saved project, show the first screen
bool TrackerApp::setup() {
  // Copy bundled assets (mobile builds only)
  assets.copyAssets();

  // Load settings
  inputUtils.initDefaultKeyMapping(&state.settings.keyMapping);
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
  chipnomad::Project* p = chipnomad::loadProject(file.getAutosavePath());
  if (p == nullptr) {
    // Failed to load autosave, initialize empty project
    state.project = new chipnomad::Project();
    state.project->initAYproject();
  } else {
    state.project = p;
  }

  // Create and configure the audio system
  audio = new AudioManager(audioDevice, state.settings.audioSampleRate, state.settings.audioBufferSize, nullptr);
  audio->engine.setProject(state.project);

  // Set mix volume and dithering from settings
  audio->engine.mixVolume = state.settings.mixVolume;
  audio->engine.aySampleDithering = state.settings.aySampleDithering;
  audio->engine.setQuality(state.settings.quality);

  audio->resume();

  ui = new TrackerUI(gfx, fontManager, *audio, file, inputUtils, *this, state);

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
void TrackerApp::onMainLoopEvent(const MainLoopEvent& event) {
  static int dPadMask = keyLeft | keyRight | keyUp | keyDown;
  static int doubleTapMask = keyEdit | keyOpt | keyUnmapped;

  switch (event.type) {
    case MainLoopEventType::tick: {
      // Autosave
      if (++autosaveCounter >= kAutosaveInterval) {
        autosaveCounter = 0;
        chipnomad::saveProject(state.project, file.getAutosavePath());
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
            input(true, pressedButtons, 0);
          }
        } else {
          keyRepeatCount = 0;
        }
      }

      // Send all queued events. Currently TrackerUI is the only receiver
      while (!eventQueue.empty()) {
        const auto& event = eventQueue.front();
        ui->onEvent(*event);
        eventQueue.pop();
      }

      break;
    }
    case MainLoopEventType::keyDown: {
      int value = inputUtils.inputCodeToKey(event.data.input, state.settings.keyMapping);
      rawInput(true, event.data.input);

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
      input(true, pressedButtons, tapCount);

      break;
    }
    case MainLoopEventType::keyUp: {
      int value = inputUtils.inputCodeToKey(event.data.input, state.settings.keyMapping);

      rawInput(false, event.data.input);
      pressedButtons &= ~value;
      input(false, pressedButtons, 0);

      break;
    }
    case MainLoopEventType::exit: {
      // Auto-save the current project and settings on exit
      chipnomad::saveProject(state.project, file.getAutosavePath());
      state.settings.saveSettings(file.getSettingsPath());
      break;
    }
    case MainLoopEventType::sleep: {
      audio->pause();
      audio->engine.player.stop();
      chipnomad::saveProject(state.project, file.getAutosavePath());
      state.settings.saveSettings(file.getSettingsPath());
      break;
    }
    case MainLoopEventType::wake: {
      // Resume audio when app comes back to foreground
      audio->resume();
      break;
    }
    case MainLoopEventType::fullRedraw: {
      redrawScreen();
      break;
    }
  }
}

void TrackerApp::dispatch(std::unique_ptr<Event> e) {
  eventQueue.push(std::move(e));
}

void TrackerApp::dispatchInstant(std::unique_ptr<Event> e) {
  ui->onEvent(*e);
}
