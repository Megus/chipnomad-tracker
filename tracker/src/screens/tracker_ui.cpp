#include <stdarg.h>
#include <string.h>
#include "chipnomad_lib.h"
#include "file_system.h"
#include "utils.h"
#include "tracker_ui.h"
#include "waveform_display.h"

#include "screen_song.h"

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

  // Remaining initialization
  waveDisplay = new WaveformDisplay(gfx);
  screenStack[0] = screenSong;
}

TrackerUI::~TrackerUI() {
  delete screenSong;
}

Screen* TrackerUI::getCurrentScreen() {
  return screenStack[screenStackIdx];
}

void TrackerUI::drawScreenMap() {
  const static int smY = 15;

  const ColorTheme t = state.settings.colorTheme;
  gfx.setBgColor(t.background);
  gfx.setFgColor(t.textInfo);
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
  gfx.setFgColor(t.textDefault);
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
void TrackerUI::draw() {
  const ColorTheme t = state.settings.colorTheme;

  /////////////////////////////////////////////////////////////////////////////
  // Tracks playback preview

  char digit[2] = "0";
  for (int c = 0; c < state.project.tracksCount; c++) {
    // Draw mute/solo indicator to the left of track number
    gfx.setFgColor(t.textTitles);
    if (audio.trackStates[c] == TrackState::muted) {
      gfx.print(34, 3 + c, "M");
    } else if (audio.trackStates[c] == TrackState::solo) {
      gfx.print(34, 3 + c, "S");
    } else {
      gfx.print(34, 3 + c, " ");
    }

    // Use warning color for track numbers if audio overload is active
    int useOverloadColor = (audio.engine.audioOverload > 0);
    gfx.setFgColor(useOverloadColor ? t.warning : (state.songTrack == c ? t.textDefault : t.textInfo));
    digit[0] = c + 49;
    gfx.print(35, 3 + c, digit);

    // Draw waveform between track number and note
    gfx.setFgColor(t.textInfo);
    Bitmap* waveformBitmap = waveDisplay->getWaveformBitmap(audio.engine, c);
    if (waveformBitmap) {
      gfx.drawBitmap(waveformBitmap, 36, 3 + c);
    }

    uint8_t note = audio.engine.player.tracks[c].note.pitchFinal;
    const char* noteStr = noteName(&state.project, note);

    // Use warning color if track warning is active
    int useWarningColor = (state.settings.pitchConflictWarning && audio.engine.trackWarnings[c] > 0);

    gfx.setFgColor(useWarningColor ? t.warning :
      (noteStr[0] == '-' ? t.textEmpty : t.textValue));
      gfx.print(37, 3 + c, noteStr);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Screen draw
  getCurrentScreen()->draw();

  /////////////////////////////////////////////////////////////////////////////
  // Message at the bottom line
  if (strlen(messageBuffer) > 0) {
    gfx.setFgColor(t.textDefault);
    gfx.clearRect(0, 19, 40, 1);
    gfx.print(0, 19, messageBuffer);
  }

  if (messageTimer >= 0) {
    messageTimer--;
    if (messageTimer == 0) {
      messageBuffer[0] = '\0';
      gfx.clearRect(0, 19, 40, 1);
    }
  }
}

bool TrackerUI::onEvent(Event e) {
  bool handled = false;

  switch (e.type) {
    case EventType::input:
      handled = onInput(e);
      break;
    case EventType::playSong:
      handled = onPlay(e);
      break;
    case EventType::playChain:
      handled = onPlay(e);
      break;
    case EventType::playPhrase:
      handled = onPlay(e);
      break;
    case EventType::playPhraseRow:
      break;
    case EventType::stopPlayback:
      audio.engine.player.stop();
      handled = true;
      break;
    case EventType::redrawScreen:
/*
  // Full redraw event
    gfxSetBgColor(appSettings.colorScheme.background);
    gfxClear();
    if (currentScreen) {
      currentScreen->fullRedraw();
      drawScreenMap();
    }
*/
      break;
    case EventType::switchScreen:
      break;
    case EventType::pushScreen:
      break;
    case EventType::popScreen:
      break;
    case EventType::showMessage:
      onShowMessage(e, true);
      handled = true;
      break;
    case EventType::showTimedMessage:
      onShowMessage(e, false);
      handled = false;
      break;
    default:
      break;
  }

  // If the event wasn't handled by the UI itself, forward it to the topmost screen
  if (!handled) handled = getCurrentScreen()->onEvent(e);

  return handled;
}

bool TrackerUI::onInput(Event e) {
  using namespace chipnomad;

  // All keys are released
  if (!e.input.isKeyDown && e.input.keys == 0) {
    // Clear immediate screen message
    dispatcher.dispatch({.type = EventType::showMessage, .stringValue = ""});

    // Stop phrase row playback
    if (audio.engine.player.tracks[state.songTrack].mode == PlaybackMode::phraseRow) {
      dispatcher.dispatch({.type = EventType::stopPlayback});
    }
  }

  // Let the current screen handle input first and then handle global playback keys
  bool handled = getCurrentScreen()->onEvent(e);
  if (handled || !e.input.isKeyDown) return true;

  ScreenPlaybackLevel playbackLevel = getCurrentScreen()->getPlaybackLevel();
  LoopRange range = getCurrentScreen()->getLoopRange();
  Player& player = audio.engine.player;
  int isPlaying = player.isPlaying();

  // Stop playback
  if (isPlaying && e.input.keys == keyPlay) {
    dispatcher.dispatch({.type = EventType::stopPlayback});
    return true;
  }

  // This screen doesn't support playback
  if (playbackLevel == ScreenPlaybackLevel::none) return true;

  // Play song/chain/phrase depending on the screen's playback level
  if (!isPlaying && e.input.keys == keyPlay) {
    if (playbackLevel == ScreenPlaybackLevel::song) {
      dispatcher.dispatch({
        .type = EventType::playSong,
        .play = {.loopRange = range, .songRow = range.enabled ? range.startSongRow : state.songRow, .chainRow = 0}
      });
    } else if (playbackLevel == ScreenPlaybackLevel::chain) {
      dispatcher.dispatch({
        .type = EventType::playChain,
        .play = {.loopRange = range, .track = state.songTrack, .songRow = state.songRow, .chainRow = range.enabled ? range.startChainRow : state.chainRow}
      });
    } else if (playbackLevel == ScreenPlaybackLevel::phrase) {
      dispatcher.dispatch({
        .type = EventType::playPhrase,
        .play = {.loopRange = range, .track = state.songTrack, .songRow = state.songRow, .chainRow = state.chainRow}
      });
    }
  }
  // Play song from music screens (Shift+Play)
  else if (!isPlaying && e.input.keys == (keyPlay | keyShift)) {
    if (playbackLevel == ScreenPlaybackLevel::song) {
      dispatcher.dispatch({
        .type = EventType::playSong,
        .play = {.loopRange = range, .songRow = range.enabled ? range.startSongRow : state.songRow, .chainRow = 0}
      });
    } else if (playbackLevel == ScreenPlaybackLevel::chain || playbackLevel == ScreenPlaybackLevel::phrase) {
      dispatcher.dispatch({
        .type = EventType::playSong,
        .play = {.loopRange = range, .songRow = state.songRow, .chainRow = range.enabled ? range.startChainRow : state.chainRow}
      });
    }
  }
  return true;
}

void TrackerUI::onShowMessage(Event e, bool isTimed) {
  if (messageTimer > 0 && e.stringValue.length() == 0) return; // Don't clear timed messages
  gfx.clearRect(0, 19, 40, 1);
  messageTimer = isTimed ? timedMessageFrames : 0;
  strncpy(messageBuffer, e.stringValue.c_str(), sizeof(messageBuffer) - 1);
  messageBuffer[sizeof(messageBuffer) - 1] = 0;
}

bool TrackerUI::onPlay(Event e) {
  chipnomad::Player& player = audio.engine.player;

  if (player.isPlaying()) return true;

  if (e.type == EventType::playSong) {
    player.playSong(e.play.songRow, e.play.chainRow, true);
  } else if (e.type == EventType::playChain) {
    player.playChain(e.play.track, e.play.songRow, e.play.chainRow, true);
  } else if (e.type == EventType::playPhrase) {
    player.playPhrase(e.play.track, e.play.songRow, e.play.chainRow, true);
  } else {
    return false;
  }

  if (e.play.loopRange.enabled) {
    player.setLoopRange(e.play.loopRange);
  } else {
    player.clearLoopRange();
  }
  return true;
}

/*
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
}
*/