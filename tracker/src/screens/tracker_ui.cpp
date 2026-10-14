#include <stdarg.h>
#include <string.h>
#include "chipnomad_lib.h"
#include "file_system.h"
#include "utils.h"
#include "tracker_ui.h"
#include "waveform_display.h"

#include "screen_song.h"


#define MESSAGE_TIME (60)

TrackerUI::TrackerUI(Gfx& gfx, FontManager& fontManager, AudioManager& audio, FileSystem& file, InputUtils& input, EventDispatcher& dispatcher, TrackerState& state):
  gfx(gfx),
  fontManager(fontManager),
  audio(audio),
  file(file),
  input(input),
  dispatcher(dispatcher),
  state(state)
{

  // Create all screen objects
  screenSong = new ScreenSong(state, dispatcher, gfx);

  // Initialize waveform display
  waveformDisplayInit();

  screenStack[0] = screenSong;
}

void TrackerUI::drawScreenMap() {
  const static int smY = 15;

  const ColorTheme cs = state.settings.colorTheme;
  gfx.setBgColor(cs.background);
  gfx.setFgColor(cs.textInfo);
  gfx.clearRect(35, smY, 5, 5);

  // Core screens
  gfx.print(35, smY + 1, "SCPIT");
/*
  // Additional screens
  if (currentScreen == &screenSong || currentScreen == &screenProject) {
    gfx.print(35, smY, "P");
  } else if (currentScreen == &screenPhrase || currentScreen == &screenGroove) {
    gfx.print(37, smY, "G");
  } else if (currentScreen == &screenInstrument || currentScreen == &screenInstrumentPool) {
    gfx.print(38, smY + 2, "P");
    gfx.print(38, smY, "M");
  } else if (currentScreen == &screenModulation) {
    gfx.print(38, smY + 2, "P");
  } else if (currentScreen == &screenTable || currentScreen == &screenWavetable) {
    gfx.print(39, smY + 2, "W");
  }

  // Show Settings below Song
  if (currentScreen == &screenSong) {
    gfx.print(35, smY + 2, "S");
  }

  // Highlight current screen
  gfx.setFgColor(cs.textDefault);
  if (currentScreen == &screenSong) {
    gfx.print(35, smY + 1, "S");
  } else if (currentScreen == &screenChain) {
    gfx.print(36, smY + 1, "C");
  } else if (currentScreen == &screenPhrase) {
    gfx.print(37, smY + 1, "P");
  } else if (currentScreen == &screenInstrument) {
    gfx.print(38, smY + 1, "I");
  } else if (currentScreen == &screenInstrumentPool) {
    gfx.print(38, smY + 2, "P");
  } else if (currentScreen == &screenModulation) {
    gfx.print(38, smY, "M");
  } else if (currentScreen == &screenTable) {
    gfx.print(39, smY + 1, "T");
  } else if (currentScreen == &screenWavetable) {
    gfx.print(39, smY + 2, "W");
  } else if (currentScreen == &screenProject) {
    gfx.print(35, smY, "P");
  } else if (currentScreen == &screenGroove) {
    gfx.print(37, smY, "G");
  } else if (currentScreen == &screenSettings) {
    gfx.print(35, smY + 2, "S");
  }
*/
}

bool TrackerUI::onEvent(Event event) {

  return false;
}


void TrackerUI::draw() {
  const ColorTheme cs = state.settings.colorTheme;

  // Tracks
  char digit[2] = "0";
  for (int c = 0; c < state.project.tracksCount; c++) {
    // Draw mute/solo indicator to the left of track number
    gfx.setFgColor(cs.textTitles);
    if (audio.trackStates[c] == TrackState::muted) {
      gfx.print(34, 3 + c, "M");
    } else if (audio.trackStates[c] == TrackState::solo) {
      gfx.print(34, 3 + c, "S");
    } else {
      gfx.print(34, 3 + c, " "); // Clear indicator
    }

    // Use warning color for track numbers if audio overload is active
    int useOverloadColor = (audio.engine.audioOverload > 0);
    gfx.setFgColor(useOverloadColor ? cs.warning :
      (state.songTrack == c ? cs.textDefault : cs.textInfo));
    digit[0] = c + 49;
    gfx.print(35, 3 + c, digit);

    // Draw waveform between track number and note
    gfx.setFgColor(cs.textInfo);
    Bitmap* waveformBitmap = waveformDisplayGetBitmap(c);
    if (waveformBitmap) {
      gfx.drawBitmap(waveformBitmap, 36, 3 + c);
    }

    uint8_t note = audio.engine.player.tracks[c].note.pitchFinal;
    const char* noteStr = noteName(&state.project, note);

    // Use warning color if track warning is active
    int useWarningColor = (state.settings.pitchConflictWarning && audio.engine.trackWarnings[c] > 0);

    gfx.setFgColor(useWarningColor ? cs.warning :
      (noteStr[0] == '-' ? cs.textEmpty : cs.textValue));
      gfx.print(37, 3 + c, noteStr);
  }
}



void screenSetup(const AppScreen* screen, int input) {
  pendingScreen = screen;
  pendingScreenInput = input;
}

void screenDraw() {
  if (pendingScreen != NULL) {
    currentScreen = pendingScreen;
    currentScreen->setup(pendingScreenInput);
    gfxSetBgColor(appSettings.colorScheme.background);
    gfxSetCursorColor(appSettings.colorScheme.cursor);
    gfxClearRect(0, 0, 40, 20);
    currentScreen->fullRedraw();
    drawScreenMap();

    pendingScreen = NULL;
  }

  currentScreen->draw();

  // Draw cached message
  if (strlen(messageBuffer) > 0) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxClearRect(0, 19, 40, 1);
    gfxPrint(0, 19, messageBuffer);
  }

  if (messageTimer >= 0) {
    messageTimer--;
    if (messageTimer == 0) {
      messageBuffer[0] = '\0';
      gfxClearRect(0, 19, 40, 1);
    }
  }
}


void screenMessage(int time, const char* format, ...) {
  // Don't clear timed messages
  if (messageTimer > 0 && strlen(format) == 0) {
    return;
  }

  messageTimer = time;

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  // Clear message area immediately if setting empty message
  if (strlen(messageBuffer) == 0) {
    gfxClearRect(0, 19, 40, 1);
  }
}


bool TrackerUI::onEvent(Event event) {

/*
  // On keyUp and no buttons - clear the message
    if (pressedButtons == 0) {
      // Clean untimed screen message when all keys are released
      screenMessage(0, "");
    }
*/

/*
  // Full redraw event
    gfxSetBgColor(appSettings.colorScheme.background);
    gfxClear();
    if (currentScreen) {
      currentScreen->fullRedraw();
      drawScreenMap();
    }
*/

  return false;
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



static void applyLoopRange(void) {
  chipnomad::LoopRange range = screenGetLoopRange(currentScreen);
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


void clearNotePreview(void) {
  // Clear the note preview area for all tracks (right side of screen)
  gfxClearRect(35, 3, 5, PROJECT_MAX_TRACKS);
}
