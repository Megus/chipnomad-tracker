#include <screens.h>
#include <corelib_gfx.h>

uint8_t currentFX;

void fxEditFullDraw();


int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastValue) {
  if (action == editClear) {
    // Clear FX
    if (fx[0] != EMPTY_VALUE_8) {
      lastValue[0] = fx[0];
      lastValue[1] = fx[1];
    }
    fx[0] = EMPTY_VALUE_8;
    fx[1] = 0;
    return 2;
  } else if (action == editTap) {
    // Insert last FX
    if (fx[0] == EMPTY_VALUE_8) {
      fx[0] = lastValue[0];
      fx[1] = lastValue[1];
      lastValue[0] = fx[0];
      lastValue[1] = fx[1];
    }
  return 2;
  } else if (action == editIncrease) {
    // Next FX
    if (fx[0] < fxTotalCount - 1) fx[0]++;
    lastValue[0] = fx[0];
    return 2;
  } else if (action == editDecrease) {
    // Previous FX
    if (fx[0] > 0) fx[0]--;
    lastValue[0] = fx[0];
    return 2;
  } else if (action == editIncreaseBig || action == editDecreaseBig) {
    // Show FX select screen
    currentFX = fx[0];
    fxEditFullDraw();
    return 1;
  }
  return 0;
}

int fxEditInput(int keys, int isDoubleTap, uint8_t* fx, uint8_t* lastFX) {
  if (keys == 0) {
    return 1;
  }

  return 0;
}

void fxEditFullDraw() {
  gfxClearRect(0, 0, 40, 20);

}