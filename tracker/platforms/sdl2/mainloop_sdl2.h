#ifndef __MAINLOOP_SDL2_H__
#define __MAINLOOP_SDL2_H__

#include "mainloop.h"

class Gfx;

class MainLoopSDL2 : public MainLoop {
  public:
    MainLoopSDL2(Gfx& gfx) : MainLoop(gfx) {}

    void run(App& app) override;
    void quit() override;
    void triggerQuit() override;
};

#endif // __MAINLOOP_SDL2_H__
