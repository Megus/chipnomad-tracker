#include <screens.h>
#include <project.h>

int edit16withLimit(enum CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t max) {
  int handled = 0;

  // Convert multi-edit actions to regular actions
  if (action == editMultiIncrease) action = editIncrease;
  if (action == editMultiDecrease) action = editDecrease;

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
  // Convert multi-edit actions to regular actions
  if (action == editMultiIncrease) action = editIncrease;
  if (action == editMultiDecrease) action = editDecrease;

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

  // Convert multi-edit actions to regular actions
  if (action == editMultiIncrease) action = editIncrease;
  if (action == editMultiDecrease) action = editDecrease;

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
