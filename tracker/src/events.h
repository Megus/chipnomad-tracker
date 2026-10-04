#ifndef __EVENTS_H__
#define __EVENTS_H__

enum class EventType {
  playSong,
  playChain,
  playPhrase,
  playPhraseRow,
  stopPlayback,
  switchScreen,
  pushScreen,
  popScreen,
};

struct Event {
  EventType type;
  union {
    int value;
  } data;
};

class EventHandler {
  virtual bool onEvent(Event event) = 0;
};

class EventDispatcher {
  virtual void dispatch(Event event) = 0;
};

#endif // __EVENTS_H__