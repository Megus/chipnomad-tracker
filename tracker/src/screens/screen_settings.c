#include "screen_settings.h"
#include "common.h"
#include "corelib_gfx.h"
#include "corelib_mainloop.h"
#include "screens.h"

// Forward declarations
static int settingsColumnCount(int row);
static void settingsDrawStatic(void);
static void settingsDrawCursor(int col, int row);
static void settingsDrawRowHeader(int row, int state);
static void settingsDrawColHeader(int col, int state);
static void settingsDrawField(int col, int row, int state);
static int settingsOnEdit(int col, int row, enum CellEditAction action);

static ScreenData screenSettingsData = {
  .rows = 5,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = settingsColumnCount,
  .drawStatic = settingsDrawStatic,
  .drawCursor = settingsDrawCursor,
  .drawRowHeader = settingsDrawRowHeader,
  .drawColHeader = settingsDrawColHeader,
  .drawField = settingsDrawField,
  .onEdit = settingsOnEdit,
};

static void setup(int input) {
}

static void fullRedraw(void) {
  screenFullRedraw(&screenSettingsData);
}

static void draw(void) {
}

int settingsColumnCount(int row) {
  return 1;
}

void settingsDrawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "SETTINGS");
}

void settingsDrawCursor(int col, int row) {
  if (row == 0 && col == 0) {
    gfxCursor(23, 2, 3); // Under ON/OFF value
  } else if (row == 1 && col == 0) {
    gfxCursor(23, 3, 4); // Under mix volume percentage
  } else if (row == 2 && col == 0) {
    gfxCursor(23, 4, 6); // Under quality value
  } else if (row == 3 && col == 0) {
    gfxCursor(23, 5, 3); // Under gamepad swap ON/OFF
  } else if (row == 4 && col == 0) {
    gfxCursor(0, 17, 14); // "Quit ChipNomad"
  }
}

void settingsDrawRowHeader(int row, int state) {
}

void settingsDrawColHeader(int col, int state) {
}

void settingsDrawField(int col, int row, int state) {
  if (row == 0 && col == 0) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(0, 2, "Pitch conflict warning");
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    gfxPrint(23, 2, appSettings.pitchConflictWarning ? "ON " : "OFF");
  } else if (row == 1 && col == 0) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(0, 3, "Mix volume");
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    int mixVolumePercent = (int)(appSettings.mixVolume * 100.0f + 0.5f);
    gfxPrintf(23, 3, "%03d%%", mixVolumePercent);
  } else if (row == 2 && col == 0) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(0, 4, "Quality");
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    const char* qualityNames[] = {"LOW   ", "MEDIUM", "HIGH  ", "BEST  "};
    gfxPrint(23, 4, qualityNames[appSettings.quality]);
  } else if (row == 3 && col == 0) {
    gfxSetFgColor(appSettings.colorScheme.textDefault);
    gfxPrint(0, 5, "Gamepad swap A/B");
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    gfxPrint(23, 5, appSettings.gamepadSwapAB ? "ON " : "OFF");
  } else if (row == 4 && col == 0) {
    gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
    gfxPrint(0, 17, "Quit ChipNomad");
  }
}

int settingsOnEdit(int col, int row, enum CellEditAction action) {
  if (row == 0 && col == 0) {
    // Pitch conflict warning (0/1)
    static uint8_t lastValue = 0;
    return edit8withLimit(action, (uint8_t*)&appSettings.pitchConflictWarning, &lastValue, 1, 1);
  } else if (row == 1 && col == 0) {
    // Mix volume (1-100%)
    int mixVolumePercent = (int)(appSettings.mixVolume * 100.0f + 0.5f);
    uint8_t mixVolumePercentU8 = (uint8_t)mixVolumePercent;
    int handled = edit8noLast(action, &mixVolumePercentU8, 10, 1, 100);
    if (handled) {
      appSettings.mixVolume = (float)mixVolumePercentU8 / 100.0f;
      if (chipnomadState) {
        chipnomadState->mixVolume = appSettings.mixVolume;
      }
    }
    return handled;
  } else if (row == 2 && col == 0) {
    // Quality (0-3)
    static uint8_t lastValue = 0;
    int handled = edit8withLimit(action, (uint8_t*)&appSettings.quality, &lastValue, 1, 3);
    if (handled && chipnomadState) {
      chipnomadSetQuality(chipnomadState, appSettings.quality);
    }
    return handled;
  } else if (row == 3 && col == 0) {
    // Gamepad swap A/B (0/1)
    static uint8_t lastValue = 0;
    return edit8withLimit(action, (uint8_t*)&appSettings.gamepadSwapAB, &lastValue, 1, 1);
  } else if (row == 4 && col == 0 && action == editTap) {
    // Trigger exit event
    mainLoopTriggerQuit();
    return 1;
  }
  return 0;
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyUp | keyShift)) {
    screenSetup(&screenSong, 0);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return 1;
  return screenInput(&screenSettingsData, isKeyDown, keys, isDoubleTap);
}

const AppScreen screenSettings = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
