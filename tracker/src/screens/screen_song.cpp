#include <string.h>
#include "screen_song.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "audio_manager.h"

#include "clipboard.h"

void ScreenSong::setup(int input) {
  mode = ScreenMode::edit;
}

Rect ScreenSong::getSelectionRect(Rect range) {
  int x = 3 + range.x * 3;
  int y = 3 + range.y - topRow;
  int w = 3 * range.w - 1;
  int y2 = y + range.h - 1;

  if (y < 3) y = 3; // Top row of selection is above the screen
  if (y > (3 + 15)) return {.x = -1, .y = 0, .w = 0, .h = 0}; // Top row of selection is below the screen
  if (y2 < 3) return {.x = -1, .y = 0, .w = 0, .h = 0}; // Bottom row is above the screen
  if (y2 > (3 + 15)) y2 = (3 + 15); // Bottom row is below the screen

  return {.x = x, .y = y, .w = w, .h = y2 - y + 1};
}

chipnomad::LoopRange ScreenSong::getLoopRange() {
  chipnomad::LoopRange range = {0};
  if (mode == ScreenMode::select) {
    Rect selection = getSelectionRange();
    range.enabled = true;
    range.level = 0;
    range.startSongRow = selection.y;
    range.startChainRow = 0;
    range.startPhraseRow = 0;
    range.endSongRow = selection.y + selection.h - 1;
    range.endChainRow = 15;
    range.endPhraseRow = 15;
  }
  return range;
}

void ScreenSong::draw() {
  for (int c = 0; c < state.project.tracksCount; c++) {
    gfx.clearRect(2 + c * 3, 3, 1, 16);
    if (audio.engine.player.tracks[c].songRow != EMPTY_VALUE_16) {
      int row = audio.engine.player.tracks[c].songRow - topRow;
      if (row >= 0 && row < 16) {
        gfx.setFgColor(theme().playMarkers);
        gfx.print(2 + c * 3, 3 + row, ">");
      }
    }

    // Draw mute/solo indicator above track number
    gfx.setFgColor(theme().textTitles);
    if (audio.trackStates[c] == TrackState::muted) {
      gfx.print(3 + c * 3, 1, "M");
    } else if (audio.trackStates[c] == TrackState::solo) {
      gfx.print(3 + c * 3, 1, "S");
    } else {
      gfx.print(3 + c * 3, 1, " "); // Clear indicator
    }
  }

  drawSelectionBorder(getSelectionRange(), true);
}

void ScreenSong::drawStatic(void) {
  gfx.setFgColor(theme().textTitles);
  gfx.print(0, 0, "SONG");
}

void ScreenSong::drawCursor(int col, int row) {
  gfx.cursor(3 + col * 3, 3 + row - topRow, 2);
}

void ScreenSong::drawRowHeader(int row, CellState state) {
  gfx.setFgColor((state == CellState::focus) ? theme().textDefault : theme().textInfo);
  gfx.print(0, 3 + row - topRow, chipnomad::byteToHex(row));
}

void ScreenSong::drawColHeader(int col, CellState state) {
  static char digit[2] = "0";

  gfx.setFgColor((state == CellState::focus) ? theme().textDefault : theme().textInfo);
  digit[0] = col + 49;
  gfx.print(3 + col * 3, 2, digit);
}

void ScreenSong::drawCell(int col, int row, CellState cellState) {
  if (!isRowVisible(row)) return;

  int chain = state.project.song[row][col];
  int isHighlighted = state.project.songHighlight[row][col];

  if (isHighlighted) {
    gfx.setFgColor(theme().textTitles);
  } else {
    setCellColor(cellState, chain == EMPTY_VALUE_16, chain != EMPTY_VALUE_16 && chainHasNotes(state.project, chain));
  }
  gfx.print(3 + col * 3, 3 + row - topRow, chain == EMPTY_VALUE_16 ? "--" : chipnomad::byteToHex(chain));
}

bool ScreenSong::isRowVisible(int row) {
  return row >= topRow && row < (topRow + 16);
}

bool ScreenSong::adjustVerticalScroll(bool pageJump) {
  bool needsRedraw = false;
  // TODO: Handle pageJump - change topRow by 16

  if (cursorRow < topRow) {
    topRow = cursorRow;
    needsRedraw = true;
  } else if (cursorRow >= topRow + 16) {
    topRow = cursorRow - 15;
    needsRedraw = true;
  }

  return needsRedraw;
}



bool ScreenSong::onEdit(int col, int row, EditCellAction action) {
  bool handled = 0;
  Rect selection = getSelectionRange();

  if (action == EditCellAction::switchSelection) {
    return switchSelectionMode();
  } else if (action == EditCellAction::multiIncreaseBig || action == EditCellAction::multiDecreaseBig) {
    if (mode != ScreenMode::select) return false;

    if (action == EditCellAction::multiDecreaseBig) {
      handled = applySongMoveDown(state.project, selection);
      if (handled) {
        selectStartRow++;
        cursorRow++;
        // Scroll down if selection moved below visible area
        if (cursorRow >= topRow + 16) {
          topRow++;
        }
      }
    } else {
      handled = applySongMoveUp(state.project, selection);
      if (handled) {
        selectStartRow--;
        cursorRow--;
        // Scroll up if selection moved above visible area
        if (cursorRow < topRow) {
          topRow--;
        }
      }
    }

    if (handled) {
      fullRedraw();
    }
  } else if (action == EditCellAction::shallowClone || action == EditCellAction::deepClone) {
    int clonedCount = 0;
    for (int r = selection.y; r < selection.y + selection.h; r++) {
      for (int c = selection.x; c < selection.x + selection.w; c++) {
        if (editCell(c, r, action)) clonedCount++;
      }
    }
    if (clonedCount > 0) {
      const char* msg = (action == EditCellAction::shallowClone) ? "Shallow-cloned" : "Deep-cloned";
      handled = true;
      showMessage(true, "%s %d chain%s", msg, clonedCount, clonedCount == 1 ? "" : "s");
    } else {
      showMessage(true, "No chains to clone");
    }
  } else if (applyMultiEdit(selection, action, this)) {
    handled = true;
  } else if (action == EditCellAction::copy) {
    clipboard.copySong(state.project, selection, false);
    handled = true;
  } else if (action == EditCellAction::cut) {
    clipboard.copySong(state.project, selection, true);
    handled = true;
  } else if (action == EditCellAction::paste) {
    const int rowsPasted = clipboard.pasteSong(state.project, col, row);
    if (rowsPasted > 0) {
      // Move cursor below pasted data, or to last row if paste extends to end
      int newRow = row + rowsPasted;
      if (newRow >= getRowCount()) newRow = getRowCount() - 1;
      cursorRow = newRow;
    }
    fullRedraw();
    handled = true;
  } else {
    handled = editCell(col, row, action);
  }

  if (handled) state.projectModified = true;
  return handled;
}

bool ScreenSong::onNavigationInput(InputEventData e) {
  // Fix all bare returns in this function
  // Go to Chain screen
  if (e.keys == (keyRight | keyShift)) {
    int chain = state.project.song[cursorRow][cursorCol];

    if (chain == EMPTY_VALUE_16) {
      showMessage(false, "Enter a chain");
    } else {
      events.dispatch({.type = EventType::switchScreen, .screen = {.screen = ScreenEnum::chain, .input = -1}});
    }
    return true;
  }

  // Go to Project screen
  if (e.keys == (keyUp | keyShift)) {
    events.dispatch({.type = EventType::switchScreen, .screen = {.screen = ScreenEnum::project, .input = 0}});
    return true;
  }

  // Go to Settings screen
  if (e.keys == (keyDown | keyShift)) {
    events.dispatch({.type = EventType::switchScreen, .screen = {.screen = ScreenEnum::settings, .input = 0}});
    return true;
  }

  return false;
}

bool ScreenSong::editCell(int col, int row, EditCellAction action) {
  if (action == EditCellAction::doubleTap) {
    int current = state.project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int nextEmpty = findEmptyChain(state.project, current + 1);
      if (nextEmpty != EMPTY_VALUE_16) {
        state.project.song[row][col] = nextEmpty;
        lastChainValue = nextEmpty;
      } else {
        showMessage(true, "No free chains");
      }
    }
    return 1;
  } else if (action == EditCellAction::clear) {
    if (state.project.song[row][col] != EMPTY_VALUE_16) {
      // Clear the value
      state.project.song[row][col] = EMPTY_VALUE_16;
      return 1;
    } else {
      // Value is already empty, shift column up
      shiftSongColumnUp(state.project, col, row);
      fullRedraw();
      return 1;
    }
  } else if (action == EditCellAction::shallowClone) {
    int current = state.project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int cloned = clipboard.cloneChainToNext(state.project, current);
      if (cloned != EMPTY_VALUE_16) {
        state.project.song[row][col] = cloned;
        lastChainValue = cloned;
        return 1;
      }
    }
    return 0;
  } else if (action == EditCellAction::deepClone) {
    int current = state.project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      // Deep clone the phrases in the existing chain (don't create a new chain)
      if (clipboard.deepCloneChain(state.project, current)) {
        return 1;
      }
    }
    return 0;
  }
  return edit16withLimit(action, &state.project.song[row][col], &lastChainValue, 16, PROJECT_MAX_CHAINS - 1);
}

bool ScreenSong::onEvent(Event event) {
  bool handled = false;

  if (event.type != EventType::input) return false; // This screen only responds to input events
  InputEventData e = event.input;

  // Only handle mute/solo when not in selection mode
  if (mode == ScreenMode::edit) {
    switch (muteSoloState) {
      case MuteSoloState::empty:
        if (e.isKeyDown && e.keys == keyOpt) {
          if (e.tapCount == 3) {
            // Toggle highlight on triple-tap Opt
            state.project.songHighlight[cursorRow][cursorCol] ^= 1;
            state.projectModified = 1;
            drawCell(cursorCol, cursorRow, CellState::focus);
            drawCursor(cursorCol, cursorRow);
          } else {
            muteSoloState = MuteSoloState::optPressed;
          }
            handled = true;
        }
        break;

      case MuteSoloState::optPressed:
        if (e.isKeyDown && e.keys == (keyOpt | keyShift)) {
          audio.toggleTrackMute(cursorCol);
          muteSoloState = MuteSoloState::mute;
          handled = true;
        } else if (e.isKeyDown && e.keys == (keyOpt | keyPlay)) {
          audio.toggleTrackSolo(cursorCol);
          muteSoloState = MuteSoloState::solo;
          handled = true;
        } else if (e.isKeyDown && e.keys == (keyOpt | keyLeft)) {
          // Solo tracks to the left (including cursor)
          for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
            audio.trackStates[i] = (i <= cursorCol) ? TrackState::solo : TrackState::normal;
          }
          audio.toggleTrackSolo(-1);
          muteSoloState = MuteSoloState::tempLeft;
          handled = true;
        } else if (e.isKeyDown && e.keys == (keyOpt | keyRight)) {
          // Solo tracks to the right (including cursor)
          for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
            audio.trackStates[i] = (i >= cursorCol) ? TrackState::solo : TrackState::normal;
          }
          audio.toggleTrackSolo(-1);
          muteSoloState = MuteSoloState::tempRight;
          handled = true;
        } else if (e.isKeyDown && e.keys == keyOpt) {
          handled = 1; // Stay in OPT_PRESSED state
        } else if (!e.isKeyDown && e.keys == 0) {
          muteSoloState = MuteSoloState::empty;
        }
        break;

      case MuteSoloState::mute:
        if (!e.isKeyDown && e.keys == keyOpt) {
          // SHIFT released first - momentary unmute
          audio.toggleTrackMute(cursorCol);
          muteSoloState = MuteSoloState::optPressed;
          handled = 1;
        } else if (!e.isKeyDown && e.keys == keyShift) {
          // OPT released first - mute sticks
          muteSoloState = MuteSoloState::empty;
          handled = 1;
        } else if (e.keys == (keyOpt | keyShift)) {
          handled = 1; // Stay in mute state
        } else if (e.keys == 0) {
          muteSoloState = MuteSoloState::empty;
        }
        break;

      case MuteSoloState::solo:
        if (!e.isKeyDown && e.keys == keyOpt) {
          // PLAY released first - momentary unsolo
          audio.toggleTrackSolo(cursorCol);
          muteSoloState = MuteSoloState::optPressed;
          handled = 1;
        } else if (!e.isKeyDown && e.keys == keyPlay) {
          // OPT released first - solo sticks
          muteSoloState = MuteSoloState::empty;
          handled = 1;
        } else if (e.keys == (keyOpt | keyPlay)) {
          handled = 1; // Stay in solo state
        } else if (e.keys == 0) {
          muteSoloState = MuteSoloState::empty;
        }
        break;

      case MuteSoloState::tempLeft:
      case MuteSoloState::tempRight:
        if (!e.isKeyDown && (e.keys == keyOpt || e.keys == 0)) {
          // Reset all tracks to normal
          for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
            audio.trackStates[i] = TrackState::normal;
          }
          audio.toggleTrackSolo(-1);
          muteSoloState = (e.keys == keyOpt) ? MuteSoloState::optPressed : MuteSoloState::empty;
          handled = 1;
        } else if (e.keys == (keyOpt | keyLeft) || e.keys == (keyOpt | keyRight)) {
          handled = 1;
        }
        break;
    }
  }

  // Reset state when all keys released or entering selection mode
  if (e.keys == 0 || mode == ScreenMode::select) {
    muteSoloState = MuteSoloState::empty;
  }

  // Only call common input handling if we didn't handle Song-specific input
  if (!handled) {
    if (e.isKeyDown && mode == ScreenMode::edit && onNavigationInput(e)) return 1;
    return commonInputHandler(e);
  }

  return handled;
}


bool ScreenSong::switchSelectionMode() {
  if (isSingleCellSelection(getSelectionRange())) {
    selectRow(getColumnCount(0) - 1);
    return false;
  }
  return true;
}

