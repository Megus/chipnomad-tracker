#ifndef __SCREEN_PROJECT_H__
#define __SCREEN_PROJECT_H__

#include <screens.h>

int projectCommonColumnCount(int row);
void projectCommonDrawStatic(void);
void projectCommonDrawCursor(int col, int row);
void projectCommonDrawField(int col, int row, int state);
int projectCommonOnEdit(int col, int row, enum CellEditAction action);

extern struct ScreenData screenProjectAY;

#endif