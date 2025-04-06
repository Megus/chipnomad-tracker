#ifndef __CORELIB_MAINLOOP_H__
#define __CORELIB_MAINLOOP_H__

enum MainLoopEvent {
  eventTick,
  eventKeyDown,
  eventKeyUp,
  eventExit,
};

enum Key {
  keyLeft = 0x1,
  keyRight = 0x2,
  keyUp = 0x4,
  keyDown = 0x8,
  keyEdit = 0x10,
  keyOpt = 0x20,
  keyPlay = 0x40,
  keyShift = 0x80,
  keyVolumeUp = 0x100,
  keyVolumeDown = 0x200,
};

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata));
void mainLoopDelay(int ms);
void mainLoopQuit(void);

#endif