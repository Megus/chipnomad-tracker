#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static int phrase = 0;

static uint8_t lastNote = 48;
static uint8_t lastInstrument = 0;
static uint8_t lastVolume = 15;
static uint8_t lastFX[2];

static void drawCell(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static int onEdit(int col, int row, enum CellEditAction action);

static struct SpreadsheetScreenData sheet = {
  .rows = 16,
  .cols = 9,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawCell = drawCell,
  .onEdit = onEdit,
};

static void setup(int input) {
  phrase = project.chains[project.song[*pSongRow][*pSongTrack]].phrases[*pChainRow];
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void fullRedraw(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "PHRASE %03X", phrase);

  spreadsheetFullRedraw(&sheet);
}

static void drawCell(int col, int row, int state) {
  if (col == 0) {
    // Note
    uint8_t value = project.phrases[phrase].notes[row];
    setCellColor(state, value == EMPTY_VALUE_8, 1);
    gfxPrint(3, 3 + row, noteName(value));
  } else if (col == 1 || col == 2) {
    // Instrument and volume
    uint8_t value = (col == 1) ? project.phrases[phrase].instruments[row] : project.phrases[phrase].volumes[row];
    setCellColor(state, value == EMPTY_VALUE_8, 1);
    gfxPrint(4 + col * 3, 3 + row, byteToHexOrEmpty(value));
  } else if (col == 3 || col == 5 || col == 7) {
    // FX name
    uint8_t fx = project.phrases[phrase].fx[row][(col - 3) / 2][0];
    setCellColor(state, fx == EMPTY_VALUE_8, 1);
    gfxPrint(4 + col * 3, 3 + row, fxName(fx));
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    uint8_t value = project.phrases[phrase].fx[row][(col - 4) / 2][1];
    setCellColor(state, 0, project.phrases[phrase].fx[row][(col - 3) / 2][0] != EMPTY_VALUE_8);
    gfxPrint(4 + col * 3, 3 + row, byteToHex(value));
  }
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : ((row & 3) == 0 ? cs.textValue : cs.textInfo));
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
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

static void draw(void) {
  gfxClearRect(0, 3, 1, 16);
  gfxSetFgColor(appSettings.colorScheme.textInfo);
  gfxPrint(0, 3 + *pChainRow, "<");

  gfxClearRect(2, 3, 1, 16);
  struct PlaybackTrackState* track = &playback.tracks[*pSongTrack];
  if (playback.mode != playbackModeStopped && playback.mode != playbackModePhraseRow && track->songRow != EMPTY_VALUE_16) {
    // Chain row
    if (*pSongRow == track->songRow) {
      gfxSetFgColor(appSettings.colorScheme.playMarkers);
      gfxPrint(0, 3 + track->chainRow, "<");
    }

    // Phrase row
    int playingPhrase = project.chains[project.song[track->songRow][*pSongTrack]].phrases[track->chainRow];
    if (playingPhrase == phrase) {
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

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;
  uint8_t maxVolume = 16; // This is for AY, will add m ore conditions in the future

  if (col == 0) {
    // Note
    if (action == editClear && project.phrases[phrase].notes[row] == EMPTY_VALUE_8) {
      // Special case: inserting OFF
      project.phrases[phrase].notes[row] = NOTE_OFF;
      handled = 1;
    } else if (action == editClear) {
      // When clearing note, we also need to clear instrument and volume
      handled = edit8withLimit(action, &project.phrases[phrase].notes[row], &lastNote, project.pitchTable.octaveSize, project.pitchTable.length);
      edit8withLimit(action, &project.phrases[phrase].instruments[row], &lastInstrument, 16, PROJECT_MAX_INSTRUMENTS);
      edit8withLimit(action, &project.phrases[phrase].volumes[row], &lastVolume, 16, maxVolume);
    } else if (action == editTap && project.phrases[phrase].notes[row] == EMPTY_VALUE_8) {
      // When inserting note, also insert instrument and volume
      project.phrases[phrase].notes[row] = lastNote;
      project.phrases[phrase].instruments[row] = lastInstrument;
      project.phrases[phrase].volumes[row] = lastVolume;
      handled = 1;
    } else if (project.phrases[phrase].notes[row] != NOTE_OFF) {
      handled = edit8withLimit(action, &project.phrases[phrase].notes[row], &lastNote, project.pitchTable.octaveSize, project.pitchTable.length);
    }

    if (handled) {
      // Also draw instrument and volume as they could change
      drawCell(1, row, 0);
      drawCell(2, row, 0);
    }
  } else if (col == 1) {
    // Instrument
    if (action == editDoubleTap) {

    } else {
      handled = edit8withLimit(action, &project.phrases[phrase].instruments[row], &lastInstrument, 16, PROJECT_MAX_INSTRUMENTS);
    }

    uint8_t instrument = project.phrases[phrase].instruments[row];
    if (handled && instrument != EMPTY_VALUE_8) {
      screenMessage("%s: %s", byteToHex(instrument), instrumentName(instrument));
    }

  } else if (col == 2) {
    // Volume
    handled = edit8withLimit(action, &project.phrases[phrase].volumes[row], &lastVolume, 16, maxVolume);
  } else if (col == 3 || col == 5 || col == 7) {
    // FX
    int fxIdx = (col - 3) / 2;
    handled = editFX(action, project.phrases[phrase].fx[row][fxIdx], lastFX);
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    int fxIdx = (col - 4) / 2;
    if (project.phrases[phrase].fx[row][fxIdx][0] != EMPTY_VALUE_8) {
      handled = edit8noLimit(action, &project.phrases[phrase].fx[row][fxIdx][1], &lastFX[1], 16);
    }
  }

  if (handled) {
    project.phrases[phrase].hasNotes = -1;
    project.chains[project.song[*pSongRow][*pSongTrack]].hasNotes = -1;
    playbackStartPhraseRow(&playback, *pSongTrack, *pSongRow, *pChainRow, sheet.cursorRow);
  }

  return handled;
}


static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    // To Instrument screen
    // TODO: Select instrument from the current row
    screenSetup(&screenInstrument, 0);
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    // To Chain screen
    screenSetup(&screenChain, -1);
    return 1;
  } else if (keys == (keyUp | keyShift)) {
    // To Groove screen
    // TODO: If we currently on the groove command, go to this groove
    screenSetup(&screenGroove, 0);
    return 1;
  } else if (keys == (keyLeft | keyOpt)) {
    // Previous track
    if (*pSongTrack == 0) return 1;
    uint16_t chain = project.song[*pSongRow][*pSongTrack - 1];
    if (chain != EMPTY_VALUE_16 && !chainIsEmpty(chain)) {
      *pSongTrack -= 1;
      while (project.chains[chain].phrases[*pChainRow] == EMPTY_VALUE_16) {
        *pChainRow -= 1;
        if (*pChainRow == 0) break;
      }
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyRight | keyOpt)) {
    // Next track
    if (*pSongTrack == project.tracksCount - 1) return 1;
    uint16_t chain = project.song[*pSongRow][*pSongTrack + 1];
    if (chain != EMPTY_VALUE_16 && !chainIsEmpty(chain)) {
      *pSongTrack += 1;
      while (project.chains[chain].phrases[*pChainRow] == EMPTY_VALUE_16) {
        *pChainRow -= 1;
        if (*pChainRow == 0) break;
      }
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if ((keys == (keyUp | keyOpt)) || (keys == keyUp && sheet.cursorRow == 0)) {
    // Previous phrase in the chain
    if (*pChainRow == 0) return 1;
    if (project.chains[project.song[*pSongRow][*pSongTrack]].phrases[*pChainRow - 1] != EMPTY_VALUE_16) {
      *pChainRow -= 1;
      if (keys == keyUp) sheet.cursorRow = 15;
      setup(-1);
      playbackQueuePhrase(&playback, *pSongTrack, *pSongRow, *pChainRow);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyDown | keyOpt) || (keys == keyDown && sheet.cursorRow == 15)) {
    // Next phrase in the chain
    if (*pChainRow == 15) return 1;
    if (project.chains[project.song[*pSongRow][*pSongTrack]].phrases[*pChainRow + 1] != EMPTY_VALUE_16) {
      *pChainRow += 1;
      if (keys == keyDown) sheet.cursorRow = 0;
      setup(-1);
      playbackQueuePhrase(&playback, *pSongTrack, *pSongRow, *pChainRow);
      fullRedraw();
    }
    return 1;
  }
  return 0;
}

static int inputPlayback(int keys, int isDoubleTap) {
  if (playback.mode == playbackModeStopped && (keys == keyPlay)) {
    playbackStartPhrase(&playback, *pSongTrack, *pSongRow, *pChainRow);
    return 1;
  } else if (playback.mode == playbackModeStopped && (keys == (keyPlay | keyShift))) {
    playbackStartSong(&playback, *pSongRow, *pChainRow);
    return 1;
  } else if (playback.mode != playbackModeStopped && (keys == keyPlay)) {
    playbackStop(&playback);
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (spreadsheetInput(&sheet, keys, isDoubleTap)) return;
  inputPlayback(keys, isDoubleTap);
}

const struct AppScreen screenPhrase = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};