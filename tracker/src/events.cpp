#include "events.h"

void EventDispatcher::input(bool isKeyDown, int keys, int tapCount) {
  dispatch(InputEvent(isKeyDown, keys, tapCount));
}

void EventDispatcher::rawInput(bool isKeyDown, const InputCode& keyCode) {
  dispatch(RawInputEvent(isKeyDown, keyCode));
}

void EventDispatcher::playSong(int songRow, int chainRow) {
  dispatch(PlayEvent(EventType::playSong, {.enabled = false}, 0, songRow, chainRow));
}

void EventDispatcher::playSong(int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  dispatch(PlayEvent(EventType::playSong, loopRange, 0, songRow, chainRow));
}

void EventDispatcher::playChain(int track, int songRow, int chainRow) {
  dispatch(PlayEvent(EventType::playChain, {.enabled = false}, 0, songRow, chainRow));
}

void EventDispatcher::playChain(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  dispatch(PlayEvent(EventType::playChain, loopRange, 0, songRow, chainRow));
}

void EventDispatcher::playPhrase(int track, int songRow, int chainRow) {
  dispatch(PlayEvent(EventType::playPhrase, {.enabled = false}, 0, songRow, chainRow));
}

void EventDispatcher::playPhrase(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  dispatch(PlayEvent(EventType::playPhrase, loopRange, 0, songRow, chainRow));
}

void EventDispatcher::stopPlayback() {
  dispatchInstant(Event(EventType::stopPlayback));
}

void EventDispatcher::redrawScreen() {
  dispatch(Event(EventType::redrawScreen));
}

void EventDispatcher::switchScreen(ScreenEnum screen) {
  dispatch(ScreenEvent(EventType::switchScreen, screen, ScreenData()));
}

void EventDispatcher::switchScreen(ScreenEnum screen, const ScreenData& input) {
  dispatch(ScreenEvent(EventType::switchScreen, screen, input));
}

void EventDispatcher::pushScreen(ScreenEnum screen) {
  dispatch(ScreenEvent(EventType::pushScreen, screen, ScreenData()));
}

void EventDispatcher::pushScreen(ScreenEnum screen, const ScreenData& input) {
  dispatch(ScreenEvent(EventType::pushScreen, screen, input));
}

void EventDispatcher::popScreen(ScreenEnum screen, const ScreenData& output) {
  dispatch(ScreenEvent(EventType::popScreen, screen, output));
  dispatch(ScreenEvent(EventType::screenOutput, screen, output));
}

void EventDispatcher::showInstantMessage(const char* format, ...) {
  static char messageBuffer[41];

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  dispatch(MessageEvent(false, std::string(messageBuffer)));
}

void EventDispatcher::showTimedMessage(const char* format, ...) {
  static char messageBuffer[41];

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  dispatch(MessageEvent(true, std::string(messageBuffer)));
}
