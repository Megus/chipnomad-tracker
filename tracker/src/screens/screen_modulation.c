#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "screen_instrument.h"
#include <string.h>

// Screen layout (20 rows):
// y 0: "MODULATION 00"
// y 1: (empty)
// y 2:  Mod1  [type]       Mod3  [type]
// y 3:  Dest  Off          Dest  Off
// y 4:  Amt   [00]         Amt   [00]
// y 5:  p1 label [val]     p1 label [val]
// y 6:  p2 label [val]     p2 label [val]
// y 7:  p3 label [val]     p3 label [val]
// y 8:  p4 label [val]     p4 label [val]
// y 9:  (spacing)
// y 10: Mod2  [type]       Mod4  [type]
// y 11: Dest  Off          Dest  Off
// y 12: Amt   [00]         Amt   [00]
// y 13: p1 label [val]     p1 label [val]
// y 14: p2 label [val]     p2 label [val]
// y 15: p3 label [val]     p3 label [val]
// y 16: p4 label [val]     p4 label [val]
//
// Logical rows (per modulator, 7 rows each):
// Top block (Mod1 left, Mod3 right): rows 0-6
// Bottom block (Mod2 left, Mod4 right): rows 7-13
//
// Column mapping:
// col 0 = left modulator (Mod1 top, Mod2 bottom)
// col 1 = right modulator (Mod3 top, Mod4 bottom)

#define COL_LEFT_X    0
#define COL_LEFT_VAL  7
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 24

#define ROW_TOTAL 14
#define ROWS_PER_MOD 7

static ScreenData screenData;

// Map logical row to screen Y
static int rowToY(int row) {
  if (row < 7) return row + 2;   // Top block: y 2-8
  return row + 3;                 // Bottom block: y 10-16
}

// Map (col, row) to modulator index (0-3)
static int getModIndex(int col, int row) {
  if (row < 7) return col == 0 ? 0 : 2;  // Top: Mod1 (left), Mod3 (right)
  return col == 0 ? 1 : 3;                // Bottom: Mod2 (left), Mod4 (right)
}

// Row within a modulator block (0-6)
static int getModRow(int row) {
  return row < 7 ? row : row - 7;
}

static const char* modTypeName(enum ModulationType type) {
  switch (type) {
    case modADSR: return "ADSR";
    case modAHD:  return "AHD ";
    case modLFO:  return "LFO ";
    default:      return "?   ";
  }
}

static const char* lfoShapeName(uint8_t shape) {
  switch (shape) {
    case lfoShapeTri:      return "Tri   ";
    case lfoShapeSin:      return "Sin   ";
    case lfoShapeRampDown: return "RampDn";
    case lfoShapeRampUp:   return "RampUp";
    case lfoShapeExpDown:  return "ExpDn ";
    case lfoShapeExpUp:    return "ExpUp ";
    case lfoShapeSquare:   return "Square";
    case lfoShapeRandom:   return "Random";
    default:               return "?     ";
  }
}

static const char* lfoTrigName(uint8_t trig) {
  switch (trig) {
    case lfoTrigFree:   return "Free  ";
    case lfoTrigRetrig: return "Retrig";
    case lfoTrigHold:   return "Hold  ";
    case lfoTrigOnce:   return "Once  ";
    default:            return "?     ";
  }
}

// Get the parameter label for a given modulator type and parameter index (0-3)
static const char* paramLabel(enum ModulationType type, int paramIdx) {
  switch (type) {
    case modADSR:
      if (paramIdx == 0) return "Atk";
      if (paramIdx == 1) return "Dec";
      if (paramIdx == 2) return "Sus";
      if (paramIdx == 3) return "Rel";
      break;
    case modAHD:
      if (paramIdx == 0) return "Atk";
      if (paramIdx == 1) return "Hold";
      if (paramIdx == 2) return "Dec";
      break;
    case modLFO:
      if (paramIdx == 0) return "Shape";
      if (paramIdx == 1) return "Trig";
      if (paramIdx == 2) return "Period";
      break;
    default:
      break;
  }
  return NULL;
}

// How many parameter rows does this modulation type have?
static int paramCount(enum ModulationType type) {
  switch (type) {
    case modADSR: return 4;
    case modAHD:  return 3;
    case modLFO:  return 3;
    default:      return 0;
  }
}

static int isCellValid(int col, int row) {
  if (chipnomadState->project.instruments[cInstrument].type == instNone) return 0;

  int modRow = getModRow(row);
  if (modRow <= 2) return 1; // Type, Dest, Amt always valid

  int modIdx = getModIndex(col, row);
  Modulation* mod = &chipnomadState->project.instruments[cInstrument].modulation[modIdx];
  int paramIdx = modRow - 3;
  return paramIdx < paramCount(mod->type);
}

static int getColumnCount(int row) {
  return 2;
}

static void drawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "MODULATION %02X", cInstrument);

  if (chipnomadState->project.instruments[cInstrument].type == instNone) return;

  for (int block = 0; block < 2; block++) {
    int baseY = block == 0 ? 2 : 10;

    gfxSetFgColor(cs.textTitles);
    gfxPrintf(COL_LEFT_X, baseY, "Mod%d", block == 0 ? 1 : 2);
    gfxPrintf(COL_RIGHT_X, baseY, "Mod%d", block == 0 ? 3 : 4);

    gfxSetFgColor(cs.textDefault);
    gfxPrint(COL_LEFT_X, baseY + 1, "Dest");
    gfxPrint(COL_LEFT_X, baseY + 2, "Amt");
    gfxPrint(COL_RIGHT_X, baseY + 1, "Dest");
    gfxPrint(COL_RIGHT_X, baseY + 2, "Amt");
  }
}

static void drawCursor(int col, int row) {
  int y = rowToY(row);
  int valX = col == 0 ? COL_LEFT_VAL : COL_RIGHT_VAL;
  int modRow = getModRow(row);
  int modIdx = getModIndex(col, row);
  Modulation* mod = &chipnomadState->project.instruments[cInstrument].modulation[modIdx];

  switch (modRow) {
    case 0: gfxCursor(valX, y, 4); break; // Type
    case 1: gfxCursor(valX, y, 7); break; // Dest
    case 2: gfxCursor(valX, y, 2); break; // Amt
    default:
      if (mod->type == modLFO && (modRow - 3) <= 1) {
        gfxCursor(valX, y, 6); // LFO shape/trig names
      } else {
        gfxCursor(valX, y, 2); // Hex values
      }
      break;
  }
}

static void drawField(int col, int row, int state) {
  if (chipnomadState->project.instruments[cInstrument].type == instNone) return;

  int y = rowToY(row);
  int valX = col == 0 ? COL_LEFT_VAL : COL_RIGHT_VAL;
  int labelX = col == 0 ? COL_LEFT_X : COL_RIGHT_X;
  int modRow = getModRow(row);
  int modIdx = getModIndex(col, row);
  Modulation* mod = &chipnomadState->project.instruments[cInstrument].modulation[modIdx];

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  switch (modRow) {
    case 0: // Type (on header row)
      gfxPrint(valX, y, modTypeName(mod->type));
      break;
    case 1: // Destination
      gfxClearRect(valX, y, 7, 1);
      {
        Instrument* inst = &chipnomadState->project.instruments[cInstrument];
        InstrumentFunctions funcs = getInstrumentFunctions(inst->type);
        if (funcs.modName) {
          gfxPrint(valX, y, funcs.modName(mod->destination));
        }
      }
      break;
    case 2: // Amount
      gfxClearRect(valX, y, 2, 1);
      gfxPrint(valX, y, byteToHex(mod->amount));
      break;
    default: {
      int paramIdx = modRow - 3;
      if (paramIdx >= paramCount(mod->type)) break;

      // Draw the label
      gfxSetFgColor(appSettings.colorScheme.textDefault);
      gfxClearRect(labelX, y, 6, 1);
      const char* label = paramLabel(mod->type, paramIdx);
      if (label) gfxPrint(labelX, y, label);

      // Draw the value
      gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);
      gfxClearRect(valX, y, 6, 1);

      if (mod->type == modAHD || mod->type == modADSR) {
        uint8_t* params = &mod->p1;
        gfxPrint(valX, y, byteToHex(params[paramIdx]));
      } else if (mod->type == modLFO) {
        if (paramIdx == 0) {
          gfxPrint(valX, y, lfoShapeName(mod->p1));
        } else if (paramIdx == 1) {
          gfxPrint(valX, y, lfoTrigName(mod->p2));
        } else if (paramIdx == 2) {
          gfxPrint(valX, y, byteToHex(mod->p3));
        }
      }
      break;
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (chipnomadState->project.instruments[cInstrument].type == instNone) return 0;

  int handled = 0;
  int modRow = getModRow(row);
  int modIdx = getModIndex(col, row);
  Modulation* mod = &chipnomadState->project.instruments[cInstrument].modulation[modIdx];

  switch (modRow) {
    case 0: { // Type
      uint8_t type = (uint8_t)mod->type;
      uint8_t oldType = type;
      handled = edit8noLast(action, &type, 1, 0, modTotalCount - 1);
      mod->type = (enum ModulationType)type;
      if (oldType != type) {
        mod->p1 = 0;
        mod->p2 = 0;
        mod->p3 = (mod->type == modADSR) ? 255 : 6;
        mod->p4 = 0;
        screenFullRedraw(&screenData);
      }
      break;
    }
    case 1: { // Destination
      Instrument* inst = &chipnomadState->project.instruments[cInstrument];
      InstrumentFunctions funcs = getInstrumentFunctions(inst->type);
      handled = edit8noLast(action, &mod->destination, 1, 0, funcs.modDestinationsCount);
      break;
    }
    case 2: // Amount
      handled = editSigned8(action, &mod->amount, 16, -128, 127);
      if (handled) {
        screenMessage(0, "Modulation amount %hhd", mod->amount);
      }
      break;
    default: {
      int paramIdx = modRow - 3;
      if (paramIdx >= paramCount(mod->type)) break;

      if (mod->type == modAHD || mod->type == modADSR) {
        uint8_t* params = &mod->p1;
        handled = edit8noLast(action, &params[paramIdx], 16, 0, 255);
        if (handled) {
          // Full field names for AHD and ADSR
          const char* fullName = NULL;
          if (mod->type == modADSR) {
            const char* adsrNames[] = {"Attack", "Decay", "Sustain", "Release"};
            if (paramIdx < 4) fullName = adsrNames[paramIdx];
          } else if (mod->type == modAHD) {
            const char* ahdNames[] = {"Attack", "Hold", "Decay"};
            if (paramIdx < 3) fullName = ahdNames[paramIdx];
          }
          if (fullName) {
            screenMessage(0, "%s %hhu ticks", fullName, params[paramIdx]);
          }
        }
      } else if (mod->type == modLFO) {
        if (paramIdx == 0) {
          handled = edit8noLast(action, &mod->p1, 1, 0, lfoShapeTotalCount - 1);
          // No hint for LFO shape - not adding value
        } else if (paramIdx == 1) {
          handled = edit8noLast(action, &mod->p2, 1, 0, lfoTrigTotalCount - 1);
          // No hint for LFO trigger - not adding value
        } else if (paramIdx == 2) {
          handled = edit8noLast(action, &mod->p3, 16, 0, 255);
          if (handled) {
            screenMessage(0, "Period %hhu ticks", mod->p3);
          }
        }
      }
      break;
    }
  }

  if (handled) projectModified = 1;
  return handled;
}

static void fullRedraw(void) {
  screenFullRedraw(&screenData);
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  if (keys == 0) {
    playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
  }

  if (keys == (keyDown | keyShift)) {
    // To Instrument screen
    screenSetup(&screenInstrument, cInstrument);
    return 1;
  } else if (keys == (keyOpt | keyLeft)) {
    if (cInstrument != 0) {
      cInstrument--;
      playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyOpt | keyRight)) {
    if (cInstrument != PROJECT_MAX_INSTRUMENTS - 1) {
      cInstrument++;
      playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyOpt | keyUp)) {
    cInstrument += 16;
    if (cInstrument >= PROJECT_MAX_INSTRUMENTS) cInstrument = PROJECT_MAX_INSTRUMENTS - 1;
    playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
    fullRedraw();
    return 1;
  } else if (keys == (keyOpt | keyDown)) {
    cInstrument -= 16;
    if (cInstrument < 0) cInstrument = 0;
    playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
    fullRedraw();
    return 1;
  } else if (keys == (keyEdit | keyPlay)) {
    if (!instrumentIsEmpty(&chipnomadState->project, cInstrument) && !playbackIsPlaying(&chipnomadState->playbackState)) {
      uint8_t note = instrumentFirstNote(&chipnomadState->project, cInstrument);
      playbackPreviewNote(&chipnomadState->playbackState, *pSongTrack, note, cInstrument);
    }
    return 1;
  }

  if (screenInput(&screenData, isKeyDown, keys, tapCount)) return 1;

  return 0;
}

static void init(void) {
}

static void setup(int input) {
  if (input != -1) {
    cInstrument = input;
  }
}

static void draw(void) {
}

static void drawRowHeader(int row, int state) {}
static void drawColHeader(int col, int state) {}

static ScreenData screenData = {
  .rows = ROW_TOTAL,
  .cursorRow = 0,
  .cursorCol = 0,
  .selectMode = -1,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
  .isCellValid = isCellValid,
};

static enum ScreenPlaybackLevel getPlaybackLevel(void) {
  return screenPlaybackPhrase;
}

const AppScreen screenModulation = {
  .init = init,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .getPlaybackLevel = getPlaybackLevel
};
