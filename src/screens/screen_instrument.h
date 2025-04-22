#ifndef __SCREEN_INSTRUMENT_H__
#define __SCREEN_INSTRUMENT_H__

#include <screens.h>

int formInstrumentCommonColumnCount(int row);
void formInstrumentCommonDrawForm(void);
void formInstrumentCommonDrawCursor(int col, int row);
void formInstrumentCommonDrawField(int col, int row);
int formInstrumentCommonOnEdit(int col, int row, enum CellEditAction action);

extern struct FormScreenData formInstrumentAY;

#endif
