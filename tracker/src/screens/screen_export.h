#ifndef __SCREEN_EXPORT_H__
#define __SCREEN_EXPORT_H__

#include "screens.h"

// Forward declaration
struct Exporter;

// Common rows on the export screen
#define SCR_EXPORT_ROWS (4)

// Export state
extern struct Exporter* currentExporter;
extern int startRow;

int exportCommonColumnCount(int row);
void exportCommonDrawStatic(void);
void exportCommonDrawCursor(int col, int row);
void exportCommonDrawField(int col, int row, int state);
int exportCommonOnEdit(int col, int row, enum CellEditAction action);
void generateExportPath(char* outputPath, int maxLen, const char* extension);
void generatePSGExportPath(char* outputPath, int maxLen);

extern ScreenData screenExportAY;

#endif