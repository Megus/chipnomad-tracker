#include "corelib_gfx.h"
#include <stdarg.h>

int gfxSetup(int *screenWidth, int* screenHeight) { return 0; }
void gfxCleanup(void) {}
void gfxSetFgColor(int rgb) {}
void gfxSetCursorColor(int rgb) {}
void gfxSetBgColor(int rgb) {}
void gfxClear(void) {}
void gfxUpdateScreen(void) {}
void gfxClearRect(int x, int y, int w, int h) {}
void gfxCursor(int x, int y, int w) {}
void gfxRect(int x, int y, int w, int h) {}
void gfxPrint(int x, int y, const char* text) {}
void gfxPrintf(int x, int y, const char* format, ...) {}
void gfxDrawCharBitmap(uint8_t* bitmap, int col, int row) {}
int gfxGetCharWidth(void) { return 8; }
int gfxGetCharHeight(void) { return 16; }
void gfxReloadFont(void) {}
void gfxDrawHUD(void) {}
void gfxSetButtonPressed(int buttonIndex, int pressed) {}
