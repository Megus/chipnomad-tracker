#include "doctest.h"
#include "gfx.h"
#include "gfx_mock.h"

TEST_SUITE("gfx") {

TEST_CASE("mock records color state and char metrics") {
  GfxMock mock;
  mock.setFgColor(0x123456);
  mock.setBgColor(0x000000);
  mock.setCursorColor(0xFFFFFF);
  CHECK(mock.fgColor == 0x123456);
  CHECK(mock.bgColor == 0x000000);
  CHECK(mock.cursorColor == 0xFFFFFF);
  CHECK(mock.getCharWidth() == 8);
  CHECK(mock.getCharHeight() == 16);
}

TEST_CASE("legacy gfx forwarders dispatch to the global gfx (which is a mock here)") {
  IGfx* previous = gfx;

  GfxMock mock;
  gfx = &mock;

  gfxSetFgColor(0xABCDEF);
  CHECK(mock.fgColor == 0xABCDEF);

  // Variadic forwarder path must reach vprintf without crashing
  gfxPrintf(0, 0, "%d-%s", 7, "x");

  CHECK(gfxGetCharWidth() == 8);

  gfx = previous; // restore
  CHECK(gfx == previous);
}

} // TEST_SUITE("gfx")
