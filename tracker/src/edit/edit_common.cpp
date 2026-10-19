#include "edit_common.h"
#include "chipnomad_lib.h"

CellEditAction convertMultiAction(CellEditAction action) {
  if (action == CellEditAction::multiIncrease) return CellEditAction::increase;
  if (action == CellEditAction::multiDecrease) return CellEditAction::decrease;
  if (action == CellEditAction::multiIncreaseBig) return CellEditAction::increaseBig;
  if (action == CellEditAction::multiDecreaseBig) return CellEditAction::decreaseBig;
  return action;
}

int isMultiAction(CellEditAction action) {
  return action == CellEditAction::multiIncrease || action == CellEditAction::multiDecrease || action == CellEditAction::multiIncreaseBig || action == CellEditAction::multiDecreaseBig;
}

int applyMultiEdit(int startCol, int startRow, int endCol, int endRow, CellEditAction action, int (*editFunc)(int col, int row, CellEditAction action)) {
  if (!isMultiAction(action)) return 0;

  for (int r = startRow; r <= endRow; r++) {
    for (int c = startCol; c <= endCol; c++) {
      editFunc(c, r, action);
    }
  }
  return 1;
}

int edit16withLimit(CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max) {
  int handled = 0;
  int isNotMultiAction = !isMultiAction(action);
  action = convertMultiAction(action);

  switch (action) {
    case CellEditAction::clear:
      if (*value != EMPTY_VALUE_16) {
        if (*value <= max) *lastValue = *value;
        *value = EMPTY_VALUE_16;
      }
      handled = 1;
      break;
    case CellEditAction::tap:
      if (*value == EMPTY_VALUE_16) *value = *lastValue;
      handled = 1;
      break;
    case CellEditAction::increase:
      if (*value != EMPTY_VALUE_16 && *value < max) *value += 1;
      handled = 1;
      break;
    case CellEditAction::decrease:
      if (*value != EMPTY_VALUE_16 && *value > 0) *value -= 1;
      handled = 1;
      break;
    case CellEditAction::increaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value > max - bigStep ? max : *value + bigStep;
      handled = 1;
      break;
    case CellEditAction::decreaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value < bigStep ? 0 : *value - bigStep;
      handled = 1;
      break;
    default:
      break;
  }

  if (isNotMultiAction && handled && *value != EMPTY_VALUE_16 && *value <= max) {
    *lastValue = *value;
  }

  return handled;
}

int edit8withLimit(CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max) {
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

int edit8noLast(CellEditAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case CellEditAction::tap:
      return 1;
      break;
    case CellEditAction::clear:
      *value = min;
      return 1;
      break;
    case CellEditAction::increase:
      if (*value < max) *value += 1;
      return 1;
      break;
    case CellEditAction::decrease:
      if (*value > min) *value -= 1;
      return 1;
      break;
    case CellEditAction::increaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return 1;
      break;
    case CellEditAction::decreaseBig:
      *value = *value < bigStep + min ? min : *value - bigStep;
      return 1;
      break;
    default:
      break;
  }

  return 0;
}

int edit8noLimit(CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep) {
  int handled = 0;
  int isNotMultiAction = !isMultiAction(action);
  action = convertMultiAction(action);

  switch (action) {
    case CellEditAction::clear:
      if (*value != 0) {
        *lastValue = *value;
        *value = 0;
      }
      handled = 1;
      break;
    case CellEditAction::tap:
      if (*value == 0) *value = *lastValue;
      handled = 1;
      break;
    case CellEditAction::increase:
      *value += 1;
      handled = 1;
      break;
    case CellEditAction::decrease:
      *value -= 1;
      handled = 1;
      break;
    case CellEditAction::increaseBig:
      *value += bigStep;
      handled = 1;
      break;
    case CellEditAction::decreaseBig:
      *value -= bigStep;
      handled = 1;
      break;
    default:
      break;
  }

  if (isNotMultiAction && handled) {
    *lastValue = *value;
  }

  return handled;
}

int editSigned16(CellEditAction action, int16_t* value, int16_t bigStep, int16_t min, int16_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case CellEditAction::tap:
      return 1;
    case CellEditAction::clear:
      *value = 0;
      return 1;
    case CellEditAction::increase:
      if (*value < max) *value += 1;
      return 1;
    case CellEditAction::decrease:
      if (*value > min) *value -= 1;
      return 1;
    case CellEditAction::increaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return 1;
    case CellEditAction::decreaseBig:
      *value = *value < min + bigStep ? min : *value - bigStep;
      return 1;
    default:
      break;
  }
  return 0;
}

int editSigned8(CellEditAction action, int8_t* value, int8_t bigStep, int8_t min, int8_t max) {
  int16_t value16 = *value;
  int result = editSigned16(action, &value16, bigStep, min, max);
  *value = (int8_t)value16;
  return result;
}

int edit16withMinMax(CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case CellEditAction::tap:
      return 1;
    case CellEditAction::clear:
      *value = min;
      return 1;
    case CellEditAction::increase:
      if (*value < max) *value += 1;
      return 1;
    case CellEditAction::decrease:
      if (*value > min) *value -= 1;
      return 1;
    case CellEditAction::increaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return 1;
    case CellEditAction::decreaseBig:
      *value = *value < min + bigStep ? min : *value - bigStep;
      return 1;
    default:
      break;
  }
  return 0;
}

int edit16withOverflow(CellEditAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max) {
  switch (action) {
    case CellEditAction::tap:
      return 1;
    case CellEditAction::clear:
      *value = min;
      return 1;
    case CellEditAction::increase:
      *value = (*value >= max) ? min : *value + 1;
      return 1;
    case CellEditAction::decrease:
      *value = (*value <= min) ? max : *value - 1;
      return 1;
    case CellEditAction::increaseBig:
      *value = (*value > max - bigStep) ? min + (*value + bigStep - max - 1) : *value + bigStep;
      return 1;
    case CellEditAction::decreaseBig:
      *value = (*value < min + bigStep) ? max - (min + bigStep - *value - 1) : *value - bigStep;
      return 1;
    default:
      break;
  }
  return 0;
}

int applyPhraseRotation(chipnomad::Phrase& p, int startRow, int endRow, int direction) {
  using namespace chipnomad;

  if (startRow == endRow) return 0;

  if (direction > 0) {
    // Rotate down: move last row to first
    PhraseRow temp = p.rows[endRow];
    for (int r = endRow; r > startRow; r--) {
      p.rows[r] = p.rows[r - 1];
    }
    p.rows[startRow] = temp;
  } else {
    // Rotate up: move first row to last
    PhraseRow temp = p.rows[startRow];
    for (int r = startRow; r < endRow; r++) {
      p.rows[r] = p.rows[r + 1];
    }
    p.rows[endRow] = temp;
  }

  return 1;
}

int applyTableRotation(chipnomad::Table& t, int startRow, int endRow, int direction) {
  using namespace chipnomad;

  if (startRow == endRow) return 0;

  if (direction > 0) {
    // Rotate down: move last row to first
    TableRow temp = t.rows[endRow];
    for (int r = endRow; r > startRow; r--) {
      t.rows[r] = t.rows[r - 1];
    }
    t.rows[startRow] = temp;
  } else {
    // Rotate up: move first row to last
    TableRow temp = t.rows[startRow];
    for (int r = startRow; r < endRow; r++) {
      t.rows[r] = t.rows[r + 1];
    }
    t.rows[endRow] = temp;
  }

  return 1;
}

int applySongMoveDown(chipnomad::Project& p, int startCol, int startRow, int endCol, int endRow) {
  // Check if we can move down (bottom row must be empty)
  for (int c = startCol; c <= endCol; c++) {
    if (p.song[PROJECT_MAX_LENGTH - 1][c] != EMPTY_VALUE_16) {
      return 0;
    }
  }

  // Push everything below selection down by 1 row (from bottom up)
  for (int c = startCol; c <= endCol; c++) {
    for (int r = PROJECT_MAX_LENGTH - 1; r > endRow + 1; r--) {
      p.song[r][c] = p.song[r - 1][c];
      p.songHighlight[r][c] = p.songHighlight[r - 1][c];
    }
  }

  // Move selection down by 1 row
  for (int c = startCol; c <= endCol; c++) {
    for (int r = endRow; r >= startRow; r--) {
      p.song[r + 1][c] = p.song[r][c];
      p.songHighlight[r + 1][c] = p.songHighlight[r][c];
    }
    p.song[startRow][c] = EMPTY_VALUE_16;
    p.songHighlight[startRow][c] = 0;
  }

  return 1;
}

int applySongMoveUp(chipnomad::Project& p, int startCol, int startRow, int endCol, int endRow) {
  // Check if we can move up (space above selection must be empty)
  if (startRow == 0) return 0;

  for (int c = startCol; c <= endCol; c++) {
    if (p.song[startRow - 1][c] != EMPTY_VALUE_16) {
      return 0; // Can't move up
    }
  }

  // Move selection up
  for (int c = startCol; c <= endCol; c++) {
    for (int r = startRow - 1; r < endRow; r++) {
      p.song[r][c] = p.song[r + 1][c];
      p.songHighlight[r][c] = p.songHighlight[r + 1][c];
    }
    p.song[endRow][c] = EMPTY_VALUE_16;
    p.songHighlight[endRow][c] = 0;
  }

  return 1;
}
