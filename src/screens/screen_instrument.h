#ifndef __SCREEN_INSTRUMENT_H__
#define __SCREEN_INSTRUMENT_H__

#include <screens.h>

int instrumentCommonColumnCount(int row);
void instrumentCommonDrawStatic(void);
void instrumentCommonDrawCursor(int col, int row);
void instrumentCommonDrawField(int col, int row, int state);
int instrumentCommonOnEdit(int col, int row, enum CellEditAction action);

extern struct ScreenData formInstrumentAY;

#endif
