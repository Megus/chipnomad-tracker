#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "copy_paste.h"
#include "help.h"

static int phraseIdx = 0;
static PhraseRow *phraseRows = NULL;
static int isFxEdit = 0;

static uint8_t lastNote = 48;
static uint8_t lastInstrument = 0;
static uint8_t lastVolume = 15;
static uint8_t lastFX[2] = {0, 0};

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static void drawSelection(int col1, int row1, int col2, int row2);
static int onEdit(int col, int row, enum CellEditAction action);

static int columnX[] = {3, 7, 10, 13, 16, 19, 22, 25, 28, 31};

static ScreenData screen = {
  .rows = 16,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = 0,
  .selectStartRow = 0,
  .selectStartCol = 0,
  .selectAnchorRow = 0,
  .selectAnchorCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawSelection = drawSelection,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

static void init(void) {
  lastNote = 48;
  lastInstrument = 0;
  lastVolume = 15;
  lastFX[0] = 0;
  lastFX[1] = 0;
  screen.cursorRow = 0;
  screen.cursorCol = 0;
  screen.topRow = 0;
  screen.selectMode = 0;
  screen.selectStartRow = 0;
  screen.selectStartCol = 0;
  screen.selectAnchorRow = 0;
  screen.selectAnchorCol = 0;
  isFxEdit = 0;
}

static void setup(int input) {
  phraseIdx = chipnomadState->project.chains[chipnomadState->project.song[*pSongRow][*pSongTrack]].rows[*pChainRow].phrase;
  phraseRows = chipnomadState->project.phrases[phraseIdx].rows;
  screen.selectMode = 0;
  isFxEdit = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static int getColumnCount(int row) {
  return 9;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "PHRASE %03X%c", phraseIdx, isPhraseUsedElsewhere(&chipnomadState->project, phraseIdx, chipnomadState->project.song[*pSongRow][*pSongTrack], *pChainRow) ? '*' : ' ');
}

static void fullRedraw(void) {
  screenFullRedraw(&screen);
}

static void drawField(int col, int row, int state) {
  int x = columnX[col];
  int y = 3 + row;
  if (col == 0) {
    // Note
    uint8_t value = phraseRows[row].note;
    setCellColor(state, value == EMPTY_VALUE_8, 1);
    gfxPrint(x, y, noteName(&chipnomadState->project, value));
  } else if (col == 1 || col == 2) {
    // Instrument and volume
    uint8_t value = (col == 1) ? phraseRows[row].instrument : phraseRows[row].volume;
    setCellColor(state, value == EMPTY_VALUE_8, 1);
    gfxPrint(x, y, byteToHexOrEmpty(value));
  } else if (col == 3 || col == 5 || col == 7) {
    // FX name
    uint8_t fx = phraseRows[row].fx[(col - 3) / 2][0];
    setCellColor(state, fx == EMPTY_VALUE_8, 1);
    gfxPrint(x, y, fxNames[fx].name);
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    uint8_t value = phraseRows[row].fx[(col - 4) / 2][1];
    setCellColor(state, 0, phraseRows[row].fx[(col - 3) / 2][0] != EMPTY_VALUE_8);
    gfxPrint(x, y, byteToHex(value));
  }
}

static void drawRowHeader(int row, int state) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : ((row & 3) == 0 ? cs.textValue : cs.textInfo));
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);

  switch (col) {
    case 0:
      gfxPrint(3, 2, "N");
      break;
    case 1:
      gfxPrint(7, 2, "I");
      break;
    case 2:
      gfxPrint(10, 2, "V");
      break;
    case 3:
    case 4:
      gfxPrint(13, 2, "FX1");
      break;
    case 5:
    case 6:
      gfxPrint(19, 2, "FX2");
      break;
    case 7:
    case 8:
      gfxPrint(25, 2, "FX3");
      break;
    default:
      break;
  }
}

static void drawCursor(int col, int row) {
  int width = 2;
  if (col == 0 || col == 3 || col == 5 || col == 7) width = 3;
  gfxCursor(col == 0 ? 3 : 4 + col * 3, 3 + row, width);
}

static void drawSelection(int col1, int row1, int col2, int row2) {
  int x = columnX[col1];
  int w = columnX[col2 + 1] - x - 1;
  int y = 3 + row1;
  int h = row2 - row1 + 1;
  if (col2 == 3 || col2 == 5 || col2 == 7) w++;
  gfxRect(x, y, w, h);
}

static void draw(void) {
  if (isFxEdit) return;

  gfxClearRect(0, 3, 1, 16);
  gfxSetFgColor(appSettings.colorScheme.textInfo);
  gfxPrint(0, 3 + *pChainRow, "<");

  gfxClearRect(2, 3, 1, 16);
  PlaybackTrackState* track = &chipnomadState->playbackState.tracks[*pSongTrack];
  if (track->mode != playbackModeStopped && track->mode != playbackModePhraseRow && track->songRow != EMPTY_VALUE_16) {
    // Chain row
    if (*pSongRow == track->songRow) {
      gfxSetFgColor(appSettings.colorScheme.playMarkers);
      gfxPrint(0, 3 + track->chainRow, "<");
    }

    // Phrase row
    int playingPhrase = chipnomadState->project.chains[chipnomadState->project.song[track->songRow][*pSongTrack]].rows[track->chainRow].phrase;
    if (playingPhrase == phraseIdx) {
      int row = track->phraseRow;
      if (row >= 0 && row < 16) {
        gfxSetFgColor(appSettings.colorScheme.playMarkers);
        gfxPrint(2, 3 + row, ">");
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int editCell(int col, int row, enum CellEditAction action) {
  int handled = 0;
  uint8_t maxVolume = 15;

  if (col == 0) {
    // Note
    if (action == editClear && phraseRows[row].note == EMPTY_VALUE_8) {
      // Insert OFF
      phraseRows[row].note = NOTE_OFF;
      phraseRows[row].instrument = EMPTY_VALUE_8;
      phraseRows[row].volume = EMPTY_VALUE_8;
      handled = 1;
    } else if (action == editClear) {
      // Clear note
      handled = edit8withLimit(action, &phraseRows[row].note, &lastNote, chipnomadState->project.pitchTable.octaveSize, chipnomadState->project.pitchTable.length - 1);
      edit8withLimit(action, &phraseRows[row].instrument, &lastInstrument, 16, PROJECT_MAX_INSTRUMENTS - 1);
      edit8withLimit(action, &phraseRows[row].volume, &lastVolume, 16, maxVolume);
    } else if (action == editTap && phraseRows[row].note == EMPTY_VALUE_8) {
      phraseRows[row].note = lastNote;
      phraseRows[row].instrument = lastInstrument;
      phraseRows[row].volume = lastVolume;
      handled = 1;
    } else if (phraseRows[row].note != NOTE_OFF) {
      handled = edit8withLimit(action, &phraseRows[row].note, &lastNote, chipnomadState->project.pitchTable.octaveSize, chipnomadState->project.pitchTable.length - 1);
      if (handled) {
        lastInstrument = phraseRows[row].instrument;
        lastVolume = phraseRows[row].volume;
      }
    }
    if (handled) {
      drawField(1, row, 0);
      drawField(2, row, 0);
    }
  } else if (col == 1) {
    // Instrument
    if (action == editDoubleTap) {
      uint8_t nextInstrument = findEmptyInstrument(&chipnomadState->project, 0);
      if (nextInstrument != EMPTY_VALUE_8) {
        phraseRows[row].instrument = nextInstrument;
        handled = 1;
      }
    } else {
      handled = edit8withLimit(action, &phraseRows[row].instrument, &lastInstrument, 16, PROJECT_MAX_INSTRUMENTS - 1);
    }
    uint8_t instrument = phraseRows[row].instrument;
    if (handled && instrument != EMPTY_VALUE_8) {
      screenMessage(0, "%s: %s", byteToHex(instrument), instrumentName(&chipnomadState->project, instrument));
    }
  } else if (col == 2) {
    // Volume
    handled = edit8withLimit(action, &phraseRows[row].volume, &lastVolume, 16, maxVolume);
  } else if (col == 3 || col == 5 || col == 7) {
    // FX
    int fxIdx = (col - 3) / 2;
    int result = editFX(action, phraseRows[row].fx[fxIdx], lastFX, 0);
    if (result == 2) {
      drawField(col + 1, row, 0);
      handled = 1;
    } else if (result == 1) {
      isFxEdit = 1;
      handled = 0;
    }
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    int fxIdx = (col - 4) / 2;
    if (phraseRows[row].fx[fxIdx][0] != EMPTY_VALUE_8) {
      handled = editFXValue(action, phraseRows[row].fx[fxIdx], lastFX, 0);
    }
  }

  if (handled && (!playbackIsPlaying(&chipnomadState->playbackState) || chipnomadState->playbackState.tracks[*pSongTrack].mode == playbackModePhraseRow)) {
    playbackStartPhraseRow(&chipnomadState->playbackState, *pSongTrack, &phraseRows[screen.cursorRow]);
  }

  return handled;
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (action == editSwitchSelection) {
    return switchPhraseSelectionMode(&screen);
  } else if (action == editMultiIncrease || action == editMultiDecrease) {
    if (!isSingleColumnSelection(&screen)) return 0;
    return applyMultiEdit(&screen, action, editCell);
  } else if (action == editMultiIncreaseBig || action == editMultiDecreaseBig) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);

    // Check if full width selection (all columns)
    if (startCol == 0 && endCol == 8) {
      // Rotation mode
      int direction = (action == editMultiIncreaseBig) ? -1 : 1;
      applyPhraseRotation(phraseIdx, startRow, endRow, direction);
      fullRedraw();
      return 1;
    } else if (isSingleColumnSelection(&screen)) {
      // Single column: big increase/decrease or FX selection
      if (startCol == 3 || startCol == 5 || startCol == 7) {
        // FX type column: fill selection with selected FX type
        int fxIdx = (startCol - 3) / 2;
        fxEditFullDraw(phraseRows[screen.cursorRow].fx[fxIdx][0]);
        isFxEdit = 1;
        return 0;
      } else {
        // Regular big increase/decrease for note, volume, instrument, FX value
        return applyMultiEdit(&screen, action, editCell);
      }
    }
    return 0;
  } else if (action == editCopy) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copyPhrase(phraseIdx, startCol, startRow, endCol, endRow, 0);
    return 1;
  } else if (action == editCut) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copyPhrase(phraseIdx, startCol, startRow, endCol, endRow, 1);
    return 1;
  } else if (action == editPaste) {
    pastePhrase(phraseIdx, col, row);
    fullRedraw();
    return 1;
  } else if (action == editShallowClone) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);

    // Handle instrument column cloning
    if (startCol == 1 && endCol == 1) {
      int distinctCount = cloneInstrumentsInPhrase(phraseIdx, startRow, endRow);
      if (distinctCount == 0) {
        screenMessage(MESSAGE_TIME, "No empty instruments");
        return 0;
      }
      screenMessage(MESSAGE_TIME, "Cloned %d instrument%s", distinctCount, distinctCount == 1 ? "" : "s");
      return 1;
    }
    return 0;
  } else {
    return editCell(col, row, action);
  }
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    // To Instrument/Phrase screen
    int table = -1;
    if (screen.cursorCol > 2) {
      // If we currently on the table command, go to this table
      int fxIdx = (screen.cursorCol - 3) / 2;
      uint8_t fxType = phraseRows[screen.cursorRow].fx[fxIdx][0];
      if (fxType == fxTBL || fxType == fxTBX) {
        table = phraseRows[screen.cursorRow].fx[fxIdx][1];
      }
    }

    if (table >= 0) {
      screenSetup(&screenTable, table | 0x1000);
    } else {
      int instrument = 0;
      for (int row = screen.cursorRow; row >= 0; row--) {
        if (phraseRows[row].instrument != EMPTY_VALUE_8) {
          instrument = phraseRows[row].instrument;
          break;
        }
      }
      screenSetup(&screenInstrument, instrument);
    }
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    // To Chain screen
    screenSetup(&screenChain, -1);
    return 1;
  } else if (keys == (keyUp | keyShift)) {
    // To Groove screen
    int groove = 0;
    if (screen.cursorCol > 2) {
      // If we currently on the groove command, go to this groove
      int fxIdx = (screen.cursorCol - 3) / 2;
      uint8_t fxType = phraseRows[screen.cursorRow].fx[fxIdx][0];
      if (fxType == fxGRV || fxType == fxGGR) {
        groove = phraseRows[screen.cursorRow].fx[fxIdx][1] & (PROJECT_MAX_GROOVES - 1);
      }
    }
    screenSetup(&screenGroove, groove);
    return 1;
  } else if (keys == (keyLeft | keyOpt)) {
    // Previous track
    if (*pSongTrack == 0) return 1;
    uint16_t chain = chipnomadState->project.song[*pSongRow][*pSongTrack - 1];
    if (chain != EMPTY_VALUE_16 && !chainIsEmpty(&chipnomadState->project, chain)) {
      *pSongTrack -= 1;
      while (chipnomadState->project.chains[chain].rows[*pChainRow].phrase == EMPTY_VALUE_16) {
        *pChainRow -= 1;
        if (*pChainRow == 0) break;
      }
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyRight | keyOpt)) {
    // Next track
    if (*pSongTrack == chipnomadState->project.tracksCount - 1) return 1;
    uint16_t chain = chipnomadState->project.song[*pSongRow][*pSongTrack + 1];
    if (chain != EMPTY_VALUE_16 && !chainIsEmpty(&chipnomadState->project, chain)) {
      *pSongTrack += 1;
      while (chipnomadState->project.chains[chain].rows[*pChainRow].phrase == EMPTY_VALUE_16) {
        *pChainRow -= 1;
        if (*pChainRow == 0) break;
      }
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if ((keys == (keyUp | keyOpt)) || (keys == keyUp && screen.cursorRow == 0)) {
    // Previous phrase in the chain
    if (*pChainRow == 0) return 1;
    if (chipnomadState->project.chains[chipnomadState->project.song[*pSongRow][*pSongTrack]].rows[*pChainRow - 1].phrase != EMPTY_VALUE_16) {
      *pChainRow -= 1;
      if (keys == keyUp) screen.cursorRow = 15;
      setup(-1);
      playbackQueuePhrase(&chipnomadState->playbackState, *pSongTrack, *pSongRow, *pChainRow);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyDown | keyOpt) || (keys == keyDown && screen.cursorRow == 15)) {
    // Next phrase in the chain
    if (*pChainRow == 15) return 1;
    if (chipnomadState->project.chains[chipnomadState->project.song[*pSongRow][*pSongTrack]].rows[*pChainRow + 1].phrase != EMPTY_VALUE_16) {
      *pChainRow += 1;
      if (keys == keyDown) screen.cursorRow = 0;
      setup(-1);
      playbackQueuePhrase(&chipnomadState->playbackState, *pSongTrack, *pSongRow, *pChainRow);
      fullRedraw();
    }
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (isFxEdit) {
    int fxIdx = (screen.cursorCol - 3) / 2;
    int result = fxEditInput(keys, isDoubleTap, phraseRows[screen.cursorRow].fx[fxIdx], lastFX);
    if (result) {
      isFxEdit = 0;

      // If in selection mode and on FX type column, fill selection with selected FX
      if (screen.selectMode == 1 && (screen.cursorCol == 3 || screen.cursorCol == 5 || screen.cursorCol == 7)) {
        int startCol, startRow, endCol, endRow;
        getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);

        if (isSingleColumnSelection(&screen)) {
          uint8_t selectedFX = phraseRows[screen.cursorRow].fx[fxIdx][0];
          for (int r = startRow; r <= endRow; r++) {
            phraseRows[r].fx[fxIdx][0] = selectedFX;
          }
        }
      }

      fullRedraw();
    }
    return 1;
  }

  if (screen.selectMode == 0 && inputScreenNavigation(keys, isDoubleTap)) return 1;
  return screenInput(&screen, isKeyDown, keys, isDoubleTap);
}

const AppScreen screenPhrase = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .init = init
};
