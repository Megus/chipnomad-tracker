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
  editShallowClone,
  editDeepClone
};

struct ScreenData {
  //int cols;
  int rows;
  int cursorRow;
  int cursorCol;
  int topRow; // For scrollable screens
  int isSelectMode; // 0 - edit, 1 - select, -1 - select is disabled for this screen (e.g. Instrument screen)
  int selectStartRow;
  int selectStartCol;
  int (*getColumnCount)(int row);
  void (*drawStatic)(void);
  void (*drawCursor)(int col, int row);
  void (*drawSelection)(int col1, int row1, int col2, int row2);
  void (*drawRowHeader)(int row, int state);
  void (*drawColHeader)(int col, int state);
  void (*drawField)(int col, int row, int state);
  int (*onEdit)(int col, int row, enum CellEditAction action);
};

extern const struct AppScreen screenProject;
extern const struct AppScreen screenProjectLoad;
extern const struct AppScreen screenProjectSave;
extern const struct AppScreen screenConfirm;
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
void screenFullRedraw(struct ScreenData* screen);
int screenInput(struct ScreenData* screen, int keys, int isDoubleTap);

// Utility functions
void setCellColor(int state, int isEmpty, int hasContent);

// Confirmation dialog
void confirmSetup(const char* message, void (*confirmCallback)(void), void (*cancelCallback)(void));

// Common edit functions
int edit16withLimit(enum CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max);
int edit8withLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max);
int edit8noLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep);
int edit8noLast(enum CellEditAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max);
int edit16withOverflow(enum CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max);

// Character edit
int editCharacter(enum CellEditAction action, char* str, int idx, int maxLen);
char charEditInput(int keys, int isDoubleTap, char* str, int idx, int maxLen);

// FX edit
int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable);
int editFXValue(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable);
int fxEditInput(int keys, int isDoubleTap, uint8_t* fx, uint8_t* lastFX);

#endif

