#ifndef __SCREEN_INSTRUMENT_H__
#define __SCREEN_INSTRUMENT_H__

#include "screens.h"

extern int cInstrument;

int instrumentCommonColumnCount(int row);
void instrumentCommonDrawStatic(void);
void instrumentCommonDrawCursor(int col, int row);
void instrumentCommonDrawField(int col, int row, int state);
int instrumentCommonOnEdit(int col, int row, enum CellEditAction action);

void initAYInstrument(int instrument);
void initAY2Instrument(int instrument);
void initAYSampleInstrument(int instrument);
void initAYWavetableInstrument(int instrument);

extern ScreenData screenInstrumentAY;
extern ScreenData screenInstrumentAY2;
extern ScreenData screenInstrumentAYSample;
extern ScreenData screenInstrumentAYWavetable;

#endif
