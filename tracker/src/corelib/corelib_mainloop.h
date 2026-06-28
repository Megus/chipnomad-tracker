#ifndef __CORELIB_MAINLOOP_H__
#define __CORELIB_MAINLOOP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "corelib_input.h"

enum MainLoopEvent {
  eventTick,
  eventKeyDown,
  eventKeyUp,
  eventExit,
  eventSleep,
  eventWake,
  eventFullRedraw,
};

struct MainLoopEventData {
  enum MainLoopEvent type;
  union {
    int value;
    InputCode input;
  } data;
};

void mainLoopRun(void (*draw)(void), void (*onEvent)(MainLoopEventData eventData));
void mainLoopDelay(int ms);
void mainLoopQuit(void);
void mainLoopTriggerQuit(void);


#ifdef __cplusplus
}
#endif

#endif
