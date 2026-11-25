#include "screen_phrase.h"
#include "utils.h"
#include "project_utils.h"
#include "clipboard.h"
#include "help.h"

void ScreenPhrase::setup(const ScreenData& input) {
  phraseIdx = state.project->chains[state.project->song[state.songRow][state.songTrack]].rows[state.chainRow].phrase;
  phraseRows = state.project->phrases[phraseIdx].rows;
}

void ScreenPhrase::resetState() {
  lastNote = 48; // TODO: Make C-4 based on the octave size
  lastInstrument = 0;
  lastVolume = 15; // TODO: Take it from the chip settings. One more TODO: Decide how to better deal with projects with different chip types (YM2203)
  lastFX[0] = 0;
  lastFX[1] = 0;
  cursorRow = 0;
  cursorCol = 0;
}

Rect ScreenPhrase::getSelectionRect(const Rect& range) {
  int col2 = range.x + range.w - 1;
  int x = columnX[range.x];
  int w = columnX[col2 + 1] - x - 1;
  int y = 3 + range.y;
  int h = range.h;
  if (col2 == 3 || col2 == 5 || col2 == 7) w++;
  return {.x = x, .y = y, .w = w, .h = h};
}

bool ScreenPhrase::switchSelectionRange() {
  Rect selection = getSelectionRange();

  if (isSingleCellSelection(selection)) {
    selectColumn(15);
    return true;
  }
  if (isFullColumnSelection(selection, 15)) {
    selectRow(8);
    return true;
  }
  if (isFullRowSelection(selection, 8)) {
    selectAll(8, 15);
    return true;
  }
  return false;
}

chipnomad::LoopRange ScreenPhrase::getLoopRange() {
  chipnomad::LoopRange range = {0};
  if (mode == ScreenMode::select) {
    Rect selection = getSelectionRange();
    range.enabled = true;
    range.level = 2;
    range.startSongRow = state.songRow;
    range.startChainRow = state.chainRow;
    range.startPhraseRow = selection.y;
    range.endSongRow = state.songRow;
    range.endChainRow = state.chainRow;
    range.endPhraseRow = selection.y + selection.h - 1;
  }
  return range;
}

void ScreenPhrase::draw() {
  using namespace chipnomad;

  gfx.clearRect(0, 3, 1, 16);
  gfx.setFgColor(theme().textInfo);
  gfx.print(0, 3 + state.chainRow, "<");

  gfx.clearRect(2, 3, 1, 16);
  PlaybackTrackState* track = &audio.engine.player.tracks[state.songTrack];
  if (track->mode != PlaybackMode::stopped && track->mode != PlaybackMode::phraseRow && track->songRow != EMPTY_VALUE_16) {
    // Chain row
    if (state.songRow == track->songRow) {
      gfx.setFgColor(theme().playMarkers);
      gfx.print(0, 3 + track->chainRow, "<");
    }

    // Phrase row
    int playingPhrase = state.project->chains[state.project->song[track->songRow][state.songTrack]].rows[track->chainRow].phrase;
    if (playingPhrase == phraseIdx) {
      int row = track->phraseRow;
      if (row >= 0 && row < 16) {
        gfx.setFgColor(theme().playMarkers);
        gfx.print(2, 3 + row, ">");
      }
    }
  }
}

void ScreenPhrase::drawStatic() {
  gfx.setFgColor(theme().textTitles);
  gfx.printf(0, 0, "PHRASE %03X%c", phraseIdx, isPhraseUsedElsewhere(*state.project, phraseIdx, state.project->song[state.songRow][state.songTrack], state.chainRow) ? '*' : ' ');
}

void ScreenPhrase::drawCursor(int col, int row) {
  int width = 2;
  if (col == 0 || col == 3 || col == 5 || col == 7) width = 3;
  gfx.cursor(col == 0 ? 3 : 4 + col * 3, 3 + row, width);
}

void ScreenPhrase::drawRowHeader(int row, CellState cellState) {
  gfx.setFgColor((cellState == CellState::focus) ? theme().textDefault : ((row & 3) == 0 ? theme().textValue : theme().textInfo));
  gfx.printf(1, 3 + row, "%X", row);
}

void ScreenPhrase::drawColHeader(int col, CellState cellState) {
  gfx.setFgColor((cellState == CellState::focus) ? theme().textDefault : theme().textInfo);

  switch (col) {
    case 0:
      gfx.print(3, 2, "N");
      break;
    case 1:
      gfx.print(7, 2, "I");
      break;
    case 2:
      gfx.print(10, 2, "V");
      break;
    case 3:
    case 4:
      gfx.print(13, 2, "FX1");
      break;
    case 5:
    case 6:
      gfx.print(19, 2, "FX2");
      break;
    case 7:
    case 8:
      gfx.print(25, 2, "FX3");
      break;
    default:
      break;
  }
}

void ScreenPhrase::drawCell(int col, int row, CellState cellState) {
  int x = columnX[col];
  int y = 3 + row;
  if (col == 0) {
    // Note
    uint8_t value = phraseRows[row].note;
    setCellColor(cellState, value == EMPTY_VALUE_8, 1);
    gfx.print(x, y, state.project->noteName(value));
  } else if (col == 1 || col == 2) {
    // Instrument and volume
    uint8_t value = (col == 1) ? phraseRows[row].instrument : phraseRows[row].volume;
    setCellColor(cellState, value == EMPTY_VALUE_8, 1);
    gfx.print(x, y, chipnomad::byteToHexOrEmpty(value));
  } else if (col == 3 || col == 5 || col == 7) {
    // FX name
    uint8_t fx = phraseRows[row].fx[(col - 3) / 2][0];
    setCellColor(cellState, fx == EMPTY_VALUE_8, 1);
    gfx.print(x, y, chipnomad::fxNames[fx].name);
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    uint8_t value = phraseRows[row].fx[(col - 4) / 2][1];
    setCellColor(cellState, 0, phraseRows[row].fx[(col - 3) / 2][0] != EMPTY_VALUE_8);
    gfx.print(x, y, chipnomad::byteToHex(value));
  }
}

bool ScreenPhrase::onEdit(int col, int row, EditCellAction action) {
  bool handled = false;

  Rect selection = getSelectionRange();

  if (action == EditCellAction::multiIncrease || action == EditCellAction::multiDecrease) {
    if (!isSingleColumnSelection(selection)) return false;
    handled = applyMultiEdit(selection, action, this);
  } else if (action == EditCellAction::multiIncreaseBig || action == EditCellAction::multiDecreaseBig) {
    // Check if full width selection (all columns)
    if (selection.x == 0 && selection.w == 9) {
      // Rotation mode
      int direction = (action == EditCellAction::multiIncreaseBig) ? -1 : 1;
      applyPhraseRotation(state.project->phrases[phraseIdx], selection.y, selection.y + selection.h - 1, direction);
      fullRedraw();
      handled = true;
    } else if (isSingleColumnSelection(selection)) {
      // Single column: big increase/decrease or FX selection
      if (selection.x == 3 || selection.x == 5 || selection.x == 7) {
        // FX type column: show FX selection
        int fxIdx = (selection.x - 3) / 2;
        // Get instrument index from current phrase row or traverse back
        uint8_t instrumentNum = lookupInstrument(*state.project, state.songRow, state.chainRow, cursorRow, state.songTrack);



        fxEditFullDraw(phraseRows[screen.cursorRow].fx[fxIdx][0], instrumentNum);
      } else {
        // Regular big increase/decrease for note, volume, instrument, FX value
        handled = applyMultiEdit(selection, action, this);
      }
    }
  } else if (action == EditCellAction::copy) {
    copyPhrase(phraseIdx, startCol, startRow, endCol, endRow, 0);
    handled = 1;
  } else if (action == EditCellAction::cut) {
    copyPhrase(phraseIdx, startCol, startRow, endCol, endRow, 1);
    handled = 1;
  } else if (action == EditCellAction::paste) {
    const int rowsPasted = pastePhrase(phraseIdx, col, row);
    if (rowsPasted > 0) {
      // Move cursor below pasted data, or to last row if paste extends to end
      int newRow = row + rowsPasted;
      if (newRow > 15) newRow = 15;
      screen.cursorRow = newRow;
    }
    fullRedraw();
    handled = 1;
  } else if (action == EditCellAction::shallowClone) {
    // Handle instrument column cloning
    if (startCol == 1 && endCol == 1) {
      int distinctCount = cloneInstrumentsInPhrase(phraseIdx, startRow, endRow);
      if (distinctCount == 0) {
        screenMessage(MESSAGE_TIME, "No empty instruments");
      }
      screenMessage(MESSAGE_TIME, "Cloned %d instrument%s", distinctCount, distinctCount == 1 ? "" : "s");
      handled = 1;
    }
  } else {
    handled = editCell(col, row, action);
  }

  if (handled) projectModified = 1;
  return handled;
}

bool ScreenPhrase::editCell(int col, int row, EditCellAction action) {
  using namespace chipnomad;

  bool handled = false;
  uint8_t maxVolume = 15; // TODO: Use chip max volume value

  if (col == 0) {
    // Note
    if (action == EditCellAction::clear && phraseRows[row].note == EMPTY_VALUE_8) {
      // Insert OFF
      phraseRows[row].note = NOTE_OFF;
      phraseRows[row].instrument = EMPTY_VALUE_8;
      phraseRows[row].volume = EMPTY_VALUE_8;
      handled = 1;
    } else if (action == EditCellAction::clear) {
      // Clear note
      handled = edit8withLimit(action, &phraseRows[row].note, &lastNote, state.project->pitchTable.octaveSize, state.project->pitchTable.length - 1);
      edit8withLimit(action, &phraseRows[row].instrument, &lastInstrument, 16, PROJECT_MAX_INSTRUMENTS - 1);
      edit8withLimit(action, &phraseRows[row].volume, &lastVolume, 16, maxVolume);
    } else if (action == EditCellAction::tap && phraseRows[row].note == EMPTY_VALUE_8) {
      phraseRows[row].note = lastNote;
      phraseRows[row].instrument = lastInstrument;
      phraseRows[row].volume = lastVolume;
      handled = 1;
    } else if (phraseRows[row].note != NOTE_OFF) {
      handled = edit8withLimit(action, &phraseRows[row].note, &lastNote, state.project->pitchTable.octaveSize, state.project->pitchTable.length - 1);
      if (handled) {
        lastInstrument = phraseRows[row].instrument;
        lastVolume = phraseRows[row].volume;
      }
    }
    if (handled) {
      drawCell(1, row, CellState::normal);
      drawCell(2, row, CellState::normal);
    }
  } else if (col == 1) {
    // Instrument
    if (action == EditCellAction::doubleTap) {
      uint8_t nextInstrument = findEmptyInstrument(*state.project, 0);
      if (nextInstrument != EMPTY_VALUE_8) {
        phraseRows[row].instrument = nextInstrument;
        handled = 1;
      }
    } else {
      handled = edit8withLimit(action, &phraseRows[row].instrument, &lastInstrument, 16, PROJECT_MAX_INSTRUMENTS - 1);
    }
    uint8_t instrument = phraseRows[row].instrument;
    if (handled && instrument != EMPTY_VALUE_8) {
      showInstantMessage("%s: %s", byteToHex(instrument), state.project->instrumentName(instrument));
    }
  } else if (col == 2) {
    // Volume
    handled = edit8withLimit(action, &phraseRows[row].volume, &lastVolume, 16, maxVolume);
  } else if (col == 3 || col == 5 || col == 7) {
    // FX
    int fxIdx = (col - 3) / 2;
    // Get instrument number from current phrase row or traverse back
    uint8_t instrumentNum = lookupInstrument(*state.project, state.songRow, state.chainRow, row, state.songTrack);
    int result = editFX(action, phraseRows[row].fx[fxIdx], lastFX, 0, instrumentNum);
    if (result == 2) {
      drawField(col + 1, row, CellState::normal);
      handled = 1;
    } else if (result == 1) {
      isFxEdit = 1;
      handled = 0;
    }
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    int fxIdx = (col - 4) / 2;
    if (phraseRows[row].fx[fxIdx][0] != EMPTY_VALUE_8) {
      uint8_t instrumentNum = lookupInstrument(*state.project, state.songRow, state.chainRow, row, state.songTrack);
      handled = editFXValue(action, phraseRows[row].fx[fxIdx], lastFX, 0, instrumentNum);
    }
  }

  if (handled && (!audio.engine.player.isPlaying() || audio.engine.player.tracks[state.songTrack].mode == PlaybackMode::phraseRow)) {
    PhraseRow* row = &phraseRows[screen.cursorRow];
    if (row->note != EMPTY_VALUE_8 && row->note != NOTE_OFF && row->instrument == EMPTY_VALUE_8) {
      PhraseRow previewRow = *row;
      previewRow.instrument = lookupInstrument(state.project, state.songRow, state.chainRow, screen.cursorRow, state.songTrack);
      audio.engine.player.playPhraseRow(state.songTrack, &previewRow);
    } else {
      audio.engine.player.playPhraseRow(state.songTrack, row);
    }
  }

  return handled;
}

bool ScreenPhrase::onNavigationInput(const Event& e) {
  if (keys == (keyRight | keyShift)) {
    // To Instrument/Phrase screen
    int table = -1;
    if (screen.cursorCol > 2) {
      // If we currently on the table command, go to this table
      int fxIdx = (screen.cursorCol - 3) / 2;
      uint8_t fxType = phraseRows[screen.cursorRow].fx[fxIdx][0];
      uint8_t fxValue = phraseRows[screen.cursorRow].fx[fxIdx][1];
      if ((fxType == fxTBL || fxType == fxTBX) && fxValue != 0xff) {
        table = fxValue;
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
    if (state.songTrack == 0) return 1;
    uint16_t chain = state.project->song[state.songRow][state.songTrack - 1];
    if (chain != EMPTY_VALUE_16 && !chainIsEmpty(state.project, chain)) {
      state.songTrack -= 1;
      while (state.project->chains[chain].rows[state.chainRow].phrase == EMPTY_VALUE_16) {
        state.chainRow -= 1;
        if (state.chainRow == 0) break;
      }
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyRight | keyOpt)) {
    // Next track
    if (state.songTrack == state.project->tracksCount - 1) return 1;
    uint16_t chain = state.project->song[state.songRow][state.songTrack + 1];
    if (chain != EMPTY_VALUE_16 && !chainIsEmpty(state.project, chain)) {
      state.songTrack += 1;
      while (state.project->chains[chain].rows[state.chainRow].phrase == EMPTY_VALUE_16) {
        state.chainRow -= 1;
        if (state.chainRow == 0) break;
      }
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if ((keys == (keyUp | keyOpt)) || (keys == keyUp && screen.cursorRow == 0)) {
    // Previous phrase in the chain
    if (state.chainRow == 0) return 1;
    if (state.project->chains[state.project->song[state.songRow][state.songTrack]].rows[state.chainRow - 1].phrase != EMPTY_VALUE_16) {
      state.chainRow -= 1;
      if (keys == keyUp) screen.cursorRow = 15;
      setup(-1);
      audio.engine.player.queuePhrase(state.songTrack, state.songRow, state.chainRow);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyDown | keyOpt) || (keys == keyDown && screen.cursorRow == 15)) {
    // Next phrase in the chain
    if (state.chainRow == 15) return 1;
    if (state.project->chains[state.project->song[state.songRow][state.songTrack]].rows[state.chainRow + 1].phrase != EMPTY_VALUE_16) {
      state.chainRow += 1;
      if (keys == keyDown) screen.cursorRow = 0;
      setup(-1);
      audio.engine.player.queuePhrase(state.songTrack, state.songRow, state.chainRow);
      fullRedraw();
    }
    return 1;
  }
  return 0;
}

bool ScreenPhrase::onEvent(const Event& e) {
  bool handled = commonEventHandler(e);
  if (handled) return true;

  if (e.type == EventType::screenOutput && e.screen == ScreenEnum::selectFX) {
    int selectedFX = e.screenData.intValue;
    int fxIdx = (cursorCol - 3) / 2;
    // If in selection mode and on FX type column, fill selection with selected FX
    if (mode == ScreenMode::select && (cursorCol == 3 || cursorCol == 5 || cursorCol == 7)) {
      if (isSingleColumnSelection()) {
        Rect range = getSelectionRange();
        uint8_t selectedFX = phraseRows[screen.cursorRow].fx[fxIdx][0];
        for (int r = startRow; r <= endRow; r++) {
          phraseRows[r].fx[fxIdx][0] = selectedFX;
        }
      }
    }
  }
  return handled;
}



static int onInput(int isKeyDown, int keys, int tapCount) {
  if (isFxEdit) {
    int fxIdx = (screen.cursorCol - 3) / 2;
    int result = fxEditInput(keys, tapCount, phraseRows[screen.cursorRow].fx[fxIdx], lastFX);
    if (result) {
      isFxEdit = 0;


      fullRedraw();
    }
    return 1;
  }

  if (screen.selectMode == 0 && inputScreenNavigation(keys, tapCount)) return 1;
  return screenInput(&screen, isKeyDown, keys, tapCount);
}
