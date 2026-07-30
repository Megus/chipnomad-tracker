#ifndef __CORELIB_MAINLOOP_H__
#define __CORELIB_MAINLOOP_H__

#include "corelib_input.h"

enum class MainLoopEvent {
  tick,
  keyDown,
  keyUp,
  exit,
  sleep,
  wake,
  fullRedraw,
};

struct MainLoopEventData {
  MainLoopEvent type;
  union {
    int value;
    InputCode input;
  } data;
};

void mainLoopRun(void (*draw)(void), void (*onEvent)(MainLoopEventData eventData));
void mainLoopDelay(int ms);
void mainLoopQuit(void);
void mainLoopTriggerQuit(void);

#endif
