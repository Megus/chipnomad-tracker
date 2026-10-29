#ifndef __EDIT_COMMON_H__
#define __EDIT_COMMON_H__

#include <stdint.h>
#include "chipnomad_lib.h"
#include "gfx.h"

enum class EditCellAction : int {
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

class EditCellInterface {
  public:
    virtual bool editCell(int col, int row, EditCellAction action) = 0;
};

// Common edit functions
bool edit16withLimit(EditCellAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max);
bool edit8withLimit(EditCellAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max);
bool edit8noLimit(EditCellAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep);
bool edit8noLast(EditCellAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max);
bool editSigned16(EditCellAction action, int16_t* value, int16_t bigStep, int16_t min, int16_t max);
bool editSigned8(EditCellAction action, int8_t* value, int8_t bigStep, int8_t min, int8_t max);
bool edit16withMinMax(EditCellAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max);
bool edit16withOverflow(EditCellAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max);
bool applyMultiEdit(Rect range, EditCellAction action, EditCellInterface* editor);
bool applyPhraseRotation(chipnomad::Phrase& p, int startRow, int endRow, int direction);
bool applyTableRotation(chipnomad::Table& t, int startRow, int endRow, int direction);
bool applySongMoveDown(chipnomad::Project& p, Rect range);
bool applySongMoveUp(chipnomad::Project& p, Rect range);
void shiftSongColumnUp(chipnomad::Project &project, int col, int startRow);
EditCellAction convertMultiAction(EditCellAction action);

#endif // __EDIT_COMMON_H__