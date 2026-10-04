#ifndef __GFX_SDL2_H__
#define __GFX_SDL2_H__

#include "gfx.h"

class GfxSDL2 : public Gfx {
  public:
    GfxSDL2(FontManager& fontManager) : Gfx(fontManager) {}
    ~GfxSDL2() override;

    int setup(int* screenWidth, int* screenHeight) override;
    void teardown() override;
    void setFgColor(int rgb) override;
    void setCursorColor(int rgb) override;
    void setBgColor(int rgb) override;
    void clear() override;
    void updateScreen() override;
    void clearRect(int x, int y, int w, int h) override;
    void cursor(int x, int y, int w) override;
    void rect(int x, int y, int w, int h) override;
    void print(int x, int y, const char* text) override;
    void printf(int x, int y, const char* format, va_list args) override;
    Bitmap* bitmapCreate(int widthChars, int heightChars) override;
    void bitmapClear(Bitmap* bitmap) override;
    void bitmapFree(Bitmap* bitmap) override;
    void drawBitmap(Bitmap* bitmap, int x, int y) override;
    int getCharWidth() override;
    int getCharHeight() override;
    void reloadFont() override;
    void drawHUD() override;
    void setButtonPressed(int buttonIndex, int pressed) override;
};

#endif // __GFX_SDL2_H__
