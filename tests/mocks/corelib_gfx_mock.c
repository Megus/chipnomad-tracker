#include "corelib_gfx.h"
#include <stdarg.h>
#include <stdio.h>

// Mock implementations
int gfxSetup(void) { return 0; }
void gfxCleanup(void) {}
void gfxSetFgColor(int rgb) {}
void gfxSetCursorColor(int rgb) {}
void gfxSetSelectionColor(int rgb) {}
void gfxSetBgColor(int rgb) {}
void gfxClear(void) {}
void gfxUpdateScreen(void) {}
void gfxFillRect(int x, int y, int w, int h) {}
void gfxClearRect(int x, int y, int w, int h) {}
void gfxCursor(int x, int y, int w) {}
void gfxSelection(int x, int y, int w, int h) {}
void gfxPrint(int x, int y, const char* text) {}
void gfxPrintf(int x, int y, const char* format, ...) {}