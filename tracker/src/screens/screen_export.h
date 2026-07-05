#ifndef __SCREEN_EXPORT_H__
#define __SCREEN_EXPORT_H__

#include "screens.h"
#include "export/export.h"

// Common rows on the export screen
#define SCR_EXPORT_ROWS (5)

// Export state
extern Exporter* currentExporter;
extern int startRow;

int exportCommonColumnCount(int row);
void exportCommonDrawStatic(void);
void exportCommonDrawCursor(int col, int row);
void exportCommonDrawField(int col, int row, CellState state);
int exportCommonOnEdit(int col, int row, CellEditAction action);
void generateExportPath(char* outputPath, int maxLen, const char* extension);
void generatePSGExportPath(char* outputPath, int maxLen);

extern ScreenData screenExportAY;

#endif
