#include "screens.h"
#include "corelib_gfx.h"
#include "help.h"

// State for FX selection screen
int currentGroup;      // Current group being navigated
int currentIdx;        // Current FX index within group
int expandedGroup;     // Currently expanded group (-1 = none)
uint8_t currentInstrumentIdx;  // Current instrument index for context-aware help

// Helper to get instrument type from stored instrument index
static enum InstrumentType getCurrentInstrumentType() {
  if (currentInstrumentIdx != EMPTY_VALUE_8 && currentInstrumentIdx < PROJECT_MAX_INSTRUMENTS) {
    return (enum InstrumentType)chipnomadState->project.instruments[currentInstrumentIdx].type;
  }
  return instNone;
}

void fxEditFullDraw(uint8_t currentFX, uint8_t instrumentIdx);

int editFX(enum CellEditAction action, uint8_t* fx, uint8_t* lastValue, int isTable, uint8_t instrumentIdx) {
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
  } else if (action == editIncrease && fx[0] != EMPTY_VALUE_8) {
    // Next FX
    if (fx[0] < fxTotalCount - 1) fx[0]++;
    lastValue[0] = fx[0];
    result = 2;
  } else if (action == editDecrease && fx[0] != EMPTY_VALUE_8) {
    // Previous FX
    if (fx[0] > 0) fx[0]--;
    lastValue[0] = fx[0];
    result = 2;
  } else if (action == editIncreaseBig || action == editDecreaseBig) {
    // Show FX select screen with instrument context
    fxEditFullDraw(fx[0], instrumentIdx);
    result = 1;
  }
  if (result != 1) screenMessage(0, "%s", helpFXHint(fx, isTable, instrumentIdx));
  return result;
}

int editFXValue(enum CellEditAction action, uint8_t* fx, uint8_t* lastFX, int isTable, uint8_t instrumentIdx) {
  action = convertMultiAction(action);

  uint8_t bigStep = 16;
  if (fx[0] == fxENT || fx[0] == fxTNN || fx[0] == fxENN || fx[0] == fxSFN) {
    // Note-setting FX: use octave size for big step
    bigStep = chipnomadState->project.pitchTable.octaveSize;
  }

  int handled = edit8noLimit(action, &fx[1], &lastFX[1], bigStep);
  screenMessage(0, helpFXHint(fx, 0, instrumentIdx));
  return handled;
}

// Helper functions for FX group management

// Get number of visible groups based on current instrument type
int getVisibleGroupCount(enum InstrumentType instType) {
  extern FXGroup fxGroups[];
  extern int fxGroupCount;

  int count = 0;
  for (int i = 0; i < fxGroupCount; i++) {
    // Show group if it's non-instrument (instNone) or matches current instrument type
    if (fxGroups[i].instType == instNone || fxGroups[i].instType == instType) {
      count++;
    }
  }
  return count;
}

// Get visible group by index (skips groups that don't match instrument type)
FXGroup* getVisibleGroup(int visibleIdx, enum InstrumentType instType) {
  extern FXGroup fxGroups[];
  extern int fxGroupCount;

  int visibleCount = 0;
  for (int i = 0; i < fxGroupCount; i++) {
    if (fxGroups[i].instType == instNone || fxGroups[i].instType == instType) {
      if (visibleCount == visibleIdx) {
        return &fxGroups[i];
      }
      visibleCount++;
    }
  }
  return NULL;
}

// Get actual group index from visible index
int getActualGroupIndex(int visibleIdx, enum InstrumentType instType) {
  extern FXGroup fxGroups[];
  extern int fxGroupCount;

  int visibleCount = 0;
  for (int i = 0; i < fxGroupCount; i++) {
    if (fxGroups[i].instType == instNone || fxGroups[i].instType == instType) {
      if (visibleCount == visibleIdx) {
        return i;
      }
      visibleCount++;
    }
  }
  return -1;
}

// Check if a group is expanded
int isGroupExpanded(int groupIdx) {
  return expandedGroup == groupIdx;
}

// Expand a group (collapses others)
void expandGroup(int groupIdx) {
  expandedGroup = groupIdx;
}

// Collapse a group
void collapseGroup(int groupIdx) {
  if (expandedGroup == groupIdx) {
    expandedGroup = -1;
  }
}

// Draw a group header at specified Y position
// Returns the Y position after the header (for next element)
int drawGroupHeader(int visibleGroupIdx, int y, int isCurrent) {
  FXGroup* group = getVisibleGroup(visibleGroupIdx, getCurrentInstrumentType());
  if (!group) return y;

  // Highlight current group header
  if (isCurrent) {
    gfxSetFgColor(appSettings.colorScheme.textValue);
  } else {
    gfxSetFgColor(appSettings.colorScheme.textTitles);
  }

  gfxPrint(1, y, group->name);

  return y + 1;  // Next line after header
}

// Draw FX list for a group at specified Y position
// Returns the Y position after the FX list (for next element)
int drawFXList(int visibleGroupIdx, int y) {
  FXGroup* group = getVisibleGroup(visibleGroupIdx, getCurrentInstrumentType());
  if (!group) return y;

  int cols = group->columns;  // Use group-specific column count

  // Draw FX in grid with group-specific column count
  for (int idx = 0; idx < group->count; idx++) {
    int row = idx / cols;
    int col = idx % cols;
    int fxY = y + row;

    // Highlight current FX if this is the current group
    int isCurrent = (visibleGroupIdx == currentGroup && idx == currentIdx);
    if (isCurrent) {
      gfxSetFgColor(appSettings.colorScheme.textValue);
    } else {
      gfxSetFgColor(appSettings.colorScheme.textDefault);
    }

    gfxPrint(1 + col * 4, fxY, group->fxList[idx].name);

    if (isCurrent) {
      gfxCursor(1 + col * 4, fxY, 3);
    }
  }

  // Calculate how many rows the FX list takes
  int rows = (group->count + cols - 1) / cols;  // Ceiling division
  return y + rows;
}

void fxEditFullDraw(uint8_t currentFX, uint8_t instrumentIdx) {
  gfxClearRect(0, 0, 35, 20);

  // Store instrument index for this session
  currentInstrumentIdx = instrumentIdx;

  // Get instrument type for filtering groups
  enum InstrumentType instType = instNone;
  if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
    instType = (enum InstrumentType)chipnomadState->project.instruments[instrumentIdx].type;
  }

  // Get visible groups
  int visibleGroupCount = getVisibleGroupCount(instType);

  // Find which group contains the current FX (if any)
  int foundGroup = -1;
  int foundIdx = -1;
  for (int g = 0; g < visibleGroupCount; g++) {
    FXGroup* group = getVisibleGroup(g, instType);
    if (group) {
      for (int i = 0; i < group->count; i++) {
        if (group->fxList[i].fx == currentFX) {
          foundGroup = g;
          foundIdx = i;
          break;
        }
      }
      if (foundGroup >= 0) break;
    }
  }

  // Initialize state
  if (foundGroup >= 0) {
    // Current FX found in a visible group - expand that group and position cursor on it
    currentGroup = foundGroup;
    currentIdx = foundIdx;
    expandedGroup = foundGroup;
  } else {
    // Current FX not found - default to first group, first FX
    currentGroup = 0;
    currentIdx = 0;
    expandedGroup = 0;
  }

  // Draw help for current FX at top (with instrument context)
  drawFXHelp((enum FX)currentFX, instrumentIdx);

  // Draw all visible groups (headers + expanded group's FX list)
  int y = 7;  // Start below help text
  for (int g = 0; g < visibleGroupCount; g++) {
    // Draw group header
    int isCurrent = (g == currentGroup);
    y = drawGroupHeader(g, y, isCurrent);

    // Draw FX list only if this group is expanded
    if (g == expandedGroup) {
      y = drawFXList(g, y);
    }

    // Add spacing between groups
    y++;
  }
}


int fxEditInput(int keys, int tapCount, uint8_t* fx, uint8_t* lastFX) {
  if (keys == 0) {
    // Selection complete - update the FX value
    FXGroup* group = getVisibleGroup(currentGroup, getCurrentInstrumentType());
    if (group && currentIdx < group->count) {
      fx[0] = group->fxList[currentIdx].fx;
      lastFX[0] = fx[0];
    }
    return 1;
  }

  if (keys & keyEdit) {
    int oldGroup = currentGroup;
    int visibleGroupCount = getVisibleGroupCount(getCurrentInstrumentType());
    FXGroup* group = getVisibleGroup(currentGroup, getCurrentInstrumentType());
    if (!group) return 0;

    // Navigate based on d-pad input
    if (keys & keyRight) {
      // Move to next FX (linear navigation)
      currentIdx++;
      if (currentIdx >= group->count) {
        // Reached end of current group - move to next group
        if (currentGroup < visibleGroupCount - 1) {
          currentGroup++;
          currentIdx = 0;
          expandedGroup = currentGroup;
        } else {
          // At last FX of last group - stay there
          currentIdx = group->count - 1;
        }
      }
    } else if (keys & keyLeft) {
      // Move to previous FX (linear navigation)
      currentIdx--;
      if (currentIdx < 0) {
        // Reached beginning of current group - move to previous group
        if (currentGroup > 0) {
          currentGroup--;
          FXGroup* prevGroup = getVisibleGroup(currentGroup, getCurrentInstrumentType());
          currentIdx = prevGroup ? prevGroup->count - 1 : 0;
          expandedGroup = currentGroup;
        } else {
          // At first FX of first group - stay there
          currentIdx = 0;
        }
      }
    } else if (keys & keyUp) {
      // Move up one row in grid (using group-specific column count)
      currentIdx -= group->columns;
      if (currentIdx < 0) {
        // Reached top of current group - move to previous group
        if (currentGroup > 0) {
          currentGroup--;
          FXGroup* prevGroup = getVisibleGroup(currentGroup, getCurrentInstrumentType());
          if (prevGroup) {
            // Position at last FX of previous group
            currentIdx = prevGroup->count - 1;
          } else {
            currentIdx = 0;
          }
          expandedGroup = currentGroup;
        } else {
          // At top of first group - stay at first FX
          currentIdx = 0;
        }
      }
    } else if (keys & keyDown) {
      // Move down one row in grid (using group-specific column count)
      currentIdx += group->columns;
      if (currentIdx >= group->count) {
        // Reached bottom of current group - move to next group
        if (currentGroup < visibleGroupCount - 1) {
          currentGroup++;
          currentIdx = 0;
          expandedGroup = currentGroup;
        } else {
          // At bottom of last group - stay at last FX
          currentIdx = group->count - 1;
        }
      }
    }

    // If group changed, need full redraw
    if (oldGroup != currentGroup) {
      FXGroup* newGroup = getVisibleGroup(currentGroup, getCurrentInstrumentType());
      if (newGroup && currentIdx < newGroup->count) {
        fxEditFullDraw(newGroup->fxList[currentIdx].fx, currentInstrumentIdx);
      }
    } else {
      // Same group, just update the FX selection
      // For now, do a simple redraw (can optimize later)
      FXGroup* currentGroupPtr = getVisibleGroup(currentGroup, getCurrentInstrumentType());
      if (currentGroupPtr && currentIdx < currentGroupPtr->count) {
        fxEditFullDraw(currentGroupPtr->fxList[currentIdx].fx, currentInstrumentIdx);
      }
    }
  }

  return 0;
}
