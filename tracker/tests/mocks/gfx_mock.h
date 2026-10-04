#ifndef __GFX_MOCK_H__
#define __GFX_MOCK_H__

#include "i_gfx.h"
#include <cstddef>

// GfxMock: no-op IGfx for tests. Records a few calls; drawing is a no-op and
// bitmapCreate returns nullptr (tests that need real bitmaps should use the
// real platform impl, which isn't linked into the test binary).
class GfxMock : public IGfx {
  public:
    int setupCalls = 0;
    int fgColor = 0;
    int bgColor = 0;
    int cursorColor = 0;

    int setup(int*, int*) override { setupCalls++; return 0; }
    void cleanup() override {}
    void setFgColor(int rgb) override { fgColor = rgb; }
    void setCursorColor(int rgb) override { cursorColor = rgb; }
    void setBgColor(int rgb) override { bgColor = rgb; }
    void clear() override {}
    void updateScreen() override {}
    void clearRect(int, int, int, int) override {}
    void cursor(int, int, int) override {}
    void rect(int, int, int, int) override {}
    void print(int, int, const char*) override {}
    void vprintf(int, int, const char*, va_list) override {}
    void point(int, int, uint32_t) override {}
    Bitmap* bitmapCreate(int, int) override { return nullptr; }
    void bitmapClear(Bitmap*) override {}
    void bitmapFree(Bitmap*) override {}
    void drawBitmap(Bitmap*, int, int) override {}
    int getCharWidth() override { return 8; }
    int getCharHeight() override { return 16; }
    void reloadFont() override {}
    void drawHUD() override {}
    void setButtonPressed(int, int) override {}
};

#endif // __GFX_MOCK_H__
