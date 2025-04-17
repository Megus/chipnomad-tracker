#ifndef __SCREENS_H__
#define __SCREENS_H__

#include <common.h>

struct AppScreen {
  void (*setup)(int input);
  void (*fullRedraw)(void);
  void (*draw)(void);
  void (*onInput)(int keys, int isDoubleTap);
};

enum CellState {
  stateFocus = 1,
  stateSelected = 2,
};

enum CellEditAction {
  editClear,
  editTap,
  editDoubleTap,
  editIncrease,
  editDecrease,
  editIncreaseBig,
  editDecreaseBig,
};

struct SpreadsheetScreenData {
  int cols;
  int rows;
  int cursorRow;
  int cursorCol;
  int topRow; // For scrollable screens
  void (*drawCursor)(int col, int row);
  void (*drawRowHeader)(int row, int state);
  void (*drawColHeader)(int col, int state);
  void (*drawCell)(int col, int row, int state);
  int (*onEdit)(int col, int row, enum CellEditAction action);
};

extern const struct AppScreen screenProject;
extern const struct AppScreen screenSong;
extern const struct AppScreen screenChain;
extern const struct AppScreen screenPhrase;
extern const struct AppScreen screenGroove;
extern const struct AppScreen screenInstrument;
extern const struct AppScreen screenTable;

extern const struct AppScreen* currentScreen;

void screenSetup(const struct AppScreen* screen, int input);
void screenMessage(const char* format, ...);

// Spreadsheet functions
void spreadsheetFullRedraw(struct SpreadsheetScreenData* sheet);
int spreadsheetInput(struct SpreadsheetScreenData* sheet, int keys, int isDoubleTap);

// Utility functions
void setCellColor(int state, int isEmpty, int hasContent);

// Common edit functions
int edit16withLimit(enum CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigIncrease, uint16_t upperLimit);
int edit8withLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigIncrease, uint8_t upperLimit);
int edit8noLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigIncrease);
int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX);

#endif
