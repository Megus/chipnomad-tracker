#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <string>
#include "input_utils.h"

enum class EventType {
  input,
  rawInput,
  playSong,
  playChain,
  playPhrase,
  playPhraseRow,
  stopPlayback,
  switchScreen,
  pushScreen,
  popScreen,
  showMessage,
};

struct InputEventData {
  bool isKeyDown;
  int keys;
  int tapCount;
};

struct RawInputEventData {
  bool isKeyDown;
  InputCode keyCode;
};

struct Event {
  EventType type; // Event type

  // Possible event data types (can't use union here because of std::string):

  int intValue; // Universal option for all events that only need a single integer value
  std::string stringValue; // Universal option for all events that only need a string value. std::string to simplify memory management
  InputEventData input; // input event data
  RawInputEventData rawInput; // rawInput event data
};

class EventHandler {
  public:
    virtual bool onEvent(Event event) = 0;
};

class EventDispatcher {
  public:
    virtual void dispatch(Event event) = 0;
};

#endif // __EVENTS_H__