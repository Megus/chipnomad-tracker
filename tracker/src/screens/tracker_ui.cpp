#include <stdarg.h>
#include <string.h>
#include "chipnomad_lib.h"
#include "file_system.h"
#include "utils.h"
#include "tracker_ui.h"
#include "waveform_display.h"

#include "screen_song.h"
#include "screen_project.h"

#include "screen_confirm.h"
#include "screen_edit_character.h"

TrackerUI::TrackerUI(Gfx& gfx, FontManager& fontManager, AudioManager& audio, FileSystem& file, InputUtils& input, EventDispatcher& events, TrackerState& state):
  gfx(gfx),
  fontManager(fontManager),
  audio(audio),
  file(file),
  input(input),
  events(events),
  state(state)
{
  // Create all screen objects
  screens[ScreenEnum::song] = new ScreenSong(state, events, gfx, audio, clipboard);
  screens[ScreenEnum::project] = new ScreenProject(state, events, gfx, audio);

  screens[ScreenEnum::confirm] = new ScreenConfirm(state, events, gfx);
  screens[ScreenEnum::editCharacter] = new ScreenEditCharacter(state, events, gfx);

  // Remaining initialization
  waveDisplay = new WaveformDisplay(gfx);

  events.dispatch(Event::switchScreen(ScreenEnum::song));
}

TrackerUI::~TrackerUI() {
  for (const auto& s : screens) {
    delete s.second;
  }
}

Screen* TrackerUI::getCurrentScreen() {
  return screenStackIdx >= 0 ? screenStack[screenStackIdx] : nullptr;
}

void TrackerUI::drawScreenMap() {
  const static int smY = 15;

  // Core screens
  const ColorTheme t = state.settings.colorTheme;
  gfx.setBgColor(t.background);
  gfx.setFgColor(t.textInfo);
  gfx.clearRect(35, smY, 5, 5);
  gfx.print(35, smY + 1, "SCPIT");

  Screen* currentScreen = getCurrentScreen();
  if (currentScreen == nullptr) return;

  ScreenEnum s = ScreenEnum::song;
  for (const auto& [k, v] : screens) {
    if (v == currentScreen) {
      s = k;
      break;
    }
  }

  // Additional screens
  if (s == ScreenEnum::song || s == ScreenEnum::project) {
    gfx.print(35, smY, "P");
    gfx.print(35, smY + 2, "S");
  } else if (s == ScreenEnum::phrase || s == ScreenEnum::groove) {
    gfx.print(37, smY, "G");
  } else if (s == ScreenEnum::instrument || s == ScreenEnum::instrumentPool) {
    gfx.print(38, smY, "M");
    gfx.print(38, smY + 2, "P");
  } else if (s == ScreenEnum::modulation) {
    gfx.print(38, smY + 2, "P");
  } else if (s == ScreenEnum::table || s == ScreenEnum::ayWavetable) {
    gfx.print(39, smY + 2, "W");
  }

  // Highlight current screen
  gfx.setFgColor(t.textDefault);
  if (s == ScreenEnum::song) {
    gfx.print(35, smY + 1, "S");
  } else if (s == ScreenEnum::chain) {
    gfx.print(36, smY + 1, "C");
  } else if (s == ScreenEnum::phrase) {
    gfx.print(37, smY + 1, "P");
  } else if (s == ScreenEnum::instrument) {
    gfx.print(38, smY + 1, "I");
  } else if (s == ScreenEnum::instrumentPool) {
    gfx.print(38, smY + 2, "P");
  } else if (s == ScreenEnum::modulation) {
    gfx.print(38, smY, "M");
  } else if (s == ScreenEnum::table) {
    gfx.print(39, smY + 1, "T");
  } else if (s == ScreenEnum::ayWavetable) {
    gfx.print(39, smY + 2, "W");
  } else if (s == ScreenEnum::project) {
    gfx.print(35, smY, "P");
  } else if (s == ScreenEnum::groove) {
    gfx.print(37, smY, "G");
  } else if (s == ScreenEnum::settings) {
    gfx.print(35, smY + 2, "S");
  }
}

void TrackerUI::draw() {
  const ColorTheme t = state.settings.colorTheme;

  /////////////////////////////////////////////////////////////////////////////
  // Tracks playback preview

  char digit[2] = "0";
  for (int c = 0; c < state.project->tracksCount; c++) {
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
    const char* noteStr = state.project->noteName(note);

    // Use warning color if track warning is active
    int useWarningColor = (state.settings.pitchConflictWarning && audio.engine.trackWarnings[c] > 0);

    gfx.setFgColor(useWarningColor ? t.warning :
      (noteStr[0] == '-' ? t.textEmpty : t.textValue));
      gfx.print(37, 3 + c, noteStr);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Screen draw
  Screen* screen = getCurrentScreen();
  if (screen) screen->draw();

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

void TrackerUI::redrawScreen() {
  gfx.setBgColor(state.settings.colorTheme.background);
  gfx.setCursorColor(state.settings.colorTheme.cursor);
  gfx.clear();
  getCurrentScreen()->fullRedraw();
  drawScreenMap();
}

bool TrackerUI::onEvent(const Event& e) {
  bool handled = false;

  switch (e.type) {
    case EventType::input:
      handled = onInput(e);
      break;
    case EventType::playSong:
    case EventType::playChain:
    case EventType::playPhrase:
      handled = onPlay(e);
      break;
    case EventType::playPhraseRow:
      // TODO: Implent play phrase row
      break;
    case EventType::stopPlayback:
      audio.engine.player.stop();
      handled = true;
      break;
    case EventType::redrawScreen:
      redrawScreen();
      break;
    case EventType::switchScreen:
    case EventType::pushScreen:
    case EventType::popScreen:
      handled = onScreenEvent(e);
      break;
    case EventType::showMessage:
      onShowMessage(e);
      handled = true;
      break;
    default:
      break;
  }

  // If the event wasn't handled by the UI itself, forward it to the topmost screen
  if (!handled) handled = getCurrentScreen()->onEvent(e);

  return handled;
}

bool TrackerUI::onInput(const Event& e) {
  using namespace chipnomad;

  // All keys are released
  if (!e.isKeyDown && e.keys == 0) {
    // Clear instant screen message
    events.dispatch(Event::showInstantMessage(""));

    // Stop phrase row playback
    if (audio.engine.player.tracks[state.songTrack].mode == PlaybackMode::phraseRow) {
      events.dispatchInstant(Event::stopPlayback());
    }
  }

  // Let the current screen handle input first and then handle global playback keys
  bool handled = getCurrentScreen()->onEvent(e);
  if (handled || !e.isKeyDown) return true;

  ScreenPlaybackLevel playbackLevel = getCurrentScreen()->getPlaybackLevel();
  LoopRange range = getCurrentScreen()->getLoopRange();
  Player& player = audio.engine.player;
  int isPlaying = player.isPlaying();

  // Stop playback
  if (isPlaying && e.keys == keyPlay) {
      events.dispatchInstant(Event::stopPlayback());
    return true;
  }

  // This screen doesn't support playback
  if (playbackLevel == ScreenPlaybackLevel::none) return true;

  // Play song/chain/phrase depending on the screen's playback level
  if (!isPlaying && e.keys == keyPlay) {
    if (playbackLevel == ScreenPlaybackLevel::song) {
      events.dispatch(Event::playSong(range.enabled ? range.startSongRow : state.songRow, 0, range));
    } else if (playbackLevel == ScreenPlaybackLevel::chain) {
      events.dispatch(Event::playChain(state.songTrack, state.songRow, range.enabled ? range.startChainRow : state.chainRow, range));
    } else if (playbackLevel == ScreenPlaybackLevel::phrase) {
      events.dispatch(Event::playPhrase(state.songTrack, state.songRow, state.chainRow, range));
    }
  }
  // Play song from music screens (Shift+Play)
  else if (!isPlaying && e.keys == (keyPlay | keyShift)) {
    if (playbackLevel == ScreenPlaybackLevel::song) {
      events.dispatch(Event::playSong(range.enabled ? range.startSongRow : state.songRow, 0, range));
    } else if (playbackLevel == ScreenPlaybackLevel::chain || playbackLevel == ScreenPlaybackLevel::phrase) {
      events.dispatch(Event::playSong(state.songRow, range.enabled ? range.startChainRow : state.chainRow, range));
    }
  }
  return true;
}

void TrackerUI::onShowMessage(const Event& e) {
  if (messageTimer > 0 && strlen(e.message) == 0) return; // Don't clear timed messages
  gfx.clearRect(0, 19, 40, 1);
  messageTimer = e.isTimedMessage ? timedMessageFrames : 0;
  strncpy(messageBuffer, e.message, sizeof(messageBuffer) - 1);
  messageBuffer[sizeof(messageBuffer) - 1] = 0;
}

bool TrackerUI::onPlay(const Event& e) {
  chipnomad::Player& player = audio.engine.player;

  if (player.isPlaying()) return true;

  if (e.type == EventType::playSong) {
    player.playSong(e.songRow, e.chainRow, true);
  } else if (e.type == EventType::playChain) {
    player.playChain(e.track, e.songRow, e.chainRow, true);
  } else if (e.type == EventType::playPhrase) {
    player.playPhrase(e.track, e.songRow, e.chainRow, true);
  } else {
    return false;
  }

  if (e.loopRange.enabled) {
    player.setLoopRange(e.loopRange);
  } else {
    player.clearLoopRange();
  }
  return true;
}

bool TrackerUI::onScreenEvent(const Event& e) {
  bool handled = false;

  switch (e.type) {
    case EventType::switchScreen: {
      Screen* screen = screens[e.screen];
      if (!screen) return true;
      screen->setup(e.screenData);
      screenStack[0] = screen;
      screenStackIdx = 0;
      redrawScreen();
      handled = true;
      break;
    }
    case EventType::pushScreen: {
      Screen* screen = screens[e.screen];
      if (!screen) return true;
      screenStackIdx++;
      screenStack[screenStackIdx] = screen;
      screen->setup(e.screenData);
      redrawScreen();
      handled = true;
      break;
    }
    case EventType::popScreen: {
      if (screenStackIdx == 0) break;
      screenStackIdx--;
      events.dispatch(Event::screenOutput(e.screen, e.screenData));
      events.dispatch(Event::redrawScreen());
      break;
    }
    default:
      break;
  }

  return handled;
}
