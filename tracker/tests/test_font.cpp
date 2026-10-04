#include "doctest.h"
#include "font_manager.h"
#include "font_mock.h"

#include <cstring>

TEST_SUITE("font") {

// ============================================================================
// FontMock through the IFont interface
// ============================================================================

TEST_CASE("mock returns fixed font and records calls") {
  FontMock mock;
  const Font* def = mock.getDefault();
  REQUIRE(def != nullptr);
  CHECK(std::strcmp(def->name, "Mock") == 0);

  mock.setCurrent(def);
  CHECK(mock.setCurrentCalls == 1);
  CHECK(mock.lastSetFont == def);

  CHECK(mock.load("/whatever.cnfont") == nullptr);
  CHECK(mock.loadCalls == 1);

  mock.freeFont(nullptr);
  CHECK(mock.freeCalls == 1);
}

// ============================================================================
// FontStdio: resolution selection + default font behaviour
// ============================================================================

TEST_CASE("FontStdio default + current") {
  FontStdio fs;
  const Font* def = fs.getDefault();
  REQUIRE(def != nullptr);
  CHECK(def->resolutionCount == 5);

  // Before setCurrent, getCurrent falls back to the default font
  CHECK(fs.getCurrent() == def);

  fs.setCurrent(nullptr); // resets to default
  CHECK(fs.getCurrent() == def);
}

TEST_CASE("FontStdio selectResolution picks largest that fits") {
  FontStdio fs;
  const Font* def = fs.getDefault();

  // Huge screen -> largest resolution (48x54, index 4)
  const FontResolution* big = fs.selectResolution(def, 4000, 4000);
  REQUIRE(big != nullptr);
  CHECK(big->charWidth == 48);
  CHECK(big->charHeight == 54);

  // Tiny screen -> falls back to smallest (12x16, index 0)
  const FontResolution* small = fs.selectResolution(def, 1, 1);
  REQUIRE(small != nullptr);
  CHECK(small->charWidth == 12);
  CHECK(small->charHeight == 16);
}

TEST_CASE("FontStdio load missing file returns null") {
  FontStdio fs;
  CHECK(fs.load("/definitely/not/here.cnfont") == nullptr);
}

// ============================================================================
// The global `font` is swappable (DI seam)
// ============================================================================

TEST_CASE("global font instance can be swapped and restored") {
  IFont* previous = font;

  FontMock mock;
  font = &mock;
  CHECK(std::strcmp(font->getDefault()->name, "Mock") == 0);

  font = previous; // restore
  CHECK(font == previous);
}

} // TEST_SUITE("font")
