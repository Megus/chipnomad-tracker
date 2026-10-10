#ifndef __EDIT_COMMON_H__
#define __EDIT_COMMON_H__

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

#endif // __EDIT_COMMON_H__