#ifndef __MAINLOOP_SDL12_H__
#define __MAINLOOP_SDL12_H__

#include "mainloop.h"

class Gfx;

class MainLoopSDL12 : public MainLoop {
  public:
    MainLoopSDL12(Gfx& gfx) : MainLoop(gfx) {}

    void run(App& app) override;
    void quit() override;
    void triggerQuit() override;
};

#endif // __MAINLOOP_SDL12_H__
