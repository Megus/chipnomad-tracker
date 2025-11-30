#include "screens.h"
#include "chipnomad_lib.h"

enum CellEditAction convertMultiAction(enum CellEditAction action) {
  if (action == editMultiIncrease) return editIncrease;
  if (action == editMultiDecrease) return editDecrease;
  if (action == editMultiIncreaseBig) return editIncreaseBig;
  if (action == editMultiDecreaseBig) return editDecreaseBig;
  return action;
}

int applyMultiEdit(ScreenData* screen, enum CellEditAction action, int (*editFunc)(int col, int row, enum CellEditAction action)) {
  if (action != editMultiIncrease && action != editMultiDecrease &&
    action != editMultiIncreaseBig && action != editMultiDecreaseBig) return 0;

  int startCol, startRow, endCol, endRow;
  getSelectionBounds(screen, &startCol, &startRow, &endCol, &endRow);

  for (int r = startRow; r <= endRow; r++) {
    for (int c = startCol; c <= endCol; c++) {
      editFunc(c, r, action);
    }
  }
  return 1;
}

int edit16withLimit(enum CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max) {
  int handled = 0;
  action = convertMultiAction(action);

  switch (action) {
    case editClear:
      if (*value != EMPTY_VALUE_16) {
        if (*value <= max) *lastValue = *value;
        *value = EMPTY_VALUE_16;
      }
      handled = 1;
      break;
    case editTap:
      if (*value == EMPTY_VALUE_16) *value = *lastValue;
      handled = 1;
      break;
    case editIncrease:
      if (*value != EMPTY_VALUE_16 && *value < max) *value += 1;
      handled = 1;
      break;
    case editDecrease:
      if (*value != EMPTY_VALUE_16 && *value > 0) *value -= 1;
      handled = 1;
      break;
    case editIncreaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value > max - bigStep ? max : *value + bigStep;
      handled = 1;
      break;
    case editDecreaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value < bigStep ? 0 : *value - bigStep;
      handled = 1;
      break;
    default:
      break;
  }

  if (handled && *value != EMPTY_VALUE_16 && *value < max) {
    *lastValue = *value;
  }

  return handled;
}

int edit8withLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max) {
  uint16_t value16 = *value;
  uint16_t lastValue16 = *lastValue;

  if (value16 == EMPTY_VALUE_8) {
    value16 = EMPTY_VALUE_16;
  }

  int handled = edit16withLimit(action, &value16, &lastValue16, bigStep, max);

  *value = (uint8_t)value16;
  *lastValue = (uint8_t)lastValue16;

  return handled;
}

int edit8noLast(enum CellEditAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case editTap:
      return 1;
      break;
    case editClear:
      *value = 0;
      return 1;
      break;
    case editIncrease:
      if (*value < max) *value += 1;
      return 1;
      break;
    case editDecrease:
      if (*value > min) *value -= 1;
      return 1;
      break;
    case editIncreaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return 1;
      break;
    case editDecreaseBig:
      *value = *value < bigStep + min ? min : *value - bigStep;
      return 1;
      break;
    default:
      break;
  }

  return 0;
}

int edit8noLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep) {
  int handled = 0;
  action = convertMultiAction(action);

  switch (action) {
    case editClear:
      if (*value != 0) {
        *lastValue = *value;
        *value = 0;
      }
      handled = 1;
      break;
    case editTap:
      if (*value == 0) *value = *lastValue;
      handled = 1;
      break;
    case editIncrease:
      *value += 1;
      handled = 1;
      break;
    case editDecrease:
      *value -= 1;
      handled = 1;
      break;
    case editIncreaseBig:
      *value += bigStep;
      handled = 1;
      break;
    case editDecreaseBig:
      *value -= bigStep;
      handled = 1;
      break;
    default:
      break;
  }

  if (handled) {
    *lastValue = *value;
  }

  return handled;
}

int edit16withOverflow(enum CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max) {
  switch (action) {
    case editTap:
      return 1;
    case editClear:
      *value = min;
      return 1;
    case editIncrease:
      *value = (*value >= max) ? min : *value + 1;
      return 1;
    case editDecrease:
      *value = (*value <= min) ? max : *value - 1;
      return 1;
    case editIncreaseBig:
      *value = (*value > max - bigStep) ? min + (*value + bigStep - max - 1) : *value + bigStep;
      return 1;
    case editDecreaseBig:
      *value = (*value < min + bigStep) ? max - (min + bigStep - *value - 1) : *value - bigStep;
      return 1;
    default:
      break;
  }
  return 0;
}

int applyPhraseRotation(int phraseIdx, int startRow, int endRow, int direction) {
  if (startRow == endRow) return 0;

  PhraseRow* rows = chipnomadState->project.phrases[phraseIdx].rows;

  if (direction > 0) {
    // Rotate down: move last row to first
    PhraseRow temp = rows[endRow];
    for (int r = endRow; r > startRow; r--) {
      rows[r] = rows[r - 1];
    }
    rows[startRow] = temp;
  } else {
    // Rotate up: move first row to last
    PhraseRow temp = rows[startRow];
    for (int r = startRow; r < endRow; r++) {
      rows[r] = rows[r + 1];
    }
    rows[endRow] = temp;
  }

  return 1;
}

int applyTableRotation(int tableIdx, int startRow, int endRow, int direction) {
  if (startRow == endRow) return 0;

  TableRow* rows = chipnomadState->project.tables[tableIdx].rows;

  if (direction > 0) {
    // Rotate down: move last row to first
    TableRow temp = rows[endRow];
    for (int r = endRow; r > startRow; r--) {
      rows[r] = rows[r - 1];
    }
    rows[startRow] = temp;
  } else {
    // Rotate up: move first row to last
    TableRow temp = rows[startRow];
    for (int r = startRow; r < endRow; r++) {
      rows[r] = rows[r + 1];
    }
    rows[endRow] = temp;
  }

  return 1;
}

int applySongMoveDown(int startCol, int startRow, int endCol, int endRow) {
  // Check if we can move down (bottom row must be empty)
  for (int c = startCol; c <= endCol; c++) {
    if (chipnomadState->project.song[PROJECT_MAX_LENGTH - 1][c] != EMPTY_VALUE_16) {
      return 0;
    }
  }

  // Push everything below selection down by 1 row (from bottom up)
  for (int c = startCol; c <= endCol; c++) {
    for (int r = PROJECT_MAX_LENGTH - 1; r > endRow + 1; r--) {
      chipnomadState->project.song[r][c] = chipnomadState->project.song[r - 1][c];
    }
  }

  // Move selection down by 1 row
  for (int c = startCol; c <= endCol; c++) {
    for (int r = endRow; r >= startRow; r--) {
      chipnomadState->project.song[r + 1][c] = chipnomadState->project.song[r][c];
    }
    chipnomadState->project.song[startRow][c] = EMPTY_VALUE_16;
  }

  return 1;
}

int applySongMoveUp(int startCol, int startRow, int endCol, int endRow) {
  // Check if we can move up (space above selection must be empty)
  if (startRow == 0) return 0;

  for (int c = startCol; c <= endCol; c++) {
    if (chipnomadState->project.song[startRow - 1][c] != EMPTY_VALUE_16) {
      return 0; // Can't move up
    }
  }

  // Move selection up
  for (int c = startCol; c <= endCol; c++) {
    for (int r = startRow - 1; r < endRow; r++) {
      chipnomadState->project.song[r][c] = chipnomadState->project.song[r + 1][c];
    }
    chipnomadState->project.song[endRow][c] = EMPTY_VALUE_16;
  }

  return 1;
}
