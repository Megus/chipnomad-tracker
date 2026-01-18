#ifndef __CORELIB_GFX_H__
#define __CORELIB_GFX_H__

#include <stdint.h>

/**
 * @brief Initialize graphics system. If screenWidth and screenHeight are not NULL or don't contain zeros,
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
 * @brief Draw 8-bit grayscale bitmap at character position
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

#endif