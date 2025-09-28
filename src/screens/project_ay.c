#include <screen_project.h>
#include <corelib_gfx.h>
#include <string.h>
#include <audio_manager.h>
#include <chips.h>

int chipClockLength = 0;

static char stereoModes[3][5] = {
  "ABC",
  "ACB",
  "BAC",
};

static int clockPresets[4] = {
  1000000,   // 1 MHz
  1750000,   // 1.75 MHz
  1773400,   // 1.7734 MHz
  2000000    // 2 MHz
};

static char clockNames[4][11] = {
  "1 MHz",
  "1.75 MHz",
  "1.7734 MHz",
  "2 MHz"
};

static int getClockPresetIndex(int clock) {
  for (int i = 0; i < 4; i++) {
    if (clockPresets[i] == clock) return i;
  }
  return 0; // Default to 1 MHz
}

static int getColumnCount(int row) {
  // The first 7 rows come from the common project screen fields
  if (row < SCR_PROJECT_ROWS) return projectCommonColumnCount(row);

  return 1; // Default
}

static void drawStatic(void) {
  projectCommonDrawStatic();
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 10, "Subtype");
  gfxPrint(0, 11, "Stereo");
  gfxPrint(0, 12, "Stereo width");
  gfxPrint(0, 13, "Chip clock");
  gfxPrint(0, 14, "Pitch table");
}

static void drawCursor(int col, int row) {
  if (row < SCR_PROJECT_ROWS) return projectCommonDrawCursor(col, row);
  if (row == SCR_PROJECT_ROWS) {
    // Chip type
    gfxCursor(13, 10, project.chipSetup.ay.isYM ? 7 : 9);
  } else if (row == SCR_PROJECT_ROWS + 1) {
    // Panning scheme
    gfxCursor(13, 11, 3);
  } else if (row == SCR_PROJECT_ROWS + 2) {
    // Stereo width
    gfxCursor(13, 12, 4);
  } else if (row == SCR_PROJECT_ROWS + 3) {
    // Chip clock
    gfxCursor(13, 13, chipClockLength);
  } else if (row == SCR_PROJECT_ROWS + 4) {
    // Pitch table
    gfxCursor(13, 14, strlen(project.pitchTable.name));
  }
}

static void drawField(int col, int row, int state) {
  if (row < SCR_PROJECT_ROWS) return projectCommonDrawField(col, row, state);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == SCR_PROJECT_ROWS) {
    gfxClearRect(13, 10, 9, 1);
    gfxPrint(13, 10, project.chipSetup.ay.isYM ? "YM2149F" : "AY-3-8910");
  } else if (row == SCR_PROJECT_ROWS + 1) {
    // Panning scheme
    gfxClearRect(13, 11, 5, 1);
    gfxPrint(13, 11, stereoModes[project.chipSetup.ay.stereoMode]);
  } else if (row == SCR_PROJECT_ROWS + 2) {
    // Stereo width
    gfxPrintf(13, 12, "%03d%%", project.chipSetup.ay.stereoSeparation);
  } else if (row == SCR_PROJECT_ROWS + 3) {
    int presetIndex = getClockPresetIndex(project.chipSetup.ay.clock);
    chipClockLength = strlen(clockNames[presetIndex]);
    gfxClearRect(13, 13, 20, 1);
    gfxPrint(13, 13, clockNames[presetIndex]);
  } else if (row == SCR_PROJECT_ROWS + 4) {
    gfxClearRect(13, 14, PROJECT_PITCH_TABLE_TITLE_LENGTH, 1);
    gfxPrint(13, 14, project.pitchTable.name);
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < SCR_PROJECT_ROWS) return projectCommonOnEdit(col, row, action);

  int handled = 0;

  if (row == SCR_PROJECT_ROWS) {
    // Chip subtype (AY-3-8910 / YM2149F)
    handled = edit8noLast(action, &project.chipSetup.ay.isYM, 1, 0, 1);
    if (handled) {
      updateChipAYType(&audioManager.chips[0], project.chipSetup.ay.isYM);
    }
  } else if (row == SCR_PROJECT_ROWS + 1) {
    // Stereo mode (ABC, ACB, BAC)
    handled = edit8noLast(action, (uint8_t*)&project.chipSetup.ay.stereoMode, 1, 0, 2);
    if (handled) {
      updateChipAYStereoMode(&audioManager.chips[0], project.chipSetup.ay.stereoMode, project.chipSetup.ay.stereoSeparation);
    }
  } else if (row == SCR_PROJECT_ROWS + 2) {
    // Stereo width (0-100%)
    handled = edit8noLast(action, &project.chipSetup.ay.stereoSeparation, 10, 0, 100);
    if (handled) {
      updateChipAYStereoMode(&audioManager.chips[0], project.chipSetup.ay.stereoMode, project.chipSetup.ay.stereoSeparation);
    }
  } else if (row == SCR_PROJECT_ROWS + 3) {
    // Chip clock presets
    int presetIndex = getClockPresetIndex(project.chipSetup.ay.clock);
    uint8_t newIndex = presetIndex;
    handled = edit8noLast(action, &newIndex, 1, 0, 3);
    if (handled) {
      project.chipSetup.ay.clock = clockPresets[newIndex];
      updateChipAYClock(&audioManager.chips[0], project.chipSetup.ay.clock, appSettings.audioSampleRate);
    }
  }

  return handled;
}

struct ScreenData screenProjectAY = {
  .rows = 12,
  .cursorRow = 0,
  .cursorCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
};
