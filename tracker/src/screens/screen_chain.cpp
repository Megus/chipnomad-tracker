#include "screen_chain.h"
#include "project_utils.h"
#include "utils.h"

void ScreenChain::setup(const ScreenData& input) {
  cursorRow = state.chainRow;
  chain = state.project->song[state.songRow][state.songTrack];
}

void ScreenChain::resetState() {
  Screen::resetState();
  lastPhraseValue = 0;
  lastTransposeValue = 0;
}

Rect ScreenChain::getSelectionRect(const Rect& range) {
  int x = (range.x == 0) ? 3 : 7;
  int w = (range.w == 2) ? 6 : (range.x == 0 ? 3 : 2);
  int y = 3 + range.y;
  int h = range.h;
  return {.x = x, .y = y, .w = w, .h = h};
}

bool ScreenChain::switchSelectionRange() {
  Rect selection = getSelectionRange();
  if (isSingleCellSelection(selection)) {
    selectColumn(15);
    return true;
  }
  if (isFullColumnSelection(selection, 15)) {
    selectRow(1);
    return 0;
  }
  if (isFullRowSelection(selection, 1)) {
    selectAll(1, 15);
    return 0;
  }
  return false;
}

chipnomad::LoopRange ScreenChain::getLoopRange() {
  chipnomad::LoopRange range = {0};
  if (mode == ScreenMode::select) {
    Rect selection = getSelectionRange();
    range.enabled = true;
    range.level = 1;
    range.startSongRow = state.songRow;
    range.startChainRow = selection.y;
    range.startPhraseRow = 0;
    range.endSongRow = state.songRow;
    range.endChainRow = selection.y + selection.h - 1;
    range.endPhraseRow = 15;
  }
  return range;
}

void ScreenChain::draw() {
  gfx.clearRect(2, 3, 1, 16);
  if (audio.engine.player.tracks[state.songTrack].songRow == state.songRow) {
    int chainRow = audio.engine.player.tracks[state.songTrack].chainRow;
    if (chainRow >= 0 && chainRow < 16) {
      gfx.setFgColor(theme().playMarkers);
      gfx.print(2, 3 + chainRow, ">");
    }
  }
}

void ScreenChain::drawStatic() {
  gfx.setFgColor(theme().textTitles);
  gfx.printf(0, 0, "CHAIN %02X%c", chain, isChainUsedElsewhere(*state.project, chain, state.songTrack, state.songRow) ? '*' : ' ');
}

void ScreenChain::drawCursor(int col, int row) {
  if (col == 0) {
    gfx.cursor(3, 3 + row, 3); // Phrase
  } else {
    gfx.cursor(7, 3 + row, 2); // Transpose
  }
}

void ScreenChain::drawRowHeader(int row, CellState state) {
  gfx.setFgColor((state == CellState::focus) ? theme().textDefault : theme().textInfo);
  gfx.printf(1, 3 + row, "%X", row);
}

void ScreenChain::drawColHeader(int col, CellState state) {
  gfx.setFgColor((state == CellState::focus) ? theme().textDefault : theme().textInfo);

  if (col == 0) {
    gfx.print(3, 2, "P"); // Phrase
  } else {
    gfx.print(7, 2, "T"); // Transpose
  }
}

void ScreenChain::drawCell(int col, int row, CellState cellState) {
  uint16_t phrase = state.project->chains[chain].rows[row].phrase;
  int hasContent = phrase != EMPTY_VALUE_16 && state.project->phraseHasNotes(phrase);

  if (col == 0) {
    // Phrase
    setCellColor(cellState, phrase == EMPTY_VALUE_16, hasContent);
    if (phrase == EMPTY_VALUE_16) {
      gfx.print(3, 3 + row, "---");
    } else {
      gfx.printf(3, 3 + row, "%03X", phrase);
    }
    // Also draw transpose to keep colors synchronized (only if not in selection mode)
    if (mode == ScreenMode::edit) {
      setCellColor(CellState::normal, 0, hasContent);
      gfx.print(7, 3 + row, chipnomad::byteToHex(state.project->chains[chain].rows[row].transpose));
    }
  } else {
    // Transpose
    setCellColor(cellState, 0, hasContent);
    gfx.print(7, 3 + row, chipnomad::byteToHex(state.project->chains[chain].rows[row].transpose));
  }
}

bool ScreenChain::onEdit(int col, int row, EditCellAction action) {
  bool handled = false;
  Rect selection = getSelectionRange();

  if (isMultiAction(action)) {
    if (!isSingleColumnSelection(selection)) return false;
    handled = applyMultiEdit(selection, action, this);
  } else if (action == EditCellAction::shallowClone || action == EditCellAction::deepClone) {
    int clonedCount = 0;
    for (int r = selection.y; r < selection.y + selection.h; r++) {
      for (int c = selection.x; c < selection.x + selection.w; c++) {
        if (editCell(c, r, action)) clonedCount++;
      }
    }
    if (clonedCount > 0) {
      showTimedMessage("Cloned %d phrase%s", clonedCount, clonedCount == 1 ? "" : "s");
      handled = true;
    } else {
      showTimedMessage("No phrases to clone");
    }
  } else if (action == EditCellAction::copy) {
    clipboard.copyChain(*state.project, chain, selection, false);
    handled = true;
  } else if (action == EditCellAction::cut) {
    clipboard.copyChain(*state.project, chain, selection, true);
    handled = true;
  } else if (action == EditCellAction::paste) {
    const int rowsPasted = clipboard.pasteChain(*state.project, chain, col, row);
    if (rowsPasted > 0) {
      // Move cursor below pasted data, or to last row if paste extends to end
      int newRow = row + rowsPasted;
      if (newRow > 15) newRow = 15;
      cursorRow = newRow;
    }
    events.dispatch(Event::redrawScreen());
    handled = true;
  } else {
    handled = editCell(col, row, action);
  }

  if (handled) state.projectModified = true;
  return handled;
}

bool ScreenChain::editCell(int col, int row, EditCellAction action) {
  if (col == 0) {
    // Phrase
    if (action == EditCellAction::doubleTap) {
      uint16_t current = state.project->chains[chain].rows[row].phrase;
      if (current != EMPTY_VALUE_16) {
        int nextEmpty = findEmptyPhrase(*state.project, current + 1);
        if (nextEmpty != EMPTY_VALUE_16) {
          state.project->chains[chain].rows[row].phrase = nextEmpty;
          lastPhraseValue = nextEmpty;
        } else {
          showTimedMessage("No empty phrases");
        }
      }
      return 1;
    } else if (action == EditCellAction::shallowClone || action == EditCellAction::deepClone) {
      uint16_t current = state.project->chains[chain].rows[row].phrase;
      if (current != EMPTY_VALUE_16) {
        int cloned = clipboard.clonePhraseToNext(*state.project, current);
        if (cloned != EMPTY_VALUE_16) {
          state.project->chains[chain].rows[row].phrase = cloned;
          lastPhraseValue = cloned;
          return 1;
        }
      }
      return 0;
    }
    return edit16withLimit(action, &state.project->chains[chain].rows[row].phrase, &lastPhraseValue, 16, PROJECT_MAX_PHRASES - 1);
  }
  // Transpose
  return edit8noLimit(action, &state.project->chains[chain].rows[row].transpose, &lastTransposeValue, state.project->pitchTable.octaveSize);
}

bool ScreenChain::onNavigationInput(const Event& e) {
  if (e.keys == (keyRight | keyShift)) {
    // To Phrase screen
    int phrase = state.project->chains[chain].rows[cursorRow].phrase;
    if (phrase == EMPTY_VALUE_16) {
      showInstantMessage("Enter a phrase");
    } else {
      events.dispatch(Event::switchScreen(ScreenEnum::phrase));
    }
    return true;
  } else if (e.keys == (keyLeft | keyShift)) {
    // To Song screen
    events.dispatch(Event::switchScreen(ScreenEnum::song));
    return true;
  } else if (e.keys == (keyLeft | keyOpt)) {
    // Previous track
    if (state.songTrack == 0) return true;
    if (state.project->song[state.songRow][state.songTrack - 1] != EMPTY_VALUE_16) {
      state.songTrack -= 1;
      setup(ScreenData());
      events.dispatch(Event::redrawScreen());
    }
    return true;
  } else if (e.keys == (keyRight | keyOpt)) {
    // Next track
    if (state.songTrack == state.project->tracksCount - 1) return 1;
    if (state.project->song[state.songRow][state.songTrack + 1] != EMPTY_VALUE_16) {
      state.songTrack += 1;
      setup(ScreenData());
      events.dispatch(Event::redrawScreen());
    }
    return true;
  } else if (e.keys == (keyUp | keyOpt)) {
    // Previous song row
    if (state.songRow == 0) return 1;
    if (state.project->song[state.songRow - 1][state.songTrack] != EMPTY_VALUE_16) {
      state.songRow -= 1;
      setup(ScreenData());
      events.dispatch(Event::redrawScreen());
    }
    return true;
  } else if (e.keys == (keyDown | keyOpt)) {
    // Next song row
    if (state.songRow == PROJECT_MAX_LENGTH - 1) return 1;
    if (state.project->song[state.songRow + 1][state.songTrack] != EMPTY_VALUE_16) {
      state.songRow += 1;
      setup(ScreenData());
      events.dispatch(Event::redrawScreen());
    }
  }
  return false;
}
