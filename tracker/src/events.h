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
  screenOutput,
  showMessage,
};

enum class ScreenEnum : int {
  song,
  project,
  pitchTable,
  exportProject,
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
  enterCharacter,
  enterString,
};

class FilePreviewer {
  public:
    virtual bool startFilePreview(const char* path) = 0;
    virtual bool stopFilePreview() = 0;
};

// Screen input and output
struct ScreenData {
  static ScreenData confirm(const char* title, int confirmValue, int cancelValue);
  static ScreenData confirmOutput(int value);
  static ScreenData enterCharacter(char* str, int charIdx, int maxLength);
  static ScreenData enterString(char* str, int maxLength, const char* title, const char* prompt, const char* detail);
  static ScreenData enterStringOutput(int value, char* str);
  static ScreenData fileBrowser(bool isFolder, const char* title, const char* startPath, const char* extensions, FilePreviewer* previewer, const char* filename);
  static ScreenData fileBrowserOutput(bool isFolder, const char* filePath, const char* folderPath);

  int intValue = 0; // Generic input/output int value
  char* str = nullptr; // Generic input/output string value
  const char* title = nullptr; // Generic popup title
  const char* prompt = nullptr; // Generic popup prompt
  const char* detail = nullptr; // Generic detail string for popups

  // Confirm popup
  int confirmValue = 0; // Confirm: return value when confirmed
  int cancelValue = 0; // Confirm: return value when cancelled

  // Character/string edit
  int charIdx = 0; // Character edit: char index in the string
  int maxLength = 0; // Character/string edit: max string length

  // File browser
  bool isFolder = false; // File browser: Input/output for file browser
  const char* filePath = nullptr; // File browser: Output for full file path
  const char* folderPath = nullptr; // File browser: Input/output for folder path (without filename)
  const char* extensions = nullptr; // File browser: Comma-separated list of extensions (.cnm,.vt2)
  FilePreviewer* previewer = nullptr; // File browser: Pointer to the file previewer
  const char* filename = nullptr; // File browser: Filename for saving (used for overwrite check)
};

// Event struct
struct Event {
  // Convenient factory methods
  static Event input(bool isKeyDown, int keys, int tapCount);
  static Event rawInput(bool isKeyDown, const InputCode& keyCode);
  static Event playSong(int songRow, int chainRow);
  static Event playSong(int songRow, int chainRow, const chipnomad::LoopRange& loopRange);
  static Event playChain(int track, int songRow, int chainRow);
  static Event playChain(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange);
  static Event playPhrase(int track, int songRow, int chainRow);
  static Event playPhrase(int track, int songRow, int chainRow, const chipnomad::LoopRange& loopRange);
  // void playPhraseRow();
  static Event stopPlayback();
  static Event redrawScreen();
  static Event switchScreen(ScreenEnum screen);
  static Event switchScreen(ScreenEnum screen, const ScreenData& input);
  static Event pushScreen(ScreenEnum screen);
  static Event pushScreen(ScreenEnum screen, const ScreenData& input);
  static Event popScreen(ScreenEnum screen);
  static Event popScreen(ScreenEnum screen, const ScreenData& output);
  static Event screenOutput(ScreenEnum screen, const ScreenData& output);
  static Event showInstantMessage(const char* format, ...);
  static Event showTimedMessage(const char* format, ...);

  EventType type = EventType::none;
  // All various parameters that can go to the event

  // Input events
  bool isKeyDown = false; // Input: Key down or key up?
  int keys = 0; // Input: Pressed keys (use bit mask)
  InputCode keyCode; // RawInput: Raw key code
  int tapCount = 0; // Input: number of taps

  // Play events
  chipnomad::LoopRange loopRange = {.enabled = false}; // Play: loop range
  int track = 0; // Play: track number
  int songRow = 0; // Play: song row
  int chainRow = 0; // Play: chain row

  // Screen events
  ScreenEnum screen = ScreenEnum::song; // Screen events: screen ID (enum)
  ScreenData screenData; // Screen events: input/output data

  // Message
  bool isTimedMessage = false;
  const char* message = nullptr;
};

class EventHandler {
  public:
    virtual bool onEvent(const Event& e) = 0;
};

class EventDispatcher {
  public:
    virtual void dispatch(const Event& e) = 0; // Dispatch event asynchronously
    virtual void dispatchInstant(const Event& e) = 0; // Dispatch event instantly
};

#endif // __EVENTS_H__