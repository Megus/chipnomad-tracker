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
  editCharacter,
};

class FilePreviewer {
  public:
    virtual bool startFilePreview(const char* path) = 0;
    virtual bool stopFilePreview() = 0;
};

// Base struct for screen input and output. Most screens only need an int value
struct ScreenData {
  static ScreenData confirm(const char* title, int confirmValue, int cancelValue);
  static ScreenData confirmOutput(int value);
  static ScreenData editCharacter(char* str, int charIdx, int maxLength);
  static ScreenData fileBrowser(bool isFolder, const char* title, const char* startPath, const char* extensions, FilePreviewer* previewer, const char* filename);

  int intValue = 0; // Generic input/output int value
  char* str = nullptr; // Generic input/output string value
  const char* title = nullptr; // Generic popup title

  // Confirm popup
  int confirmValue = 0;
  int cancelValue = 0;

  // Character edit
  int charIdx = 0;
  int maxLength = 0;

  // File browser
  bool isFolder = false; // Input/output for file browser
  const char* startPath = nullptr;
  const char* extensions = nullptr;
  FilePreviewer* previewer = nullptr;
  const char* filename = nullptr;
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
  bool isKeyDown = false;
  int keys = 0;
  InputCode keyCode;
  int tapCount = 0;

  // Play events
  chipnomad::LoopRange loopRange = {.enabled = false};
  int track = 0;
  int songRow = 0;
  int chainRow = 0;

  // Screen events
  ScreenEnum screen = ScreenEnum::song;
  ScreenData screenData;

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
    virtual void dispatch(const Event& e) = 0;
    virtual void dispatchInstant(const Event& e) = 0;
};

#endif // __EVENTS_H__