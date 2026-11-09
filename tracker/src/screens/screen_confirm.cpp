#include <string.h>
#include "screen_confirm.h"

void ScreenConfirm::setup(const ScreenData& input) {
  strncpy(message, input.title, 41);
  confirmValue = input.confirmValue;
  cancelValue = input.cancelValue;
  cursorRow = 0;
  cursorCol = 0;
}

void ScreenConfirm::drawStatic() {
  gfx.setFgColor(theme().selection);
  gfx.print(0, 8, message);
}

void ScreenConfirm::drawCell(int col, int row, CellState state) {
  gfx.setFgColor(col == cursorCol ? theme().textValue : theme().textDefault);
  if (col == 0) {
    gfx.print(0, 10, col == cursorCol ? "> OK" : "  OK");
  } else {
    gfx.print(6, 10, col == cursorCol ? "> Cancel" : "  Cancel");
  }
}

bool ScreenConfirm::onEdit(int col, int row, EditCellAction action) {
  if (action == EditCellAction::tap) {
    events.dispatch(Event::popScreen(ScreenEnum::confirm, ScreenData::confirmOutput(col == 0 ? confirmValue : cancelValue)));
    return true;
  }
  return false;
}

bool ScreenConfirm::onNavigationInput(const Event& e) {
  if (e.keys == keyOpt) {
    events.dispatch(Event::popScreen(ScreenEnum::confirm, ScreenData::confirmOutput(cancelValue)));
    return true;
  }
  return false;
}
