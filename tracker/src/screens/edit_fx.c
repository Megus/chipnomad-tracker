#include "screens.h"
#include "corelib_gfx.h"
#include "help.h"

#define FX_COLS 8

int currentGroup;
int currentIdx;

// FX Groups and layout
static const char* fxHeaders[] = {
  "Sequencer FX",
  "AY FX"
};

void fxEditFullDraw(uint8_t currentFX);

int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastValue, int isTable) {
  int result = 0;
  action = convertMultiAction(action);

  if (action == editClear) {
    // Clear FX
    if (fx[0] != EMPTY_VALUE_8) {
      lastValue[0] = fx[0];
      lastValue[1] = fx[1];
    }
    fx[0] = EMPTY_VALUE_8;
    fx[1] = 0;
    result = 2;
  } else if (action == editTap) {
    // Insert last FX
    if (fx[0] == EMPTY_VALUE_8) {
      fx[0] = lastValue[0];
      fx[1] = lastValue[1];
    }
    lastValue[0] = fx[0];
    lastValue[1] = fx[1];
    result = 2;
  } else if (action == editIncrease) {
    // Next FX
    if (fx[0] < fxTotalCount - 1) fx[0]++;
    lastValue[0] = fx[0];
    result = 2;
  } else if (action == editDecrease) {
    // Previous FX
    if (fx[0] > 0) fx[0]--;
    lastValue[0] = fx[0];
    result = 2;
  } else if (action == editIncreaseBig || action == editDecreaseBig) {
    // Show FX select screen
    fxEditFullDraw(fx[0]);
    result = 1;
  }
  if (result != 1) screenMessage(0, "%s", helpFXHint(fx, isTable));
  return result;
}

int editFXValue(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable) {
  action = convertMultiAction(action);

  uint8_t bigStep = 16;
  if (fx[0] == fxENT) {
    // AY Envelope note
    bigStep = chipnomadState->project.pitchTable.octaveSize;
  }

  int handled = edit8noLimit(action, &fx[1], &lastFX[1], bigStep);
  screenMessage(0, helpFXHint(fx, 0));
  return handled;
}

// Uses the ordered FX list
struct FXName* getGroup(int group) {
  return group == 0 ? fxNamesCommon : fxNamesAY;
}

int getGroupSize(int group) {
  return group == 0 ? fxCommonCount : fxAYCount;
}

void drawGroupHeader(int group) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);

  int y = (group == 0) ? 7 : 13;

  gfxPrint(1, y, fxHeaders[group]);
}

void drawFX(int group, int idx, int state) {
  int y = (group == 0) ? 8 : 14;

  gfxSetFgColor(state == stateFocus? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  int row = idx / FX_COLS;
  int col = idx % FX_COLS;

  gfxPrint(1 + col * 4, 1 + y + row, getGroup(group)[idx].name);

  if (state == stateFocus) {
    gfxCursor(1 + col * 4, 1 + y + row, 3);
  }
}

void fxEditFullDraw(uint8_t currentFX) {
  gfxClearRect(0, 0, 35, 20);
  currentGroup = 0;
  currentIdx = 0;

  // Draw help for current FX
  drawFXHelp((enum FX)currentFX);

  // Draw each FX group
  for (int groupIdx = 0; groupIdx < 2; groupIdx++) {
    drawGroupHeader(groupIdx);
    struct FXName *group = getGroup(groupIdx);

    for (int idx = 0; idx < getGroupSize(groupIdx); idx++) {
      if (currentFX == group[idx].fx) {
        currentGroup = groupIdx;
        currentIdx = idx;
      }

      drawFX(groupIdx, idx, group[idx].fx == currentFX);
    }
  }
}


int fxEditInput(int keys, int isDoubleTap, uint8_t* fx, uint8_t* lastFX) {
  if (keys == 0) {
    // Selection complete - update the FX value
    fx[0] = getGroup(currentGroup)[currentIdx].fx;
    lastFX[0] = fx[0];
    return 1;
  }

  if (keys & keyEdit) {
    drawFX(currentGroup, currentIdx, 0);

    if (keys & keyRight) {
      if (currentIdx < getGroupSize(currentGroup) - 1) {
        currentIdx++;
      } else if (currentGroup < 1) {
        currentGroup++;
        currentIdx = 0;
      }
    } else if (keys & keyLeft) {
      if (currentIdx > 0) {
        currentIdx--;
      } else if (currentGroup > 0) {
        currentGroup--;
        currentIdx = getGroupSize(currentGroup) - 1;
      }
    } else if (keys & keyUp) {
      currentIdx -= FX_COLS;
      if (currentIdx < 0) {
        if (currentGroup > 0) {
          currentGroup--;
          currentIdx = getGroupSize(currentGroup) - 1;
        } else {
          currentIdx = 0;
        }
      }
    } else if (keys & keyDown) {
      currentIdx += FX_COLS;
      if (currentIdx >= getGroupSize(currentGroup)) {
        if (currentGroup < 1) {
          currentGroup++;
          currentIdx = 0;
        } else {
          currentIdx = getGroupSize(currentGroup) - 1;
        }
      }
    }
    drawFX(currentGroup, currentIdx, stateFocus);

    // Update help text for newly selected FX
    gfxClearRect(0, 1, 35, 5);
    drawFXHelp(getGroup(currentGroup)[currentIdx].fx);
  }

  return 0;
}
