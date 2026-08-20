#ifndef __CORELIB_GFX_H__
#define __CORELIB_GFX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern "C" {

// Bitmap structure for multi-character grayscale images
struct Bitmap {
  int widthChars;      // Width in characters
  int heightChars;     // Height in characters
  int widthPixels;     // Actual pixel width (widthChars * charWidth)
  int heightPixels;    // Actual pixel height (heightChars * charHeight)
  uint8_t* data;       // Grayscale data (0=background, 255=foreground), row-major order
  void* userdata;      // Platform-specific data (e.g., SDL_Texture*)
};

/**
 * @brief Initialize graphics system. If screenWidth and screenHeight are not NULL and don't contain zeros,
 * function should read the screen resolution from these pointers. Otherwise, the function would detect screen resolution
 * and store it in these pointers (when they're not NULL). These arguments can be ignored on systems where
 * it is not required.
 *
 * @param screenWidth pointer to screen width in pixels
 * @param screenHeight pointer to screen height in pixels
 * @return int
 */
int gfxSetup(int *screenWidth, int *screenHeight);

/**
 * @brief Cleanup graphics system
 */
void gfxCleanup(void);

/**
 * @brief Set foreground color
 *
 * @param rgb
 */
void gfxSetFgColor(int rgb);

/**
 * @brief Set cursor color
 *
 * @param rgb
 */
void gfxSetCursorColor(int rgb);

/**
 * @brief Set background color
 *
 * @param rgb
 */
void gfxSetBgColor(int rgb);

/**
 * @brief Clear screen
 */
void gfxClear(void);

/**
 * @brief Update screen from the buffer
 */
void gfxUpdateScreen(void);

// All following functions take coordinates in characters, assuming a 40x20 screen

void gfxClearRect(int x, int y, int w, int h);
void gfxCursor(int x, int y, int w);
void gfxRect(int x, int y, int w, int h);
void gfxPrint(int x, int y, const char* text);
void gfxPrintf(int x, int y, const char* format, ...);

/**
 * @brief Draw a line with a given opacity (alpha blend over the background)
 *
 * @param x1 Start X in pixels
 * @param y1 Start Y in pixels
 * @param x2 End X in pixels
 * @param y2 End Y in pixels
 * @param alpha Opacity 0..255 (0 = invisible, 255 = fully opaque)
 */
void gfxDrawLineAlpha(int x1, int y1, int x2, int y2, int alpha);

/**
 * @brief Enable or disable transparent text rendering
 *
 * When enabled, gfxPrint/gfxPrintf draw only the glyphs without clearing the
 * character cell background. This lets text overlay an existing background
 * layer (e.g. the bottom table row drawn over the oscilloscope).
 *
 * @param enabled 1 to enable transparent text, 0 to disable (default)
 */
void gfxSetTransparentText(int enabled);

/**
 * @brief Begin rendering into the offscreen UI layer (a transparent full-screen
 * texture). Subsequent gfx drawing calls render into this layer instead of the
 * screen. The layer is cleared to transparent (alpha 0) first.
 */
void gfxBeginLayer(void);

/**
 * @brief End rendering into the UI layer and switch back to the screen.
 */
void gfxEndLayer(void);

/**
 * @brief Composite the cached UI layer onto the screen with alpha blending.
 *
 * The layer's transparent pixels let the background (cleared screen + any
 * full-bleed visuals like the oscilloscope) show through.
 */
void gfxCompositeLayer(void);

/**
 * @brief Create a bitmap with specified size in characters
 *
 * @param widthChars Width in characters
 * @param heightChars Height in characters
 * @return Bitmap* Allocated bitmap (caller must free with gfxBitmapFree)
 */
Bitmap* gfxBitmapCreate(int widthChars, int heightChars);

/**
 * @brief Clear bitmap (set all pixels to 0)
 *
 * @param bitmap Bitmap to clear
 */
void gfxBitmapClear(Bitmap* bitmap);

/**
 * @brief Free a bitmap and its resources
 *
 * @param bitmap Bitmap to free
 */
void gfxBitmapFree(Bitmap* bitmap);

/**
 * @brief Draw a bitmap at character grid position
 *
 * @param bitmap Bitmap to draw
 * @param col Column position in characters
 * @param row Row position in characters
 */
void gfxDrawBitmap(Bitmap* bitmap, int col, int row);

/**
 * @brief Draw 8-bit grayscale bitmap at character position (legacy, single character)
 *
 * @param bitmap Grayscale bitmap data (row by row, 0=background, 255=foreground)
 * @param col Column position in characters
 * @param row Row position in characters
 */
void gfxDrawCharBitmap(uint8_t* bitmap, int col, int row);

/**
 * @brief Get character width in pixels
 *
 * @return int Character width
 */
int gfxGetCharWidth(void);

/**
 * @brief Get character height in pixels
 *
 * @return int Character height
 */
int gfxGetCharHeight(void);

/**
 * @brief Get the vertical centering offset of the text grid in pixels
 *
 * The 40x20 text grid is centered on the screen; this returns the pixel margin
 * above and below it (0 when the grid exactly fills the screen height). Useful
 * for drawing full-bleed elements that reach the physical screen edges.
 *
 * @return int Vertical offset in pixels
 */
int gfxGetOffsetY(void);

/**
 * @brief Get the physical screen width in pixels
 *
 * @return int Screen width in pixels
 */
int gfxGetScreenWidth(void);

/**
 * @brief Get the physical screen height in pixels
 *
 * @return int Screen height in pixels
 */
int gfxGetScreenHeight(void);

/**
 * @brief Reload font (recreate font texture after font change)
 */
void gfxReloadFont(void);

/**
 * @brief Draw HUD overlay (e.g. virtual gamepad on touch devices)
 */
void gfxDrawHUD(void);
void gfxSetButtonPressed(int buttonIndex, int pressed);

}


#ifdef __cplusplus
}
#endif

#endif
