#include <screens.h>
#include <project.h>

int edit16withLimit(enum CellEditAction action, uint16_t* value, uint16_t* lastValue, uint16_t bigStep, uint16_t upperLimit) {
  int handled = 0;

  switch (action) {
    case editClear:
      if (*value != EMPTY_VALUE_16) {
        if (*value < upperLimit) *lastValue = *value;
        *value = EMPTY_VALUE_16;
      }
      handled = 1;
      break;
    case editTap:
      if (*value == EMPTY_VALUE_16) *value = *lastValue;
      handled = 1;
      break;
    case editIncrease:
      if (*value != EMPTY_VALUE_16 && *value < upperLimit - 1) *value += 1;
      handled = 1;
      break;
    case editDecrease:
      if (*value != EMPTY_VALUE_16 && *value > 0) *value -= 1;
      handled = 1;
      break;
    case editIncreaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value >= upperLimit - bigStep ? upperLimit - 1 : *value + bigStep;
      handled = 1;
      break;
    case editDecreaseBig:
      if (*value != EMPTY_VALUE_16) *value = *value < bigStep ? 0 : *value - bigStep;
      handled = 1;
      break;
    default:
      break;
  }

  if (handled && *value != EMPTY_VALUE_16 && *value < upperLimit) {
    *lastValue = *value;
  }

  return handled;
}

int edit8withLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep, uint8_t upperLimit) {
  uint16_t value16 = *value;
  uint16_t lastValue16 = *lastValue;

  if (value16 == EMPTY_VALUE_8) {
    value16 = EMPTY_VALUE_16;
  }

  int handled = edit16withLimit(action, &value16, &lastValue16, bigStep, upperLimit);

  *value = (uint8_t)value16;
  *lastValue = (uint8_t)lastValue16;

  return handled;
}

int edit8noLimit(enum CellEditAction action, uint8_t* value, uint8_t* lastValue, uint8_t bigStep) {
  int handled = 0;

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

int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastValue) {
  return 0;
}

int editCharacter(enum CellEditAction action, char* ch) {
  return 0;
}