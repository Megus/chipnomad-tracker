#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "misc.h"

// Screen layout:
// y 6:  Tone   [On ]       Envelope [Off   ]
// y 7:  Pitch  [+0  ]      Mode     [Osc   ]
// y 8:  Fine   [+0  ]      Pitch/N  [+0  ]  (depends on mode)
// y 9:                     Fine/D   [+0  ]  (depends on mode)
// y 10: (spacing)
// y 11: Noise  [Off]       Soft osc [Off  ]
// y 12: Period [00 ]       Pitch    [+0  ]
// y 13:                    Fine     [+0  ]
//
// Logical rows:
// 0-2: common (type, name, transpose/tic)
// 3: Tone on/off | Env shape        (y 6)
// 4: Tone pitch  | Env mode         (y 7)
// 5: Tone fine   | Env pitch/N      (y 8)
// 6:             | Env fine/D       (y 9)
// 7: Noise on/off | Soft osc type   (y 11)
// 8: Noise period | Soft osc pitch  (y 12)
// 9: (dead left)  | Soft osc fine   (y 13)

#define COL_LEFT_X    0
#define COL_LEFT_VAL  8
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 26

#define ROW_TOTAL 10

static int rowToY(int row) {
  switch (row) {
    case 3: return 6;
    case 4: return 7;
    case 5: return 8;
    case 6: return 9;
    case 7: return 11;
    case 8: return 12;
    case 9: return 13;
    default: return 0;
  }
}

static const char* softwareOscTypeName(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscNone:           return "Off  ";
    case aySoftwareOscRingMod:        return "Ring ";
    case aySoftwareOscSyncTone:       return "SyncT";
    case aySoftwareOscSyncEnvelope:   return "SyncE";
    default:                          return "?    ";
  }
}

static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  if (row >= 3 && row <= 9) return 2;
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  const ColorScheme cs = appSettings.colorScheme;

  // Top block headers (y 6)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 6, "Tone");
  gfxPrint(COL_RIGHT_X, 6, "Envelope");

  // Top block labels (y 7-9)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 7, "Pitch");
  gfxPrint(COL_LEFT_X, 8, "Fine");
  gfxPrint(COL_RIGHT_X, 7, "Mode");
  // Row 8-9 labels are dynamic (Pitch/N and Fine/D or N and D)

  // Bottom block headers (y 11)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 11, "Noise");
  gfxPrint(COL_RIGHT_X, 11, "Soft osc");

  // Bottom block labels (y 12-13)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 12, "Period");
  gfxPrint(COL_RIGHT_X, 12, "Pitch");
  gfxPrint(COL_RIGHT_X, 13, "Fine");
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  int y = rowToY(row);

  if (col == 0) {
    switch (row) {
      case 3: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Tone on/off
      case 4: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Tone pitch (hex)
      case 5: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Tone fine (hex)
      case 7: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Noise on/off
      case 8: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Noise period
    }
  } else if (col == 1) {
    switch (row) {
      case 3: gfxCursor(COL_RIGHT_VAL, y, 1); break;  // Env shape (single hex digit)
      case 4: gfxCursor(COL_RIGHT_VAL, y, 6); break;  // Env mode
      case 5: // Env pitch/N
        gfxCursor(COL_RIGHT_VAL, y, 2);  // Always 2 chars for hex
        break;
      case 6: // Env fine/D
        gfxCursor(COL_RIGHT_VAL, y, 2);  // Always 2 chars for hex
        break;
      case 7: gfxCursor(COL_RIGHT_VAL, y, 5); break;  // Soft osc type
      case 8: gfxCursor(COL_RIGHT_VAL, y, 2); break;  // Soft osc pitch (hex)
      case 9: gfxCursor(COL_RIGHT_VAL, y, 2); break;  // Soft osc fine (hex)
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  InstrumentAY2* ay2 = &chipnomadState->project.instruments[cInstrument].chip.ay2;
  int y = rowToY(row);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (col == 0) {
    switch (row) {
      case 3: // Tone on/off
        gfxPrintf(COL_LEFT_VAL, y, ay2->oscTone.isOn ? "On " : "Off");
        break;
      case 4: // Tone pitch
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)ay2->oscTone.pitchOffset));
        break;
      case 5: // Tone fine
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)ay2->oscTone.fineTune));
        break;
      case 7: // Noise on/off
        gfxPrintf(COL_LEFT_VAL, y, ay2->oscNoise.isOn ? "On " : "Off");
        break;
      case 8: // Noise period
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex(ay2->oscNoise.noisePeriod));
        break;
    }
  } else if (col == 1) {
    int isAutoEnv = ay2->oscEnvelope.autoEnvN != 0;

    switch (row) {
      case 3: // Envelope shape
        gfxClearRect(COL_RIGHT_VAL, y, 6, 1);
        if (ay2->oscEnvelope.shape == 0) {
          gfxPrint(COL_RIGHT_VAL, y, "0 Off ");
        } else {
          gfxPrintf(COL_RIGHT_VAL, y, "%X %s", ay2->oscEnvelope.shape, getEnvelopeShapeASCII(ay2->oscEnvelope.shape));
        }
        break;
      case 4: // Envelope mode
        gfxClearRect(COL_RIGHT_VAL, y, 7, 1);
        gfxPrint(COL_RIGHT_VAL, y, isAutoEnv ? "AutoEnv" : "Osc   ");
        break;
      case 5: // Envelope pitch/N
        // Draw the label for this row
        gfxSetFgColor(appSettings.colorScheme.textDefault);
        gfxPrint(COL_RIGHT_X, y, isAutoEnv ? "N    " : "Pitch");
        // Draw the value
        gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        if (isAutoEnv) {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex(ay2->oscEnvelope.autoEnvN));
        } else {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscEnvelope.pitchOffset));
        }
        break;
      case 6: // Envelope fine/D
        // Draw the label for this row
        gfxSetFgColor(appSettings.colorScheme.textDefault);
        gfxPrint(COL_RIGHT_X, y, isAutoEnv ? "D    " : "Fine ");
        // Draw the value
        gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        if (isAutoEnv) {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex(ay2->oscEnvelope.autoEnvD));
        } else {
          gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscEnvelope.fineTune));
        }
        break;
      case 7: // Software osc type
        gfxPrint(COL_RIGHT_VAL, y, softwareOscTypeName(ay2->oscSoftware.type));
        break;
      case 8: // Software osc pitch
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscSoftware.pitchOffset));
        break;
      case 9: // Software osc fine
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)ay2->oscSoftware.fineTune));
        break;
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;
  InstrumentAY2* ay2 = &chipnomadState->project.instruments[cInstrument].chip.ay2;

  if (col == 0) {
    switch (row) {
      case 3: // Tone on/off
        handled = edit8noLast(action, &ay2->oscTone.isOn, 1, 0, 1);
        break;
      case 4: // Tone pitch
        handled = editSigned8(action, &ay2->oscTone.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Tone pitch offset %hhd", ay2->oscTone.pitchOffset);
        }
        break;
      case 5: // Tone fine
        handled = editSigned8(action, &ay2->oscTone.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Tone fine tune %hhd", ay2->oscTone.fineTune);
        }
        break;
      case 7: // Noise on/off
        handled = edit8noLast(action, &ay2->oscNoise.isOn, 1, 0, 1);
        break;
      case 8: // Noise period
        handled = edit8noLast(action, &ay2->oscNoise.noisePeriod, 8, 0, 31);
        break;
    }
  } else if (col == 1) {
    int isAutoEnv = ay2->oscEnvelope.autoEnvN != 0;

    switch (row) {
      case 3: // Envelope shape
        handled = edit8noLast(action, &ay2->oscEnvelope.shape, 8, 0, 15);
        break;
      case 4: // Envelope mode (toggle between Osc and AutoEnv)
        {
          uint8_t modeValue = isAutoEnv ? 1 : 0;
          uint8_t oldValue = modeValue;
          handled = edit8noLast(action, &modeValue, 1, 0, 1);

          if (handled && oldValue != modeValue) {
            if (modeValue == 1) {
              // Switching to AutoEnv mode
              ay2->oscEnvelope.autoEnvN = 1;
              ay2->oscEnvelope.autoEnvD = 1;
            } else {
              // Switching to Osc mode
              ay2->oscEnvelope.autoEnvN = 0;
              ay2->oscEnvelope.autoEnvD = 0;
            }
            // Redraw the fields (which will also redraw labels)
            drawField(1, 5, 0);
            drawField(1, 6, 0);
          }
        }
        break;
      case 5: // Envelope pitch/N
        if (isAutoEnv) {
          handled = edit8noLast(action, &ay2->oscEnvelope.autoEnvN, 16, 1, 15);
          if (handled) {
            screenMessage(0, "Envelope auto N %hhu", ay2->oscEnvelope.autoEnvN);
          }
        } else {
          handled = editSigned8(action, &ay2->oscEnvelope.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
          if (handled) {
            screenMessage(0, "Envelope pitch offset %hhd", ay2->oscEnvelope.pitchOffset);
          }
        }
        break;
      case 6: // Envelope fine/D
        if (isAutoEnv) {
          handled = edit8noLast(action, &ay2->oscEnvelope.autoEnvD, 16, 1, 15);
          if (handled) {
            screenMessage(0, "Envelope auto D %hhu", ay2->oscEnvelope.autoEnvD);
          }
        } else {
          handled = editSigned8(action, &ay2->oscEnvelope.fineTune, 16, -128, 127);
          if (handled) {
            screenMessage(0, "Envelope fine tune %hhd", ay2->oscEnvelope.fineTune);
          }
        }
        break;
      case 7: // Software osc type
        {
          uint8_t type = (uint8_t)ay2->oscSoftware.type;
          handled = edit8noLast(action, &type, 1, 0, aysoftwareOscTotalCount - 1);
          ay2->oscSoftware.type = (enum AYSoftwareOscType)type;
        }
        break;
      case 8: // Software osc pitch
        handled = editSigned8(action, &ay2->oscSoftware.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Software pitch offset %hhd", ay2->oscSoftware.pitchOffset);
        }
        break;
      case 9: // Software osc fine
        handled = editSigned8(action, &ay2->oscSoftware.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Software fine tune %hhd", ay2->oscSoftware.fineTune);
        }
        break;
    }
  }

  if (handled) projectModified = 1;
  return handled;
}

static int isCellValid(int col, int row) {
  // Row 6 col 0 and row 9 col 0 are dead cells (tone/noise blocks have no corresponding rows)
  if ((row == 6 || row == 9) && col == 0) return 0;
  return 1;
}

ScreenData screenInstrumentAY2 = {
  .rows = ROW_TOTAL,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
  .isCellValid = isCellValid,
};
