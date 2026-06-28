#ifndef __SCREENS_H__
#define __SCREENS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "../chipnomad_lib/playback.h"

#define MESSAGE_TIME (60)

enum class CellState : int {
  normal = 0,
  focus = 1,
  selected = 2,
};

enum class CellEditAction : int {
  clear,
  tap,
  doubleTap,
  increase,
  decrease,
  increaseBig,
  decreaseBig,
  shallowClone,
  deepClone,
  copy,
  cut,
  paste,
  switchSelection,
  multiIncrease,
  multiDecrease,
  multiIncreaseBig,
  multiDecreaseBig
};

enum class ScreenPlaybackLevel : int {
  none,
  song,
  chain,
  phrase,
};

struct AppScreen {
  void (*init)(void);
  void (*setup)(int input);
  void (*fullRedraw)(void);
  void (*draw)(void);
  int (*onInput)(int isKeyDown, int keys, int tapCount); // Return 1 if handled, 0 if not
  ScreenPlaybackLevel (*getPlaybackLevel)(void); // Return playback level for this screen
};

struct ScreenData {
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
  ScreenPlaybackLevel playbackLevel; // Playback level for this screen (None, Song, Chain, Phrase)
  int (*getColumnCount)(int row);
  void (*drawStatic)(void);
  void (*drawCursor)(int col, int row);
  void (*drawSelection)(int col1, int row1, int col2, int row2);
  void (*drawRowHeader)(int row, CellState state);
  void (*drawColHeader)(int col, CellState state);
  void (*drawField)(int col, int row, CellState state);
  int (*onEdit)(int col, int row, CellEditAction action);
  int (*onInput)(int isKeyDown, int keys, int tapCount);  // Optional: handle input before standard processing (return 1 if handled completely, 0 to continue)
  int (*onRawInput)(int keyCode, int isKeyboard, int isDown);  // Optional: capture raw SDL input
  int (*isCellValid)(int col, int row);  // Optional: return 0 for dead cells that cursor should skip
  LoopRange (*getLoopRange)(void);  // Optional: return loop range for ranged playback
};

extern const AppScreen screenProject;
extern const AppScreen screenProjectLoad;
extern const AppScreen screenProjectSave;
extern const AppScreen screenConfirm;
extern const AppScreen screenPitchTable;
extern const AppScreen screenFileBrowser;
extern const AppScreen screenCreateFolder;
extern const AppScreen screenEnterName;
extern const AppScreen screenSong;
extern const AppScreen screenChain;
extern const AppScreen screenPhrase;
extern const AppScreen screenGroove;
extern const AppScreen screenInstrument;
extern const AppScreen screenInstrumentPool;
extern const AppScreen screenModulation;
extern const AppScreen screenTable;
extern const AppScreen screenWavetable;
extern const AppScreen screenExport;
extern const AppScreen screenManage;
extern const AppScreen screenSettings;
extern const AppScreen screenColorTheme;
extern const AppScreen screenKeyMapping;

extern const AppScreen* currentScreen;

void screenSetup(const AppScreen* screen, int input);
void screenDraw(void);
void screenMessage(int time, const char* format, ...);
void screensInitAll(void);
void drawScreenMap(void);
enum ScreenPlaybackLevel screenGetPlaybackLevel(const AppScreen* screen);

// Spreadsheet functions
void screenFullRedraw(ScreenData* screen);
void screenDrawOverlays(ScreenData* screen);
int screenInput(ScreenData* screen, int isKeyDown, int keys, int tapCount);

// Utility functions
void setCellColor(CellState state, int isEmpty, int hasContent);
void getSelectionBounds(ScreenData* screen, int* startCol, int* startRow, int* endCol, int* endRow);
int isSingleColumnSelection(ScreenData* screen);
LoopRange screenGetLoopRange(const AppScreen* screen);

// Confirmation dialog
void confirmSetup(const char* message, void (*confirmCallback)(void), void (*cancelCallback)(void));

// Common edit functions
int edit16withLimit(CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max);
int edit8withLimit(CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max);
int edit8noLimit(CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep);
int edit8noLast(CellEditAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max);
int editSigned16(CellEditAction action, int16_t* value, int16_t bigStep, int16_t min, int16_t max);
int editSigned8(CellEditAction action, int8_t* value, int8_t bigStep, int8_t min, int8_t max);
int edit16withMinMax(CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max);
int edit16withOverflow(CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max);
int applyMultiEdit(int startCol, int startRow, int endCol, int endRow, CellEditAction action, int (*editFunc)(int col, int row, CellEditAction action));
int applyPhraseRotation(int phraseIdx, int startRow, int endRow, int direction);
int applyTableRotation(int tableIdx, int startRow, int endRow, int direction);
int applySongMoveDown(int startCol, int startRow, int endCol, int endRow);
int applySongMoveUp(int startCol, int startRow, int endCol, int endRow);
CellEditAction convertMultiAction(CellEditAction action);

// Character edit
int editCharacter(CellEditAction action, char* str, int idx, int maxLen);
char charEditInput(int keys, int tapCount, char* str, int idx, int maxLen);

// FX edit
int editFX(CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable, uint8_t instrumentIdx);
int editFXValue(CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable, uint8_t instrumentIdx);
int fxEditInput(int keys, int tapCount, uint8_t* fx, uint8_t* lastFX);
void fxEditFullDraw(uint8_t currentFX, uint8_t instrumentIdx);

// Manage screen functions
// TODO: Remove this
int manageColumnCount(int row);
void manageDrawStatic(void);
void manageDrawCursor(int col, int row);
void manageDrawField(int col, int row, CellState state);
int manageOnEdit(int col, int row, CellEditAction action);

#ifdef __cplusplus
}
#endif

#endif
