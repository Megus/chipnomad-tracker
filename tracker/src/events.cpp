#include "events.h"

void EventDispatcher::input(bool isKeyDown, int keys, int tapCount) {
  dispatch(std::unique_ptr<Event>(new InputEvent(isKeyDown, keys, tapCount)));
}

void EventDispatcher::rawInput(bool isKeyDown, const InputCode& keyCode) {
  dispatch(std::unique_ptr<Event>(new RawInputEvent(isKeyDown, keyCode)));
}

void EventDispatcher::playSong(int songRow, int chainRow) {
  dispatch(std::unique_ptr<Event>(new PlayEvent(EventType::playSong, {.enabled = false}, 0, songRow, chainRow)));
}

void EventDispatcher::playSong(int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  dispatch(std::unique_ptr<Event>(new PlayEvent(EventType::playSong, loopRange, 0, songRow, chainRow)));
}

void EventDispatcher::playChain(int track, int songRow, int chainRow) {
  dispatch(std::unique_ptr<Event>(new PlayEvent(EventType::playChain, {.enabled = false}, 0, songRow, chainRow)));
}

void EventDispatcher::playChain(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  dispatch(std::unique_ptr<Event>(new PlayEvent(EventType::playChain, loopRange, 0, songRow, chainRow)));
}

void EventDispatcher::playPhrase(int track, int songRow, int chainRow) {
  dispatch(std::unique_ptr<Event>(new PlayEvent(EventType::playPhrase, {.enabled = false}, 0, songRow, chainRow)));
}

void EventDispatcher::playPhrase(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  dispatch(std::unique_ptr<Event>(new PlayEvent(EventType::playPhrase, loopRange, 0, songRow, chainRow)));
}

void EventDispatcher::stopPlayback() {
  dispatchInstant(std::unique_ptr<Event>(new Event(EventType::stopPlayback)));
}

void EventDispatcher::redrawScreen() {
  dispatch(std::unique_ptr<Event>(new Event(EventType::redrawScreen)));
}

void EventDispatcher::switchScreen(ScreenEnum screen) {
  dispatch(std::unique_ptr<Event>(new ScreenEvent(EventType::switchScreen, screen, ScreenData())));
}

void EventDispatcher::switchScreen(ScreenEnum screen, const ScreenData& input) {
  dispatch(std::unique_ptr<Event>(new ScreenEvent(EventType::switchScreen, screen, input)));
}

void EventDispatcher::pushScreen(ScreenEnum screen) {
  dispatch(std::unique_ptr<Event>(new ScreenEvent(EventType::pushScreen, screen, ScreenData())));
}

void EventDispatcher::pushScreen(ScreenEnum screen, const ScreenData& input) {
  dispatch(std::unique_ptr<Event>(new ScreenEvent(EventType::pushScreen, screen, input)));
}

void EventDispatcher::popScreen(ScreenEnum screen, const ScreenData& output) {
  dispatch(std::unique_ptr<Event>(new ScreenEvent(EventType::popScreen, screen, output)));
  dispatch(std::unique_ptr<Event>(new ScreenEvent(EventType::screenOutput, screen, output)));
}

void EventDispatcher::showInstantMessage(const char* format, ...) {
  static char messageBuffer[41];

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  dispatch(std::unique_ptr<Event>(new MessageEvent(false, std::string(messageBuffer))));
}

void EventDispatcher::showTimedMessage(const char* format, ...) {
  static char messageBuffer[41];

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  dispatch(std::unique_ptr<Event>(new MessageEvent(true, std::string(messageBuffer))));
}
