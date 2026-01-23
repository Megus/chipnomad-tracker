#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "utils.h"

static const char* getEnvelopeShapeASCII(int shape) {
  shape = shape & 0x0F;
  if (shape == 0) return "  ";
  if (shape <= 3 || shape == 9 || shape == 15) return "\\_";
  if (shape >= 4 && shape <= 7) return "_/";
  if (shape == 8) return "\\\\";
  if (shape == 10) return "\\/";
  if (shape == 11) return "\\-";
  if (shape == 12) return "//";
  if (shape == 13) return "/-";
  if (shape == 14) return "/\\";
  return "  ";
}

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
  // The first 3 rows come from the common instrument fields
  if (row < 3) return instrumentCommonColumnCount(row);

  // Row 3: Tone only
  if (row == 3) {
    return 1;
  }

  // Rows 4-5: Noise/Env in col 0, Auto Envelope in col 1-2
  if (row == 4) {
    return 2;
  }
  if (row == 5) {
    int isAutoEnvEnabled = chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN != 0;
    return isAutoEnvEnabled ? 3 : 1;
  }

  // Volume envelope parameters (rows 6-9) are in column 0 only
  if (row >= 6 && row <= 9) {
    return 1;
  }

  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(0, 6, "Tone");
  gfxPrint(0, 7, "Noise");
  gfxPrint(0, 8, "Env");

  gfxPrint(17, 6, "Auto env period");
  gfxPrint(17, 7, "Enabled");
  gfxPrint(17, 8, "Rate");

  gfxPrint(0, 10, "Volume envelope");
  gfxPrint(0, 11, "Atk");
  gfxPrint(0, 12, "Dec");
  gfxPrint(0, 13, "Sus");
  gfxPrint(0, 14, "Rel");
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  if (col == 0) {
    if (row == 3) {
      gfxCursor(8, 6, 3);
    } else if (row == 4) {
      gfxCursor(8, 7, 3);
    } else if (row == 5) {
      gfxCursor(8, 8, 1);
    } else if (row == 6) {
      gfxCursor(8, 11, 2);
    } else if (row == 7) {
      gfxCursor(8, 12, 2);
    } else if (row == 8) {
      gfxCursor(8, 13, 2);
    } else if (row == 9) {
      gfxCursor(8, 14, 2);
    }
  } else if (col == 1) {
    if (row == 4) {
      gfxCursor(26, 7, 3);
    } else if (row == 5) {
      gfxCursor(26, 8, 1);
    }
  } else if (col == 2) {
    if (row == 4 || row == 5) {
      gfxCursor(28, 8, 1);
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  uint8_t defaultMixer = chipnomadState->project.instruments[cInstrument].chip.ay.defaultMixer;
  int hasTone = (defaultMixer & 0x01) != 0;
  int hasNoise = (defaultMixer & 0x02) != 0;
  int envShape = (defaultMixer >> 4) & 0x0F;
  int isAutoEnvEnabled = chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN != 0;

  if (row == 3 && col == 0) {
    gfxPrintf(8, 6, hasTone ? "On " : "Off");
  } else if (row == 4 && col == 0) {
    gfxPrintf(8, 7, hasNoise ? "On " : "Off");
  } else if (row == 5 && col == 0) {
    gfxPrintf(8, 8, "%X %s", envShape, getEnvelopeShapeASCII(envShape));
  } else if (row == 6 && col == 0) {
    gfxPrint(8, 11, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veA));
  } else if (row == 7 && col == 0) {
    gfxPrint(8, 12, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veD));
  } else if (row == 8 && col == 0) {
    gfxPrint(8, 13, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veS));
  } else if (row == 9 && col == 0) {
    gfxPrint(8, 14, byteToHex(chipnomadState->project.instruments[cInstrument].chip.ay.veR));
  } else if (row == 4 && col == 1) {
    gfxPrintf(26, 7, isAutoEnvEnabled ? "On " : "Off");
    if (!isAutoEnvEnabled) gfxPrint(26, 8, "   ");
  } else if (row == 5 && col == 1) {
    if (isAutoEnvEnabled) {
      gfxPrintf(26, 8, "%hhd:", chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN);
    }
  } else if ((row == 4 || row == 5) && col == 2) {
    if (isAutoEnvEnabled) {
      gfxPrintf(28, 8, "%hhd", chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD);
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;
  uint8_t* defaultMixer = &chipnomadState->project.instruments[cInstrument].chip.ay.defaultMixer;

  if (col == 0) {
    if (row == 3) {
      uint8_t value = (*defaultMixer & 0x01) != 0;
      handled = edit8noLast(action, &value, 1, 0, 1);
      if (handled) {
        *defaultMixer = (*defaultMixer & 0xFE) | (value & 0x01);
      }
    } else if (row == 4) {
      uint8_t value = (*defaultMixer & 0x02) != 0;
      handled = edit8noLast(action, &value, 1, 0, 1);
      if (handled) {
        *defaultMixer = (*defaultMixer & 0xFD) | ((value & 0x01) << 1);
      }
    } else if (row == 5) {
      uint8_t envShape = (*defaultMixer >> 4) & 0x0F;
      handled = edit8noLast(action, &envShape, 8, 0, 15);
      if (handled) {
        *defaultMixer = (*defaultMixer & 0x0F) | (envShape << 4);
      }
    } else if (row == 6) {
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veA, 16, 0, 255);
    } else if (row == 7) {
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veD, 16, 0, 255);
    } else if (row == 8) {
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veS, 1, 0, 15);
    } else if (row == 9) {
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.veR, 16, 0, 255);
    }
  } else if (col == 1) {
    if (row == 4) {
      uint8_t value = chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN != 0;
      uint8_t oldValue = value;
      handled = edit8noLast(action, &value, 1, 0, 1);
      if (oldValue != value && value == 1) {
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN = 1;
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD = 1;
        drawField(1, 5, 0);
        drawField(2, 5, 0);
      } else if (oldValue != value && value == 0) {
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN = 0;
        chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD = 0;
      }
    } else if (row == 5) {
      handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvN, 16, 1, 8);
    }
  } else if (col == 2 && (row == 4 || row == 5)) {
    handled = edit8noLast(action, &chipnomadState->project.instruments[cInstrument].chip.ay.autoEnvD, 16, 1, 8);
  }

  return handled;
}

ScreenData screenInstrumentAY = {
  .rows = 10,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
};
