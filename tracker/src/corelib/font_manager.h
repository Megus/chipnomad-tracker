#ifndef __FONT_MANAGER_H__
#define __FONT_MANAGER_H__

#include <stdint.h>

// Default font
extern "C" {
  extern const uint8_t font12x16[];
  extern const uint8_t font16x24[];
  extern const uint8_t font24x36[];
  extern const uint8_t font32x48[];
  extern const uint8_t font48x54[];
}

// Font resolution entry
struct FontResolution {
  int charWidth;        // Character width in pixels
  int charHeight;       // Character height in pixels
  const uint8_t* data;  // Font bitmap data (95 chars, ASCII 32-126)
};

// Font with multiple resolutions
struct Font {
  char name[16];
  FontResolution resolutions[5];  // Up to 5 resolutions. Need to be ordered from the smallest to the largest.
  int resolutionCount;
};

// Bitmap font management
class FontManager {
  public:
    // Get the default built-in font
    const Font* getDefault();

    // Set the current font (NULL resets to default)
    void setCurrent(const Font* font);

    // Get the current font
    const Font* getCurrent();

    // Select best font resolution for the given screen size.
    // Returns the best matching resolution, or NULL if the font has none.
    const FontResolution* selectResolution(const Font* font, int screenWidth, int screenHeight);

    // Load a font from a .cnfont file. Returns a font the caller must free with freeFont(), or NULL on error
    Font* load(const char* path);

    // Free a loaded font (no-op for NULL or the default font).
    void freeFont(Font* font);

  private:
    const Font* currentFont = nullptr;
};

#endif // __FONT_MANAGER_H__
