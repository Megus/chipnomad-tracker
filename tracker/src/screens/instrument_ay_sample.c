#include "screen_instrument.h"
#include "corelib_gfx.h"
#include "corelib/corelib_file.h"
#include "utils.h"
#include "file_browser.h"
#include <string.h>

// Screen layout:
// y 0: INSTRUMENT 00
// y 2: Type  AY Sample  Load Save
// y 3: Name  _______________
// y 4: Transp. On       Tbl. Tic 01
// y 5: Sample [name________] Load
// y 6: (spacing)
// y 7: Rate  [8000 ]    Tone   [On ]
// y 8: Start [0000]     Pitch  [+0  ]
// y 9: Len   [0000]     Fine   [+0  ]
// y 10: LpSt  [0000]
// y 11: LpEnd [0000]
// y 12: Pitch [+0  ]    Noise  [Off]
// y 13: Fine  [+0  ]    Period [00]
//
// Logical rows:
// 0-2: common (type, name, transpose/tic)
// 3: sample name (col 0) + Load button (col 1)
// 4: Rate         | Tone on/off
// 5: Start        | Tone pitch
// 6: Length       | Tone fine
// 7: Loop Start   | (empty)
// 8: Loop End     | (empty)
// 9: Pitch        | Noise on/off
// 10: Fine        | Noise period

#define COL_LEFT_X    0
#define COL_LEFT_VAL  8
#define COL_RIGHT_X   17
#define COL_RIGHT_VAL 26

#define ROW_TOTAL 11

static int rowToY(int row) {
  switch (row) {
    case 3: return 6;
    case 4: return 8;
    case 5: return 9;
    case 6: return 10;
    case 7: return 11;
    case 8: return 12;
    case 9: return 13;
    case 10: return 14;
    default: return 0;
  }
}

static void onSampleLoaded(const char* path) {
  // TODO: Actually load the WAV file data
  // For now, just store the filename
  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;
  char* lastSeparator = strrchr(path, PATH_SEPARATOR);
  const char* filename = lastSeparator ? lastSeparator + 1 : path;

  // Strip extension and copy name
  const char* dot = strrchr(filename, '.');
  int nameLen = dot ? (int)(dot - filename) : (int)strlen(filename);
  if (nameLen > PROJECT_INSTRUMENT_NAME_LENGTH) nameLen = PROJECT_INSTRUMENT_NAME_LENGTH;
  strncpy(smp->sampleName, filename, nameLen);
  smp->sampleName[nameLen] = 0;

  // Save the directory path
  if (lastSeparator) {
    int pathLen = lastSeparator - path;
    if (pathLen < PATH_LENGTH) {
      strncpy(appSettings.samplePath, path, pathLen);
      appSettings.samplePath[pathLen] = 0;
    }
  }

  projectModified = 1;
  screenMessage(MESSAGE_TIME, "Sample loaded");
  screenSetup(&screenInstrument, cInstrument);
}

static void onSampleCancelled(void) {
  screenSetup(&screenInstrument, cInstrument);
}

static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  if (row == 3) return 2; // Sample name + Load button
  // Rows 4-6: col 0 = sample params, col 1 = tone
  if (row >= 4 && row <= 6) return 2;
  // Rows 7-8: col 0 = sample params only
  if (row == 7 || row == 8) return 1;
  // Rows 9-10: col 0 = sample pitch/fine, col 1 = noise
  if (row == 9 || row == 10) return 2;
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();

  const ColorScheme cs = appSettings.colorScheme;

  // Sample name row (y 6)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 6, "Sample");

  // Sample params labels (y 8-14)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_LEFT_X, 8, "Rate");
  gfxPrint(COL_LEFT_X, 9, "Start");
  gfxPrint(COL_LEFT_X, 10, "Length");
  gfxPrint(COL_LEFT_X, 11, "LoopSt");
  gfxPrint(COL_LEFT_X, 12, "LoopEnd");
  gfxPrint(COL_LEFT_X, 13, "Pitch");
  gfxPrint(COL_LEFT_X, 14, "Fine");

  // Tone header (y 8)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_RIGHT_X, 8, "Tone");

  // Tone labels (y 9-10)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_RIGHT_X, 9, "Pitch");
  gfxPrint(COL_RIGHT_X, 10, "Fine");

  // Noise header (y 13)
  gfxSetFgColor(cs.textTitles);
  gfxPrint(COL_RIGHT_X, 13, "Noise");

  // Noise labels (y 14)
  gfxSetFgColor(cs.textDefault);
  gfxPrint(COL_RIGHT_X, 14, "Period");
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);

  int y = rowToY(row);

  if (row == 3) {
    if (col == 0) {
      // Sample name (read-only, but cursor can be here)
      gfxCursor(COL_LEFT_VAL, y, PROJECT_INSTRUMENT_NAME_LENGTH);
    } else if (col == 1) {
      // Load button
      gfxCursor(COL_RIGHT_VAL, y, 4);
    }
    return;
  }

  if (col == 0) {
    switch (row) {
      case 4: gfxCursor(COL_LEFT_VAL, y, 5); break;  // Rate (decimal, up to 5 digits)
      case 5: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Start
      case 6: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Length
      case 7: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Loop Start
      case 8: gfxCursor(COL_LEFT_VAL, y, 4); break;  // Loop End
      case 9: gfxCursor(COL_LEFT_VAL, y, 2); break;  // Pitch (hex)
      case 10: gfxCursor(COL_LEFT_VAL, y, 2); break; // Fine (hex)
    }
  } else if (col == 1) {
    switch (row) {
      case 4: gfxCursor(COL_RIGHT_VAL, y, 3); break; // Tone on/off
      case 5: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Tone pitch (hex)
      case 6: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Tone fine (hex)
      case 9: gfxCursor(COL_RIGHT_VAL, y, 3); break; // Noise on/off
      case 10: gfxCursor(COL_RIGHT_VAL, y, 2); break; // Noise period
    }
  }
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);

  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;
  int y = rowToY(row);

  gfxSetFgColor(state == stateFocus ? appSettings.colorScheme.textValue : appSettings.colorScheme.textDefault);

  if (row == 3) {
    if (col == 0) {
      // Sample name (read-only display)
      gfxClearRect(COL_LEFT_VAL, y, PROJECT_INSTRUMENT_NAME_LENGTH, 1);
      if (strlen(smp->sampleName) > 0) {
        gfxPrint(COL_LEFT_VAL, y, smp->sampleName);
      } else {
        gfxPrint(COL_LEFT_VAL, y, "---");
      }
    } else if (col == 1) {
      // Load button
      gfxPrint(COL_RIGHT_VAL, y, "Load");
    }
    return;
  }

  if (col == 0) {
    switch (row) {
      case 4: // Rate
        gfxClearRect(COL_LEFT_VAL, y, 5, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%d", smp->sampleRate);
        break;
      case 5: // Start
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleStart);
        break;
      case 6: // Length
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleLength);
        break;
      case 7: // Loop Start
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleLoopStart);
        break;
      case 8: // Loop End
        gfxClearRect(COL_LEFT_VAL, y, 4, 1);
        gfxPrintf(COL_LEFT_VAL, y, "%04X", smp->sampleLoopEnd);
        break;
      case 9: // Pitch
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)smp->pitchOffset));
        break;
      case 10: // Fine
        gfxClearRect(COL_LEFT_VAL, y, 2, 1);
        gfxPrint(COL_LEFT_VAL, y, byteToHex((uint8_t)smp->fineTune));
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 4: // Tone on/off
        gfxPrintf(COL_RIGHT_VAL, y, smp->oscTone.isOn ? "On " : "Off");
        break;
      case 5: // Tone pitch
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)smp->oscTone.pitchOffset));
        break;
      case 6: // Tone fine
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex((uint8_t)smp->oscTone.fineTune));
        break;
      case 9: // Noise on/off
        gfxPrintf(COL_RIGHT_VAL, y, smp->oscNoise.isOn ? "On " : "Off");
        break;
      case 10: // Noise period
        gfxClearRect(COL_RIGHT_VAL, y, 2, 1);
        gfxPrint(COL_RIGHT_VAL, y, byteToHex(smp->oscNoise.noisePeriod));
        break;
    }
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  int handled = 0;
  InstrumentAYSample* smp = &chipnomadState->project.instruments[cInstrument].chip.aySample;

  if (row == 3) {
    if (col == 1) {
      // Load sample
      fileBrowserSetup("LOAD SAMPLE", ".wav", appSettings.samplePath, onSampleLoaded, onSampleCancelled);
      screenSetup(&screenFileBrowser, 0);
    }
    return 0;
  }

  if (col == 0) {
    switch (row) {
      case 4: // Rate
        handled = edit16withOverflow(action, &smp->sampleRate, 1000, 1000, 44100);
        break;
      case 5: // Start
        handled = edit16withOverflow(action, &smp->sampleStart, 256, 0, 0x3FFF);
        break;
      case 6: // Length
        handled = edit16withOverflow(action, &smp->sampleLength, 256, 0, 0x3FFF);
        break;
      case 7: // Loop Start
        handled = edit16withOverflow(action, &smp->sampleLoopStart, 256, 0, 0x3FFF);
        break;
      case 8: // Loop End
        handled = edit16withOverflow(action, &smp->sampleLoopEnd, 256, 0, 0x3FFF);
        break;
      case 9: // Pitch
        handled = editSigned8(action, &smp->pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Sample pitch offset %hhd", smp->pitchOffset);
        }
        break;
      case 10: // Fine
        handled = editSigned8(action, &smp->fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Sample fine tune %hhd", smp->fineTune);
        }
        break;
    }
  } else if (col == 1) {
    switch (row) {
      case 4: // Tone on/off
        handled = edit8noLast(action, &smp->oscTone.isOn, 1, 0, 1);
        break;
      case 5: // Tone pitch
        handled = editSigned8(action, &smp->oscTone.pitchOffset, chipnomadState->project.pitchTable.octaveSize, -128, 127);
        if (handled) {
          screenMessage(0, "Tone pitch offset %hhd", smp->oscTone.pitchOffset);
        }
        break;
      case 6: // Tone fine
        handled = editSigned8(action, &smp->oscTone.fineTune, 16, -128, 127);
        if (handled) {
          screenMessage(0, "Tone fine tune %hhd", smp->oscTone.fineTune);
        }
        break;
      case 9: // Noise on/off
        handled = edit8noLast(action, &smp->oscNoise.isOn, 1, 0, 1);
        break;
      case 10: // Noise period
        handled = edit8noLast(action, &smp->oscNoise.noisePeriod, 8, 0, 31);
        break;
    }
  }

  if (handled) projectModified = 1;
  return handled;
}

static int isCellValid(int col, int row) {
  // Rows 7-8 only have left column (loop params)
  if ((row == 7 || row == 8) && col == 1) return 0;
  return 1;
}

ScreenData screenInstrumentAYSample = {
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
