#ifndef __CORELIB_MAINLOOP_H__
#define __CORELIB_MAINLOOP_H__

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

typedef struct {
  enum MainLoopEvent type;
  union {
    int value;
    InputCode input;
  } data;
} MainLoopEventData;

void mainLoopRun(void (*draw)(void), void (*onEvent)(MainLoopEventData eventData));
void mainLoopDelay(int ms);
void mainLoopQuit(void);
void mainLoopTriggerQuit(void);

#endif