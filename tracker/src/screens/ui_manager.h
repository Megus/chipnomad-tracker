#ifndef __SCREENS_H__
#define __SCREENS_H__

#include "tracker_state.h"
#include "chipnomad_lib.h"

#define MESSAGE_TIME (60)

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

// Spreadsheet functions
int screenInput(ScreenData* screen, int isKeyDown, int keys, int tapCount);

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

#endif
