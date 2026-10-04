// Defines the gfx global + legacy gfx* free-function forwarders for the test
// binary. In the real app these live in the compiled platform gfx source
// (platforms/<platform>/corelib_gfx.cpp), which is not part of the test build.
#include "gfx.h"
#include "gfx_mock.h"
#include <stdarg.h>
#include <stddef.h>

static GfxMock defaultGfxMock;
IGfx* gfx = &defaultGfxMock;

int gfxSetup(int* screenWidth, int* screenHeight) { return gfx->setup(screenWidth, screenHeight); }
void gfxCleanup(void) { gfx->cleanup(); }
void gfxSetFgColor(int rgb) { gfx->setFgColor(rgb); }
void gfxSetCursorColor(int rgb) { gfx->setCursorColor(rgb); }
void gfxSetBgColor(int rgb) { gfx->setBgColor(rgb); }
void gfxClear(void) { gfx->clear(); }
void gfxUpdateScreen(void) { gfx->updateScreen(); }
void gfxClearRect(int x, int y, int w, int h) { gfx->clearRect(x, y, w, h); }
void gfxCursor(int x, int y, int w) { gfx->cursor(x, y, w); }
void gfxRect(int x, int y, int w, int h) { gfx->rect(x, y, w, h); }
void gfxPrint(int x, int y, const char* text) { gfx->print(x, y, text); }

void gfxPrintf(int x, int y, const char* format, ...) {
  va_list args;
  va_start(args, format);
  gfx->vprintf(x, y, format, args);
  va_end(args);
}

Bitmap* gfxBitmapCreate(int widthChars, int heightChars) { return gfx->bitmapCreate(widthChars, heightChars); }
void gfxBitmapClear(Bitmap* bitmap) { gfx->bitmapClear(bitmap); }
void gfxBitmapFree(Bitmap* bitmap) { gfx->bitmapFree(bitmap); }
void gfxDrawBitmap(Bitmap* bitmap, int col, int row) { gfx->drawBitmap(bitmap, col, row); }
int gfxGetCharWidth(void) { return gfx->getCharWidth(); }
int gfxGetCharHeight(void) { return gfx->getCharHeight(); }
void gfxReloadFont(void) { gfx->reloadFont(); }
void gfxDrawHUD(void) { gfx->drawHUD(); }
void gfxSetButtonPressed(int buttonIndex, int pressed) { gfx->setButtonPressed(buttonIndex, pressed); }
