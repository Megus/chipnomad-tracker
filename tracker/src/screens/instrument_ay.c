#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "utils.h"

void initAYInstrument(int instrument) {
  chipnomadState->project.instruments[instrument].type = instAY;
  chipnomadState->project.instruments[instrument].name[0] = 0;
  chipnomadState->project.instruments[instrument].tableSpeed = 1;
  chipnomadState->project.instruments[instrument].transposeEnabled = 1;
  chipnomadState->project.instruments[instrument].chip.ay.veA = 0;
  chipnomadState->project.instruments[instrument].chip.ay.veD = 0;
  chipnomadState->project.instruments[instrument].chip.ay.veS = 15;
  chipnomadState->project.instruments[instrument].chip.ay.veR = 0;
  chipnomadState->project.instruments[instrument].chip.ay.autoEnvN = 0;
  chipnomadState->project.instruments[instrument].chip.ay.autoEnvD = 0;
}

static int getColumnCount(int row) {
  // The first 3 rows come from the common cInstrument fields
  if (row < 3) return instrumentCommonColumnCount(row);

  // Volume envelope parameters (rows 3-6) are in column 0
  // Auto envelope parameters (rows 3-4) are in column 1

  // Both volume env and auto env parameters
  if (row == 3) {
    return 2;
  } else if (row == 4) {
    int isAutoEnvEnabled = chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN != 0;
    return isAutoEnvEnabled ? 3 : 1;
  } else if (row == 5 || row == 6) {
    return 1; // Only volume env parameters
  }

  return 1; // Default
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 6, "Volume env");
  gfxPrint(0, 7, "Atk");
  gfxPrint(0, 8, "Dec");
  gfxPrint(0, 9, "Sus");
  gfxPrint(0, 10, "Rel");

  gfxPrint(17, 6, "Auto Envelope");
  gfxPrint(17, 7, "Enabled");
  gfxPrint(17, 8, "Rate");
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  // Volume envelope parameters (column 0)
  if (col == 0) {
    if (row == 3) {
      // Attack
      gfxCursor(8, 7, 2);
    } else if (row == 4) {
      // Decay
      gfxCursor(8, 8, 2);
    } else if (row == 5) {
      // Sustain
      gfxCursor(8, 9, 2);
    } else if (row == 6) {
      // Release
      gfxCursor(8, 10, 2);
    }
  }
  // Auto envelope parameters (column 1)
  else if (col == 1) {
    if (row == 3) {
      // Enabled
      gfxCursor(26, 7, 3);
    } else if (row == 4) {
      // Numerator
      gfxCursor(26, 8, 1);
    }
  }
  // Auto envelope perameters (column 2)
  else if (col == 2) {
    if (row == 4) {
      // Denumerator
      gfxCursor(28, 8, 1);
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  int isAutoEnvEnabled = chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN != 0;

  // Volume envelope parameters
  if (row == 3 && col == 0) {
    // Attack
    gfxPrint(8, 7, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veA));
  } else if (row == 4 && col == 0) {
    // Decay
    gfxPrint(8, 8, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veD));
  } else if (row == 5 && col == 0) {
    // Sustain
    gfxPrint(8, 9, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veS));
  } else if (row == 6 && col == 0) {
    // Release
    gfxPrint(8, 10, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veR));
  }

  // Auto Envelope parameters
  else if (row == 3 && col == 1) {
    // Enabled (auto-env Numerator is not zero) / Disabled (numerator = 0)
    gfxPrintf(26, 7, isAutoEnvEnabled ? "On " : "Off");
    if (!isAutoEnvEnabled) gfxPrint(26, 8, "   ");
  } else if (row == 4 && col == 1) {
    // Rate (numerator:denumerator)
    if (isAutoEnvEnabled) {
      gfxPrintf(26, 8, "%hhd:", chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN);
    }
  } else if (row == 4 && col == 2) {
    // Denumerator (if auto-env is enabled)
    if (isAutoEnvEnabled) {
      gfxPrintf(28, 8, "%hhd", chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD);
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;

  // Volume envelope parameters (column 0)
  if (col == 0) {
    if (row == 3) {
      // Attack
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veA, 16, 0, 255);
    } else if (row == 4) {
      // Decay
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veD, 16, 0, 255);
    } else if (row == 5) {
      // Sustain
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veS, 18, 0, 15);
    } else if (row == 6) {
      // Release
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veR, 16, 0, 255);
    }
  }
  // Auto envelope parameters (column 1)
  else if (col == 1) {
    if (row == 3) {
      // Enabled (toggle between 0 and 1)
      uint8_t value = chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN != 0;
      uint8_t oldValue = value;
      handled = edit8noLast(action, &value, 1, 0, 1);
      if (oldValue != value && value == 1) {
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN = 1;
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD = 1;
        drawField(1, 4, 0);
        drawField(2, 4, 0);
      } else if (oldValue != value && value == 0) {
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN = 0;
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD = 0;
      }
    } else if (row == 4) {
      // Rate: numerator
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN, 16, 1, 8);
    }
  } else if (col == 2 && row == 4) {
    // Rate: denumerator
    handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD, 16, 1, 8);
  }

  return handled;
}

ScreenData screenInstrumentAY = {
  .rows = 7,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
};
