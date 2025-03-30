#ifndef __CORELIB_MAINLOOP_H__
#define __CORELIB_MAINLOOP_H__

enum MainLoopEvent {
  eventKeyDown,
  eventKeyUp,
  eventExit,
};

enum Key {
  keyLeft,
  keyRight,
  keyUp,
  keyDown,
  keyA,
  keyB,
  keyStart,
  keySelect,
  keyVolumeUp,
  keyVolumeDown,
};

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata));
void mainLoopDelay(int ms);
void mainLoopQuit(void);

#endif