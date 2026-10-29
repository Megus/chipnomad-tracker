#include "edit_common.h"
#include "chipnomad_lib.h"

EditCellAction convertMultiAction(EditCellAction action) {
  if (action == EditCellAction::multiIncrease) return EditCellAction::increase;
  if (action == EditCellAction::multiDecrease) return EditCellAction::decrease;
  if (action == EditCellAction::multiIncreaseBig) return EditCellAction::increaseBig;
  if (action == EditCellAction::multiDecreaseBig) return EditCellAction::decreaseBig;
  return action;
}

static bool isMultiAction(EditCellAction action) {
  return action == EditCellAction::multiIncrease || action == EditCellAction::multiDecrease || action == EditCellAction::multiIncreaseBig || action == EditCellAction::multiDecreaseBig;
}

bool applyMultiEdit(Rect range, EditCellAction action, EditCellInterface* editor) {
  if (!isMultiAction(action)) return false;

  for (int r = range.y; r < range.y + range.h; r++) {
    for (int c = range.x; c < range.x + range.w; c++) {
      editor->editCell(c, r, action);
    }
  }
  return true;
}

bool edit16withLimit(EditCellAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max) {
  bool handled = false;
  int isNotMultiAction = !isMultiAction(action);
  action = convertMultiAction(action);

  switch (action) {
    case EditCellAction::clear:
      if (*value != EMPTY_VALUE_16) {
        if (*value <= max) *lastValue = *value;
        *value = EMPTY_VALUE_16;
      }
      handled = true;
      break;
    case EditCellAction::tap:
      if (*value == EMPTY_VALUE_16) *value = *lastValue;
      handled = true;
      break;
    case EditCellAction::increase:
      if (*value != EMPTY_VALUE_16 && *value < max) *value += 1;
      handled = true;
      break;
    case EditCellAction::decrease:
      if (*value != EMPTY_VALUE_16 && *value > 0) *value -= 1;
      handled = true;
      break;
    case EditCellAction::increaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value > max - bigStep ? max : *value + bigStep;
      handled = true;
      break;
    case EditCellAction::decreaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value < bigStep ? 0 : *value - bigStep;
      handled = true;
      break;
    default:
      break;
  }

  if (isNotMultiAction && handled && *value != EMPTY_VALUE_16 && *value <= max) {
    *lastValue = *value;
  }

  return handled;
}

bool edit8withLimit(EditCellAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t max) {
  uint16_t value16 = *value;
  uint16_t lastValue16 = *lastValue;

  if (value16 == EMPTY_VALUE_8) {
    value16 = EMPTY_VALUE_16;
  }

  bool handled = edit16withLimit(action, &value16, &lastValue16, bigStep, max);

  *value = (uint8_t)value16;
  *lastValue = (uint8_t)lastValue16;

  return handled;
}

bool edit8noLast(EditCellAction action, uint8_t* value, uint8_t bigStep, uint8_t min, uint8_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case EditCellAction::tap:
      return true;
      break;
    case EditCellAction::clear:
      *value = min;
      return true;
      break;
    case EditCellAction::increase:
      if (*value < max) *value += 1;
      return true;
      break;
    case EditCellAction::decrease:
      if (*value > min) *value -= 1;
      return true;
      break;
    case EditCellAction::increaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return true;
      break;
    case EditCellAction::decreaseBig:
      *value = *value < bigStep + min ? min : *value - bigStep;
      return true;
      break;
    default:
      break;
  }

  return false;
}

bool edit8noLimit(EditCellAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep) {
  bool handled = false;
  bool isNotMultiAction = !isMultiAction(action);
  action = convertMultiAction(action);

  switch (action) {
    case EditCellAction::clear:
      if (*value != 0) {
        *lastValue = *value;
        *value = 0;
      }
      handled = true;
      break;
    case EditCellAction::tap:
      if (*value == 0) *value = *lastValue;
      handled = true;
      break;
    case EditCellAction::increase:
      *value += 1;
      handled = true;
      break;
    case EditCellAction::decrease:
      *value -= 1;
      handled = true;
      break;
    case EditCellAction::increaseBig:
      *value += bigStep;
      handled = true;
      break;
    case EditCellAction::decreaseBig:
      *value -= bigStep;
      handled = true;
      break;
    default:
      break;
  }

  if (isNotMultiAction && handled) {
    *lastValue = *value;
  }

  return handled;
}

bool editSigned16(EditCellAction action, int16_t* value, int16_t bigStep, int16_t min, int16_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case EditCellAction::tap:
      return true;
    case EditCellAction::clear:
      *value = 0;
      return true;
    case EditCellAction::increase:
      if (*value < max) *value += 1;
      return true;
    case EditCellAction::decrease:
      if (*value > min) *value -= 1;
      return true;
    case EditCellAction::increaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return true;
    case EditCellAction::decreaseBig:
      *value = *value < min + bigStep ? min : *value - bigStep;
      return true;
    default:
      break;
  }
  return false;
}

bool editSigned8(EditCellAction action, int8_t* value, int8_t bigStep, int8_t min, int8_t max) {
  int16_t value16 = *value;
  bool handled = editSigned16(action, &value16, bigStep, min, max);
  *value = (int8_t)value16;
  return handled;
}

bool edit16withMinMax(EditCellAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max) {
  action = convertMultiAction(action);

  switch (action) {
    case EditCellAction::tap:
      return true;
    case EditCellAction::clear:
      *value = min;
      return true;
    case EditCellAction::increase:
      if (*value < max) *value += 1;
      return true;
    case EditCellAction::decrease:
      if (*value > min) *value -= 1;
      return true;
    case EditCellAction::increaseBig:
      *value = *value > max - bigStep ? max : *value + bigStep;
      return true;
    case EditCellAction::decreaseBig:
      *value = *value < min + bigStep ? min : *value - bigStep;
      return true;
    default:
      break;
  }
  return false;
}

bool edit16withOverflow(EditCellAction action, uint16_t* value, uint16_t bigStep, uint16_t min, uint16_t max) {
  switch (action) {
    case EditCellAction::tap:
      return true;
    case EditCellAction::clear:
      *value = min;
      return true;
    case EditCellAction::increase:
      *value = (*value >= max) ? min : *value + 1;
      return true;
    case EditCellAction::decrease:
      *value = (*value <= min) ? max : *value - 1;
      return true;
    case EditCellAction::increaseBig:
      *value = (*value > max - bigStep) ? min + (*value + bigStep - max - 1) : *value + bigStep;
      return true;
    case EditCellAction::decreaseBig:
      *value = (*value < min + bigStep) ? max - (min + bigStep - *value - 1) : *value - bigStep;
      return true;
    default:
      break;
  }
  return false;
}

bool applyPhraseRotation(chipnomad::Phrase& p, int startRow, int endRow, int direction) {
  using namespace chipnomad;

  if (startRow == endRow) return false;

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

  return true;
}

bool applyTableRotation(chipnomad::Table& t, int startRow, int endRow, int direction) {
  using namespace chipnomad;

  if (startRow == endRow) return false;

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

  return true;
}

bool applySongMoveDown(chipnomad::Project& p, Rect range) {
  // Check if we can move down (bottom row must be empty)
  for (int c = range.x; c < range.x + range.w; c++) {
    if (p.song[PROJECT_MAX_LENGTH - 1][c] != EMPTY_VALUE_16) {
      return false;
    }
  }

  // Push everything below selection down by 1 row (from bottom up)
  for (int c = range.x; c < range.x + range.w; c++) {
    for (int r = PROJECT_MAX_LENGTH - 1; r > range.y + range.h; r--) {
      p.song[r][c] = p.song[r - 1][c];
      p.songHighlight[r][c] = p.songHighlight[r - 1][c];
    }
  }

  // Move selection down by 1 row
  for (int c = range.x; c <= range.x + range.w; c++) {
    for (int r = range.y + range.h - 1; r >= range.y; r--) {
      p.song[r + 1][c] = p.song[r][c];
      p.songHighlight[r + 1][c] = p.songHighlight[r][c];
    }
    p.song[range.y][c] = EMPTY_VALUE_16;
    p.songHighlight[range.y][c] = 0;
  }

  return true;
}

bool applySongMoveUp(chipnomad::Project& p, Rect range) {
  // Check if we can move up (space above selection must be empty)
  if (range.y == 0) return false;

  for (int c = range.x; c < range.x + range.w; c++) {
    if (p.song[range.y - 1][c] != EMPTY_VALUE_16) {
      return false; // Can't move up
    }
  }

  // Move selection up
  for (int c = range.x; c < range.x + range.w; c++) {
    for (int r = range.y; r < range.y + range.h - 1; r++) {
      p.song[r][c] = p.song[r + 1][c];
      p.songHighlight[r][c] = p.songHighlight[r + 1][c];
    }
    p.song[range.y + range.h - 1][c] = EMPTY_VALUE_16;
    p.songHighlight[range.y + range.h - 1][c] = 0;
  }

  return true;
}

void shiftSongColumnUp(chipnomad::Project &p, int col, int startRow) {
  for (int row = startRow; row < PROJECT_MAX_LENGTH - 1; row++) {
    p.song[row][col] = p.song[row + 1][col];
  }
  p.song[PROJECT_MAX_LENGTH - 1][col] = EMPTY_VALUE_16;
}

