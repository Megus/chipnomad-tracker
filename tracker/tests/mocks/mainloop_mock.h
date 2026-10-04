#ifndef __MAINLOOP_MOCK_H__
#define __MAINLOOP_MOCK_H__

#include "i_mainloop.h"

// MainLoopMock: IMainLoop stub for tests. Records calls; run() returns
// immediately without pumping events.
class MainLoopMock : public IMainLoop {
  public:
    int runCalls = 0;
    int delayCalls = 0;
    int quitCalls = 0;
    int triggerQuitCalls = 0;
    int lastDelayMs = 0;
    MainLoopDrawFunc lastDraw = nullptr;
    MainLoopEventFunc lastOnEvent = nullptr;

    void run(MainLoopDrawFunc draw, MainLoopEventFunc onEvent) override {
      runCalls++;
      lastDraw = draw;
      lastOnEvent = onEvent;
    }

    void delay(int ms) override {
      delayCalls++;
      lastDelayMs = ms;
    }

    void quit() override {
      quitCalls++;
    }

    void triggerQuit() override {
      triggerQuitCalls++;
    }
};

#endif // __MAINLOOP_MOCK_H__
