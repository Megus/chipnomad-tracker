#ifndef __GFX_H__
#define __GFX_H__

#include <stdint.h>
#include <stdarg.h>

// Bitmap structure for grayscale images
struct Bitmap {
  int widthChars;      // Width in characters
  int heightChars;     // Height in characters
  int widthPixels;     // Actual pixel width (widthChars * charWidth)
  int heightPixels;    // Actual pixel height (heightChars * charHeight)
  uint8_t* data;       // Grayscale data (0=background, 255=foreground), row-major
  void* userdata;      // Platform-specific data (e.g., SDL_Texture*)
};

class FontManager;

// Graphics functions
class Gfx {
  public:
    Gfx(FontManager& fontManager) : fontManager(fontManager) {}
    virtual ~Gfx() = default;

    virtual int setup(int* screenWidth, int* screenHeight) = 0;
    virtual void teardown() = 0;

    // Colors
    virtual void setFgColor(int rgb) = 0;
    virtual void setCursorColor(int rgb) = 0;
    virtual void setBgColor(int rgb) = 0;

    // Screen
    virtual void clear() = 0;
    virtual void updateScreen() = 0;

    // Character-grid drawing (coordinates in characters on a 40x20 grid)
    virtual void clearRect(int x, int y, int w, int h) = 0;
    virtual void cursor(int x, int y, int w) = 0;
    virtual void rect(int x, int y, int w, int h) = 0;
    virtual void print(int x, int y, const char* text) = 0;
    virtual void printf(int x, int y, const char* format, va_list args) = 0;

    // Pixel-level draw (screen pixel coordinates)
    virtual void point(int x, int y, uint32_t color) = 0;

    // Bitmaps
    virtual Bitmap* bitmapCreate(int widthChars, int heightChars) = 0;
    virtual void bitmapClear(Bitmap* bitmap) = 0;
    virtual void bitmapFree(Bitmap* bitmap) = 0;
    virtual void drawBitmap(Bitmap* bitmap, int col, int row) = 0;
    virtual void drawCharBitmap(uint8_t* bitmap, int col, int row) = 0;

    // Font
    virtual int getCharWidth() = 0;
    virtual int getCharHeight() = 0;
    virtual void reloadFont() = 0;

    // HUD / virtual gamepad
    virtual void drawHUD() = 0;
    virtual void setButtonPressed(int buttonIndex, int pressed) = 0;

  protected:
    FontManager& fontManager;
};

#endif // __GFX_H__
