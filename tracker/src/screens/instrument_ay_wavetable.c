#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "utils.h"
#include <string.h>

// Screen layout:
// y 6:  Wave  [00]       Tone   [On ]
// y 7:  Pitch [+0  ]    Pitch  [+0  ]
// y 8:  Fine  [+0  ]    Fine   [+0  ]
// y 9:  (spacing)
// y 10: Noise [Off]
// y 11: Period [00]
//
// Logical rows:
// 0-2: common (type, name, transpose/tic)
// 3: Wave index    | Tone on/off
// 4: WT pitch      | Tone pitch
// 5: WT fine       | Tone fine
// 6: Noise on/off  (left only)
// 7: Noise period  (left only)

#define COL_LEFT_X    0
#define COL_LEFT_VAL  8
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 26

#define ROW_TOTAL 8

static int rowToY(int row) {
  switch (row) {
    case 3: return 6;
    case 4: return 7;
    case 5: return 8;
    case 6: return 10;
    case 7: return 11;
    default: return 0;
  }
}

static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  // Rows 3-5: col 0 = wavetable, col 1 = tone
  if (row >= 3 && row <= 5) return 2;
  // Rows 6-7: col 0 = noise only
  if (row == 6 || row == 7) return 1;
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  const ColorScheme cs = appSettings.colorScheme;

  // Wavetable labels (y 6-8)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 6, "Wave");
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 7, "Pitch");
  gfxPrint(COL_LEFT_X, 8, "Fine");

  // Tone header (y 6)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_RIGHT_X, 6, "Tone");

  // Tone labels (y 7-8)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_RIGHT_X, 7, "Pitch");
  gfxPrint(COL_RIGHT_X, 8, "Fine");

  // Noise header (y 10)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_LEFT_X, 10, "Noise");

  // Noise labels (y 11)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 11, "Period");
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  int y = rowToY(row);

  if (col == 0) {
    switch (row) {
      case 3: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Wave index
      case 4: gfxCursor(COL_LEFT_VAL, y, 2); break;  // WT pitch (hex)
      case 5: gfxCursor(COL_LEFT_VAL, y, 2); break;  // WT fine (hex)
      case 6: gfxCursor(COL_LEFT_VAL, y, 3); break;  // Noise on/off
      case 7: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Noise period
    }
  } else if (col == 1) {
    switch (row) {
      case 3: gfxCursor(COL_RIGHT_VAL, y, 3); break; // Tone on/off
      case 4: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Tone pitch (hex)
      case 5: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Tone fine (hex)
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  InstrumentAYWavetable* wt = &chipnomadState->project.instruments[cInstrument].chip.ayWavetable;
  int y = rowToY(row);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (col == 0) {
    switch (row) {
      case 3: // Wave index
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex(wt->waveIndex));
        break;
      case 4: // WT pitch
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)wt->pitchOffset));
        break;
      case 5: // WT fine
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)wt->fineTune));
        break;
      case 6: // Noise on/off
        gfxPrintf(COL_LEFT_VAL, y, wt->oscNoise.isOn ? "On " : "Off");
        break;
      case 7: // Noise period
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex(wt->oscNoise.noisePeriod));
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 3: // Tone on/off
        gfxPrintf(COL_RIGHT_VAL, y, wt->oscTone.isOn ? "On " : "Off");
        break;
      case 4: // Tone pitch
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)wt->oscTone.pitchOffset));
        break;
      case 5: // Tone fine
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)wt->oscTone.fineTune));
        break;
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;
  InstrumentAYWavetable* wt = &chipnomadState->project.instruments[cInstrument].chip.ayWavetable;

  if (col == 0) {
    switch (row) {
      case 3: // Wave index
        handled = edit8noLast(action, &wt->waveIndex, 16, 0, 255);
        break;
      case 4: // WT pitch
        handled = editSigned8(action, &wt->pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Wavetable pitch offset %hhd", wt->pitchOffset);
        }
        break;
      case 5: // WT fine
        handled = editSigned8(action, &wt->fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Wavetable fine tune %hhd", wt->fineTune);
        }
        break;
      case 6: // Noise on/off
        handled = edit8noLast(action, &wt->oscNoise.isOn, 1, 0, 1);
        break;
      case 7: // Noise period
        handled = edit8noLast(action, &wt->oscNoise.noisePeriod, 8, 0, 31);
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 3: // Tone on/off
        handled = edit8noLast(action, &wt->oscTone.isOn, 1, 0, 1);
        break;
      case 4: // Tone pitch
        handled = editSigned8(action, &wt->oscTone.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Tone pitch offset %hhd", wt->oscTone.pitchOffset);
        }
        break;
      case 5: // Tone fine
        handled = editSigned8(action, &wt->oscTone.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Tone fine tune %hhd", wt->oscTone.fineTune);
        }
        break;
    }
  }

  if (handled) projectModified = 1;
  return handled;
}

static int isCellValid(int col, int row) {
  if ((row == 6 || row == 7) && col == 1) return 0;
  return 1;
}

ScreenData screenInstrumentAYWavetable = {
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
