#ifndef __MAINLOOP_H__
#define __MAINLOOP_H__

struct InputCode;

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

// Application interface
class App {
  public:
    virtual ~App() = default;

    virtual bool setup() = 0;
    virtual void teardown() = 0;
    virtual void draw() = 0;
    virtual void onEvent(MainLoopEventData eventData) = 0;
    virtual void onRawInput(InputCode input, int isDown) = 0;
};

// Main platform loop interface
class MainLoop {
  public:
    MainLoop(Gfx& gfx) : gfx(gfx) {}
    virtual ~MainLoop() = default;

    // Run the main loop until the app quits. Calls draw() once per frame and onEvent() for every input/lifecycle event
    virtual void run(App& app) = 0;

    // Shut down the underlying platform (called after run() returns)
    virtual void quit() = 0;

    // Request the loop to exit (posts a quit event)
    virtual void triggerQuit() = 0;

  protected:
    Gfx& gfx;
};

#endif // __MAINLOOP_H__
