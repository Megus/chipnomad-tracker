#ifndef __CORELIB_FONT_H__
#define __CORELIB_FONT_H__

#include <stdint.h>

// Font resolution entry
typedef struct {
  int charWidth;    // Character width in pixels
  int charHeight;   // Character height in pixels
  uint8_t* data;    // Font bitmap data (95 chars, ASCII 32-126)
} FontResolution;

// Font with multiple resolutions
typedef struct {
  char name[16];
  FontResolution resolutions[5];  // Up to 5 resolutions
  int resolutionCount;
} Font;

/**
 * @brief Get the default built-in font
 * 
 * @return const Font* Pointer to default font
 */
const Font* fontGetDefault(void);

/**
 * @brief Set the current font
 * 
 * @param font Font to use (NULL to reset to default)
 */
void fontSetCurrent(const Font* font);

/**
 * @brief Get the current font
 * 
 * @return const Font* Pointer to current font
 */
const Font* fontGetCurrent(void);

/**
 * @brief Select best font resolution for given screen size
 * 
 * @param font Font to select from
 * @param screenWidth Screen width in pixels
 * @param screenHeight Screen height in pixels
 * @return const FontResolution* Best matching resolution, or NULL if none fit
 */
const FontResolution* fontSelectResolution(const Font* font, int screenWidth, int screenHeight);

/**
 * @brief Load font from file
 * 
 * @param path Path to .cnfont file
 * @return Font* Loaded font, or NULL on error. Caller must free with fontFree()
 */
Font* fontLoad(const char* path);

/**
 * @brief Free a loaded font
 * 
 * @param font Font to free (does nothing if NULL or default font)
 */
void fontFree(Font* font);

#endif
