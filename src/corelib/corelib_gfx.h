#ifndef __CORELIB_GFX_H__
#define __CORELIB_GFX_H__

int gfxSetup(void);
void gfxCleanup(void);
void gfxSetFgColor(int rgb);
void gfxSetCursorColor(int rgb);
void gfxSetSelectionColor(int rgb);
void gfxSetBgColor(int rgb);
void gfxClear(void);
void gfxUpdateScreen(void);

// All following functions take coordinates in characters, assuming a 40x20 screen
void gfxFillRect(int x, int y, int w, int h);
void gfxClearRect(int x, int y, int w, int h);
void gfxCursor(int x, int y, int w);
void gfxSelection(int x, int y, int w, int h);
void gfxPrint(int x, int y, const char* text);
void gfxPrintf(int x, int y, const char* format, ...);

#endif