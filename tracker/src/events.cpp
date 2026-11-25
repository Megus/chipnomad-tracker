#include "events.h"

// ScreenData

ScreenData ScreenData::confirm(const char* title, int confirmValue, int cancelValue) {
  return {.title = title, .confirmValue = confirmValue, .cancelValue = cancelValue};
}

ScreenData ScreenData::confirmOutput(int value) {
  return {.intValue = value};
}

ScreenData ScreenData::enterCharacter(char* str, int charIdx, int maxLength) {
  return {.str = str, .charIdx = charIdx, .maxLength = maxLength};
}

ScreenData ScreenData::enterString(char* str, int maxLength, const char* title, const char* prompt, const char* detail) {
  return {.str = str, .title = title, .prompt = prompt, .detail = detail, .maxLength = maxLength};
}

ScreenData ScreenData::enterStringOutput(int value, char* str) {
  return {.intValue = value, .str = str};
}

ScreenData ScreenData::fileBrowser(bool isFolder, const char* title, const char* startPath, const char* extensions, FilePreviewer* previewer, const char* filename) {
  return {.title = title, .isFolder = isFolder, .folderPath = startPath, .extensions = extensions, .previewer = previewer, .filename = filename};
}

ScreenData ScreenData::fileBrowserOutput(bool isFolder, const char* filePath, const char* folderPath) {
  return {.isFolder = isFolder, .filePath = filePath, .folderPath = folderPath};
}

ScreenData ScreenData::editFX(int fxIdx, int instrumentIdx) {
  return {.intValue = fxIdx, .instrumentIdx = instrumentIdx};
}

// Event

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

Event Event::showInstantMessage(const char* message) {
  return {.type = EventType::showMessage, .isTimedMessage = false, .message = message};
}

Event Event::showTimedMessage(const char* message) {
  return {.type = EventType::showMessage, .isTimedMessage = true, .message = message};
}

Event Event::setProject(chipnomad::Project* project) {
  return {.type = EventType::setProject, .project = project};
}
