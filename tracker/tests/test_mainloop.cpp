#include "doctest.h"
#include "mainloop.h"
#include "mainloop_mock.h"

TEST_SUITE("mainloop") {

static void dummyDraw(void) {}
static void dummyOnEvent(MainLoopEvent) {}

TEST_CASE("mock records run/delay/quit/triggerQuit") {
  MainLoopMock mock;

  mock.run(dummyDraw, dummyOnEvent);
  CHECK(mock.runCalls == 1);
  CHECK(mock.lastDraw == dummyDraw);
  CHECK(mock.lastOnEvent == dummyOnEvent);

  mock.delay(42);
  CHECK(mock.delayCalls == 1);
  CHECK(mock.lastDelayMs == 42);

  mock.quit();
  CHECK(mock.quitCalls == 1);

  mock.triggerQuit();
  CHECK(mock.triggerQuitCalls == 1);
}

TEST_CASE("global mainLoop instance can be swapped and restored") {
  IMainLoop* previous = mainLoop;

  MainLoopMock mock;
  mainLoop = &mock;

  mainLoop->triggerQuit();
  CHECK(mock.triggerQuitCalls == 1);

  mainLoop = previous; // restore
  CHECK(mainLoop == previous);
}

} // TEST_SUITE("mainloop")
