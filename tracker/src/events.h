#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <string>
#include "input_utils.h"
#include "chipnomad_lib.h"

enum class EventType {
  none,
  input,
  rawInput,
  playSong,
  playChain,
  playPhrase,
  playPhraseRow,
  stopPlayback,
  redrawScreen,
  switchScreen,
  pushScreen,
  popScreen,
  showMessage,
};

enum class ScreenEnum : int {
  song,
  project,
  pitchTable,
  projectExport,
  manage,
  settings,
  colorTheme,
  keyMapping,
  chain,
  phrase,
  groove,
  instrument,
  instrumentPool,
  modulation,
  table,
  ayWavetable,
  confirm,
  fileBrowser,
  createFolder,
  charEntry,
};

// Base struct for screen input and output. Most screens only need an int value
struct ScreenData {
  ScreenData(): intValue(0) {};
  ScreenData(int intValue): intValue(intValue) {};
  virtual ~ScreenData() = default;
  int intValue;
};

// Base event struct
struct Event {
  Event(): type(EventType::none) {};
  Event(EventType type): type(type) {};
  virtual ~Event() = default;
  EventType type;
};

struct InputEvent : Event {
  InputEvent(bool isKeyDown, int keys, int tapCount): Event(EventType::input), isKeyDown(isKeyDown), keys(keys), tapCount(tapCount) {};
  bool isKeyDown;
  int keys;
  int tapCount;
};

struct RawInputEvent : Event {
  RawInputEvent(bool isKeyDown, InputCode keyCode): Event(EventType::rawInput), isKeyDown(isKeyDown), keyCode(keyCode) {};
  bool isKeyDown;
  InputCode keyCode;
};

struct PlayEvent : Event {
  PlayEvent(EventType type, chipnomad::LoopRange loopRange, int track, int songRow, int chainRow):
    Event(type), loopRange(loopRange), track(track), songRow(songRow), chainRow(chainRow) {};
  chipnomad::LoopRange loopRange;
  int track;
  int songRow;
  int chainRow;
};

struct ScreenEvent : Event {
  ScreenEvent(EventType type, ScreenEnum screen, ScreenData data): Event(type), screen(screen), data(data) {};
  ScreenEnum screen;
  ScreenData data;
};


struct MessageEvent : Event {
  MessageEvent(bool isTimed, std::string message): Event(EventType::showMessage), isTimed(isTimed), message(message) {};
  bool isTimed;
  std::string message;
};


class EventHandler {
  public:
    virtual bool onEvent(const Event& e) = 0;
};

class EventDispatcher {
  public:
    virtual void dispatch(Event e) = 0;

    // Convenience functions for all events

    void input(bool isKeyDown, int keys, int tapCount);
    void rawInput(bool isKeyDown, const InputCode& keyCode);
    void playSong(int songRow, int chainRow);
    void playSong(int songRow, int chainRow, const chipnomad::LoopRange& loopRange);
    void playChain(int track, int songRow, int chainRow);
    void playChain(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange);
    void playPhrase(int track, int songRow, int chainRow);
    void playPhrase(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange);
    // void playPhraseRow();
    void stopPlayback();
    void redrawScreen();
    void switchScreen(ScreenEnum screen);
    void switchScreen(ScreenEnum screen, const ScreenData& input);
    void pushScreen(ScreenEnum screen, const ScreenData& input);
    void popScreen(ScreenEnum screen, const ScreenData& output);
    void showInstantMessage(const char* format, ...);
    void showTimedMessage(const char* format, ...);
};

#endif // __EVENTS_H__