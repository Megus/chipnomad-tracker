#ifndef __SCREENS_H__
#define __SCREENS_H__

#include "common.h"

#define MESSAGE_TIME (60)

typedef struct AppScreen {
  void (*init)(void);
  void (*setup)(int input);
  void (*fullRedraw)(void);
  void (*draw)(void);
  int (*onInput)(int isKeyDown, int keys, int tapCount); // Return 1 if handled, 0 if not
} AppScreen;

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
  editDeepClone,
  editCopy,
  editCut,
  editPaste,
  editSwitchSelection,
  editMultiIncrease,
  editMultiDecrease,
  editMultiIncreaseBig,
  editMultiDecreaseBig
};

typedef struct ScreenData {
  //int cols;
  int rows;
  int cursorRow;
  int cursorCol;
  int topRow; // For scrollable screens
  int selectMode; // 0 - edit, 1 - select, -1 - select is disabled for this screen (e.g. Instrument screen)
  int selectStartRow;
  int selectStartCol;
  int selectAnchorRow; // Original cell where selection mode was entered
  int selectAnchorCol; // Original cell where selection mode was entered
  int (*getColumnCount)(int row);
  void (*drawStatic)(void);
  void (*drawCursor)(int col, int row);
  void (*drawSelection)(int col1, int row1, int col2, int row2);
  void (*drawRowHeader)(int row, int state);
  void (*drawColHeader)(int col, int state);
  void (*drawField)(int col, int row, int state);
  int (*onEdit)(int col, int row, enum CellEditAction action);
} ScreenData;

extern const AppScreen screenProject;
extern const AppScreen screenProjectLoad;
extern const AppScreen screenProjectSave;
extern const AppScreen screenConfirm;
extern const AppScreen screenPitchTable;
extern const AppScreen screenFileBrowser;
extern const AppScreen screenCreateFolder;
extern const AppScreen screenSong;
extern const AppScreen screenChain;
extern const AppScreen screenPhrase;
extern const AppScreen screenGroove;
extern const AppScreen screenInstrument;
extern const AppScreen screenInstrumentPool;
extern const AppScreen screenTable;
extern const AppScreen screenExport;
extern const AppScreen screenManage;
extern const AppScreen screenSettings;
extern const AppScreen screenColorTheme;

extern const AppScreen* currentScreen;

void screenSetup(const AppScreen* screen, int input);
void screenDraw(void);
void screenMessage(int time, const char* format, ...);
void screensInitAll(void);

// Spreadsheet functions
void screenFullRedraw(ScreenData* screen);
void screenDrawOverlays(ScreenData* screen);
int screenInput(ScreenData* screen, int isKeyDown, int keys, int tapCount);

// Utility functions
void setCellColor(int state, int isEmpty, int hasContent);
void getSelectionBounds(ScreenData* screen, int* startCol, int* startRow, int* endCol, int* endRow);
int isSingleColumnSelection(ScreenData* screen);

// Confirmation dialog
void confirmSetup(const char* message, void (*confirmCallback)(void), void (*cancelCallback)(void));

// Common edit functions
int edit16withLimit(enum CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max);
int edit8withLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max);
int edit8noLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep);
int edit8noLast(enum CellEditAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max);
int edit16withOverflow(enum CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max);
int applyMultiEdit(ScreenData* screen, enum CellEditAction action, int (*editFunc)(int col, int row, enum CellEditAction action));
int applyPhraseRotation(int phraseIdx, int startRow, int endRow, int direction);
int applyTableRotation(int tableIdx, int startRow, int endRow, int direction);
int applySongMoveDown(int startCol, int startRow, int endCol, int endRow);
int applySongMoveUp(int startCol, int startRow, int endCol, int endRow);
enum CellEditAction convertMultiAction(enum CellEditAction action);

// Character edit
int editCharacter(enum CellEditAction action, char* str, int idx, int maxLen);
char charEditInput(int keys, int tapCount, char* str, int idx, int maxLen);

// FX edit
int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable);
int editFXValue(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable);
int fxEditInput(int keys, int tapCount, uint8_t* fx, uint8_t* lastFX);
void fxEditFullDraw(uint8_t currentFX);

// Manage screen functions
int manageColumnCount(int row);
void manageDrawStatic(void);
void manageDrawCursor(int col, int row);
void manageDrawField(int col, int row, int state);
int manageOnEdit(int col, int row, enum CellEditAction action);

#endif
