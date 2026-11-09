#include "events.h"

static char messageBuffer[41]; // Static buffer for message events

ScreenData ScreenData::confirm(const char* title, int confirmValue, int cancelValue) {
  return {.title = title, .confirmValue = confirmValue, .cancelValue = cancelValue};
}

ScreenData ScreenData::confirmOutput(int value) {
  return {.intValue = value};
}

ScreenData ScreenData::editCharacter(char* str, int charIdx, int maxLength) {
  return {.str = str, .charIdx = charIdx, .maxLength = maxLength};
}

ScreenData ScreenData::fileBrowser(bool isFolder, const char* title, const char* startPath, const char* extensions, FilePreviewer* previewer, const char* filename) {
  return {.title = title, .isFolder = isFolder, .startPath = startPath, .extensions = extensions, .previewer = previewer, .filename = filename};
}

Event Event::input(bool isKeyDown, int keys, int tapCount) {
  return {.type = EventType::input, .isKeyDown = isKeyDown, .keys = keys, .tapCount = tapCount};
};

Event Event::rawInput(bool isKeyDown, const InputCode& keyCode) {
  return {.type = EventType::rawInput, .isKeyDown = isKeyDown, .keyCode = keyCode};
}

Event Event::playSong(int songRow, int chainRow) {
  return {.type = EventType::playSong, .loopRange = {.enabled = false}, .songRow = songRow, .chainRow = chainRow};
}

Event Event::playSong(int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  return {.type = EventType::playSong,  .loopRange = loopRange, .songRow = songRow, .chainRow = chainRow};
}

Event Event::playChain(int track, int songRow, int chainRow) {
  return {.type = EventType::playChain, .loopRange = {.enabled = false}, .track = track, .songRow = songRow, .chainRow = chainRow};
}

Event Event::playChain(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  return {.type = EventType::playChain, .loopRange = loopRange, .track = track, .songRow = songRow, .chainRow = chainRow};
}

Event Event::playPhrase(int track, int songRow, int chainRow) {
  return {.type = EventType::playPhrase, .loopRange = {.enabled = false}, .track = track, .songRow = songRow, .chainRow = chainRow};
}

Event Event::playPhrase(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange) {
  return {.type = EventType::playPhrase, .loopRange = loopRange, .track = track, .songRow = songRow, .chainRow = chainRow};
}

Event Event::stopPlayback() {
  return {.type = EventType::stopPlayback};
}

Event Event::redrawScreen() {
  return {.type = EventType::redrawScreen};
}

Event Event::switchScreen(ScreenEnum screen) {
  return {.type = EventType::switchScreen, .screen = screen};
}

Event Event::switchScreen(ScreenEnum screen, const ScreenData& input) {
  return {.type = EventType::switchScreen, .screen = screen, .screenData = input};
}

Event Event::pushScreen(ScreenEnum screen) {
  return {.type = EventType::pushScreen, .screen = screen};
}

Event Event::pushScreen(ScreenEnum screen, const ScreenData& input) {
  return {.type = EventType::pushScreen, .screen = screen, .screenData = input};
}

Event Event::popScreen(ScreenEnum screen) {
  return {.type = EventType::popScreen, .screen = screen};
}

Event Event::popScreen(ScreenEnum screen, const ScreenData& output) {
  return {.type = EventType::popScreen, .screen = screen, .screenData = output};
}

Event Event::screenOutput(ScreenEnum screen, const ScreenData& output) {
  return {.type = EventType::screenOutput, .screen = screen, .screenData = output};
}

Event Event::showInstantMessage(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  return {.type = EventType::showMessage, .isTimedMessage = false, .message = messageBuffer};
}

Event Event::showTimedMessage(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  return {.type = EventType::showMessage, .isTimedMessage = true, .message = messageBuffer};
}
