#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "misc.h"

void initAY2Instrument(int instrument) {
  Instrument* inst = &chipnomadState->project.instruments[instrument];
  inst->type = instAY2;
  inst->name[0] = 0;
  inst->tableSpeed = 1;
  inst->transposeEnabled = 1;
  inst->chip.ay2 = (InstrumentAY2){
    .oscTone = { .isOn = 1, .pitchFlag = 0, .pitchOffset = 0, .fineTune = 0 },
    .oscNoise = { .isOn = 0, .noisePeriod = 0 },
    .oscEnvelope = { .shape = 0, .pitchFlag = 0, .pitchOffset = 0, .fineTune = 0 },
    .oscSoftware = { .type = aySoftwareOscNone, .pitchFlag = 0, .pitchOffset = 0, .fineTune = 0 },
  };
}

// Screen layout:
// y 6:  Tone   [On ]       Envelope [Off   ]
// y 7:  Pitch  [+0  ]      Pitch    [+0  ]
// y 8:  Fine   [+0  ]      Fine     [+0  ]
// y 9:  (spacing)
// y 10: Noise  [Off]       Soft osc [Off  ]
// y 11: Period [00 ]       Pitch    [+0  ]
// y 12:                    Fine     [+0  ]
//
// Logical rows:
// 0-2: common (type, name, transpose/tic)
// 3: Tone on/off | Env shape        (y 6)
// 4: Tone pitch  | Env pitch        (y 7)
// 5: Tone fine   | Env fine         (y 8)
// 6: Noise on/off | Soft osc type   (y 10)
// 7: Noise period | Soft osc pitch  (y 11)
// 8: (dead left)  | Soft osc fine   (y 12)

#define COL_LEFT_X    0
#define COL_LEFT_VAL  8
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 26

#define ROW_TOTAL 9

static int rowToY(int row) {
  switch (row) {
    case 3: return 6;
    case 4: return 7;
    case 5: return 8;
    case 6: return 10;
    case 7: return 11;
    case 8: return 12;
    default: return 0;
  }
}

static const char* softwareOscTypeName(enum AYSoftwareOscType type) {
  switch (type) {
    case aySoftwareOscNone:           return "Off  ";
    case aySoftwareOscRingMod:        return "Ring ";
    case aySoftwareOscSyncTone:       return "SyncT";
    case aySoftwareOscSyncEnvelope:   return "SyncE";
    case aySoftwareOscNoiseWavetable: return "NoiWT";
    default:                          return "?    ";
  }
}

static void drawSignedValue(int x, int y, int8_t value, int width) {
  gfxClearRect(x, y, width, 1);
  if (value >= 0) {
    gfxPrintf(x, y, "+%d", value);
  } else {
    gfxPrintf(x, y, "%d", value);
  }
}

static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  if (row >= 3 && row <= 8) return 2;
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  const ColorScheme cs = appSettings.colorScheme;

  // Top block headers (y 6)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 6, "Tone");
  gfxPrint(COL_RIGHT_X, 6, "Envelope");

  // Top block labels (y 7-8)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 7, "Pitch");
  gfxPrint(COL_LEFT_X, 8, "Fine");
  gfxPrint(COL_RIGHT_X, 7, "Pitch");
  gfxPrint(COL_RIGHT_X, 8, "Fine");

  // Bottom block headers (y 10)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 10, "Noise");
  gfxPrint(COL_RIGHT_X, 10, "Soft osc");

  // Bottom block labels (y 11-12)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 11, "Period");
  gfxPrint(COL_RIGHT_X, 11, "Pitch");
  gfxPrint(COL_RIGHT_X, 12, "Fine");
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  int y = rowToY(row);

  if (col == 0) {
    switch (row) {
      case 3: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Tone on/off
      case 4: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Tone pitch
      case 5: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Tone fine
      case 6: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Noise on/off
      case 7: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Noise period
    }
  } else if (col == 1) {
    switch (row) {
      case 3: gfxCursor(COL_RIGHT_VAL, y, 1); break; // Env shape (single hex digit)
      case 4: gfxCursor(COL_RIGHT_VAL, y, 4); break; // Env pitch
      case 5: gfxCursor(COL_RIGHT_VAL, y, 4); break; // Env fine
      case 6: gfxCursor(COL_RIGHT_VAL, y, 5); break; // Soft osc type
      case 7: gfxCursor(COL_RIGHT_VAL, y, 4); break; // Soft osc pitch
      case 8: gfxCursor(COL_RIGHT_VAL, y, 4); break; // Soft osc fine
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
        drawSignedValue(COL_LEFT_VAL, y, ay2->oscTone.pitchOffset, 4);
        break;
      case 5: // Tone fine
        drawSignedValue(COL_LEFT_VAL, y, ay2->oscTone.fineTune, 4);
        break;
      case 6: // Noise on/off
        gfxPrintf(COL_LEFT_VAL, y, ay2->oscNoise.isOn ? "On " : "Off");
        break;
      case 7: // Noise period
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex(ay2->oscNoise.noisePeriod));
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 3: // Envelope shape
        gfxClearRect(COL_RIGHT_VAL, y, 6, 1);
        if (ay2->oscEnvelope.shape == 0) {
          gfxPrint(COL_RIGHT_VAL, y, "0 Off ");
        } else {
          gfxPrintf(COL_RIGHT_VAL, y, "%X %s", ay2->oscEnvelope.shape, getEnvelopeShapeASCII(ay2->oscEnvelope.shape));
        }
        break;
      case 4: // Envelope pitch
        drawSignedValue(COL_RIGHT_VAL, y, ay2->oscEnvelope.pitchOffset, 4);
        break;
      case 5: // Envelope fine
        drawSignedValue(COL_RIGHT_VAL, y, ay2->oscEnvelope.fineTune, 4);
        break;
      case 6: // Software osc type
        gfxPrint(COL_RIGHT_VAL, y, softwareOscTypeName(ay2->oscSoftware.type));
        break;
      case 7: // Software osc pitch
        drawSignedValue(COL_RIGHT_VAL, y, ay2->oscSoftware.pitchOffset, 4);
        break;
      case 8: // Software osc fine
        drawSignedValue(COL_RIGHT_VAL, y, ay2->oscSoftware.fineTune, 4);
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
        handled = editSigned8(action, &ay2->oscTone.pitchOffset, 12, -128, 127);
        break;
      case 5: // Tone fine
        handled = editSigned8(action, &ay2->oscTone.fineTune, 12, -128, 127);
        break;
      case 6: // Noise on/off
        handled = edit8noLast(action, &ay2->oscNoise.isOn, 1, 0, 1);
        break;
      case 7: // Noise period
        handled = edit8noLast(action, &ay2->oscNoise.noisePeriod, 8, 0, 31);
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 3: // Envelope shape
        handled = edit8noLast(action, &ay2->oscEnvelope.shape, 8, 0, 15);
        break;
      case 4: // Envelope pitch
        handled = editSigned8(action, &ay2->oscEnvelope.pitchOffset, 12, -128, 127);
        break;
      case 5: // Envelope fine
        handled = editSigned8(action, &ay2->oscEnvelope.fineTune, 12, -128, 127);
        break;
      case 6: // Software osc type
        {
          uint8_t type = (uint8_t)ay2->oscSoftware.type;
          handled = edit8noLast(action, &type, 1, 0, aysoftwareOscTotalCount - 1);
          ay2->oscSoftware.type = (enum AYSoftwareOscType)type;
        }
        break;
      case 7: // Software osc pitch
        handled = editSigned8(action, &ay2->oscSoftware.pitchOffset, 12, -128, 127);
        break;
      case 8: // Software osc fine
        handled = editSigned8(action, &ay2->oscSoftware.fineTune, 12, -128, 127);
        break;
    }
  }

  if (handled) projectModified = 1;
  return handled;
}

static int isCellValid(int col, int row) {
  // Row 8 col 0 is a dead cell (noise block has no third row)
  if (row == 8 && col == 0) return 0;
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
