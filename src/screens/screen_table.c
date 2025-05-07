#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <help.h>

static int table = 0;
static int backToPhrase = 0;
static uint8_t lastPitchValue = 0;
static uint8_t lastVolume = 15;
static uint8_t lastFX[2] = {0, 0};
static int isFxEdit = 0;

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static int onEdit(int col, int row, enum CellEditAction action);

static struct ScreenData screen = {
  .rows = 16,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

static void setup(int input) {
  isFxEdit = 0;
  table = input & 0xff;
  backToPhrase = (input & 0x1000) != 0;
}

static int getColumnCount(int row) {
  return 11; // PitchFlag, Pitch, Volume, FX1, FX1 Value, FX2, FX2 Value, FX3, FX3 Value, FX4, FX4 Value
}
static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "TABLE %02X", table);
}

static void drawField(int col, int row, int state) {
  if (col == 0) {
    // Pitch flag
    uint8_t pitchFlag = project.tables[table].pitchFlags[row];
    setCellColor(state, 0, 1);
    gfxPrint(3, 3 + row, pitchFlag ? "=" : "~");
  } else if (col == 1) {
    // Pitch offset
    uint8_t pitch = project.tables[table].pitchOffsets[row];
    setCellColor(state, 0, 1);
    gfxPrint(4, 3 + row, byteToHex(pitch));
  } else if (col == 2) {
    // Volume
    uint8_t volume = project.tables[table].volumes[row];
    setCellColor(state, volume == EMPTY_VALUE_8, 1);
    gfxPrint(7, 3 + row, byteToHexOrEmpty(volume));
  } else if (col % 2 == 1 && col >= 3) {
    // FX name (columns 3,5,7,9)
    int fxIdx = (col - 3) / 2;
    uint8_t fx = project.tables[table].fx[row][fxIdx][0];
    setCellColor(state, fx == EMPTY_VALUE_8, 1);
    gfxPrint(10 + (fxIdx * 6), 3 + row, fxNames[fx].name);
  } else if (col % 2 == 0 && col >= 4) {
    // FX value (columns 4,6,8,10)
    int fxIdx = (col - 4) / 2;
    uint8_t value = project.tables[table].fx[row][fxIdx][1];
    setCellColor(state, 0, project.tables[table].fx[row][fxIdx][0] != EMPTY_VALUE_8);
    gfxPrint(13 + (fxIdx * 6), 3 + row, byteToHex(value));
  }
}
static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  switch (col) {
    case 0:
    case 1:
      gfxPrint(3, 2, "P");
      break;
    case 2:
      gfxPrint(7, 2, "V");
      break;
    case 3:
    case 4:
      gfxPrint(10, 2, "FX1");
      break;
    case 5:
    case 6:
      gfxPrint(16, 2, "FX2");
      break;
    case 7:
    case 8:
      gfxPrint(22, 2, "FX3");
      break;
    case 9:
    case 10:
      gfxPrint(28, 2, "FX4");
      break;
  }
}

static void drawCursor(int col, int row) {
  int width = 1;
  int x;

  if (col == 0) {
    x = 3;
  } else if (col == 1) {
    x = 4;
    width = 2;
  } else if (col == 2) {
    x = 7;
    width = 2;
  } else {
    // For FX columns
    int fxIdx = (col - 3) / 2;
    if (col % 2 == 1) {
      // FX name columns
      width = 3;
      x = 10 + (fxIdx * 6);
    } else {
      // FX value columns
      width = 2;
      x = 13 + ((col - 4) / 2 * 6);
    }
  }

  gfxCursor(x, 3 + row, width);
}

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;
  uint8_t maxVolume = 15;

  if (col == 0) {
    // Pitch flag (toggle between 0 and 1)
    handled = edit8noLast(action, &project.tables[table].pitchFlags[row], 1, 0, 1);
    if (handled) {
      screenMessage(project.tables[table].pitchFlags[row] ? "Absolute pitch" : "Pitch offset", 0);
    }
  } else if (col == 1) {
    // Pitch offset
    handled = edit8noLimit(action, &project.tables[table].pitchOffsets[row], &lastPitchValue, project.pitchTable.octaveSize);
    if (handled) {
      if (project.tables[table].pitchFlags[row] == 1) {
        // If pitch flag is 1, pitch is absolute
        screenMessage("Note %s", noteName(project.tables[table].pitchOffsets[row]));
      } else {
        // If pitch flag is 0, pitch is offset
        screenMessage("Pitch offset %hhd", project.tables[table].pitchOffsets[row]);
      }
    }
  } else if (col == 2) {
    // Volume
    handled = edit8withLimit(action, &project.tables[table].volumes[row], &lastVolume, 16, maxVolume);
  } else if (col % 2 == 1 && col >= 3) {
    // FX (columns 3,5,7,9)
    int fxIdx = (col - 3) / 2;
    int result = editFX(action, project.tables[table].fx[row][fxIdx], lastFX, 1);
    if (result == 2) {
      drawField(col + 1, row, 0);
      handled = 1;
    } else if (result == 1) {
      isFxEdit = 1;
      handled = 0;
    }
  } else if (col % 2 == 0 && col >= 4) {
    // FX value (columns 4,6,8,10)
    int fxIdx = (col - 4) / 2;
    if (project.tables[table].fx[row][fxIdx][0] != EMPTY_VALUE_8) {
      handled = editFXValue(action, project.tables[table].fx[row][fxIdx], lastFX, 1);
    }
  }

  return handled;
}

static void fullRedraw(void) {
  screenFullRedraw(&screen);
}

static void draw(void) {
  if (isFxEdit) return;

  gfxClearRect(2, 3, 1, 16);
  gfxClearRect(9, 3, 1, 16);
  gfxClearRect(15, 3, 1, 16);
  gfxClearRect(21, 3, 1, 16);
  gfxClearRect(27, 3, 1, 16);

  struct PlaybackTrackState* track = &playback.tracks[*pSongTrack];
  struct PlaybackTableState* pTable = NULL;
  if (track->mode != playbackModeStopped) {
    int instrumentTableIdx = track->note.instrumentTable.tableIdx;
    int auxTableIdx = track->note.auxTable.tableIdx;
    if (table == instrumentTableIdx) {
      pTable = &track->note.instrumentTable;
    } else if (table == auxTableIdx) {
      pTable = &track->note.auxTable;
    }
  }
  if (pTable != NULL) {
    gfxSetFgColor(appSettings.colorScheme.playMarkers);
    int row = pTable->rows[0];
    if (row >= 0 && row < 16) {
      gfxPrint(2, 3 + row, ">");
      gfxPrint(9, 3 + row, ">");
    }
    row = pTable->rows[1];
    gfxPrint(15, 3 + row, ">");
    row = pTable->rows[2];
    gfxPrint(21, 3 + row, ">");
    row = pTable->rows[3];
    gfxPrint(27, 3 + row, ">");
  }
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyLeft | keyShift)) {
    // Back to the instrument or phrase screen
    if (backToPhrase) {
      screenSetup(&screenPhrase, -1);
    } else {
      screenSetup(&screenInstrument, -1);
    }
    return 1;
  } if (keys == (keyLeft | keyOpt)) {
    // Previous table
    if (table > 0) {
      table--;
      fullRedraw();
    }
    return 1;
  } if (keys == (keyRight | keyOpt)) {
    // Next table
    if (table < PROJECT_MAX_TABLES) {
      table++;
      fullRedraw();
    }
  } if (keys == (keyUp | keyOpt)) {
    // +16 tables
    table += 16;
    if (table >= PROJECT_MAX_TABLES) table = PROJECT_MAX_TABLES - 1;
    fullRedraw();
    return 1;
  } if (keys == (keyDown | keyOpt)) {
    // -16 tables
    table -= 16;
    if (table < 0) table = 0;
    fullRedraw();
    return 1;
  }

  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (isFxEdit) {
    int fxIdx = (screen.cursorCol - 2) / 2;
    int result = fxEditInput(keys, isDoubleTap, project.tables[table].fx[screen.cursorRow][fxIdx], lastFX);
    if (result) {
      isFxEdit = 0;
      fullRedraw();
    }
    return;
  }

  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (screenInput(&screen, keys, isDoubleTap)) return;
}

const struct AppScreen screenTable = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
