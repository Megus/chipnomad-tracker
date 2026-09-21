#ifndef __GFX_SDL12_H__
#define __GFX_SDL12_H__

#include "gfx.h"

class GfxSDL12 : public Gfx {
  public:
    GfxSDL12(FontManager& fontManager) : Gfx(fontManager) {}
    ~GfxSDL12() override;

    int setup(int* screenWidth, int* screenHeight) override;
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
    void point(int x, int y, uint32_t color) override;
    Bitmap* bitmapCreate(int widthChars, int heightChars) override;
    void bitmapClear(Bitmap* bitmap) override;
    void bitmapFree(Bitmap* bitmap) override;
    void drawBitmap(Bitmap* bitmap, int col, int row) override;
    void drawCharBitmap(uint8_t* bitmap, int col, int row) override;
    int getCharWidth() override;
    int getCharHeight() override;
    void reloadFont() override;
    void drawHUD() override;
    void setButtonPressed(int buttonIndex, int pressed) override;
};

#endif // __GFX_SDL12_H__
