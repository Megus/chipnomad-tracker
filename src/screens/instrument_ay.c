#include <screen_instrument.h>


static int getColumnCount(int row) {
  if (row < 3) return instrumentCommonColumnCount(row);
  return 1;
}

static void drawStatic(void) {
  instrumentCommonDrawStatic();
}

static void drawCursor(int col, int row) {
  if (row < 3) return instrumentCommonDrawCursor(col, row);
}

static void drawField(int col, int row, int state) {
  if (row < 3) return instrumentCommonDrawField(col, row, state);
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return instrumentCommonOnEdit(col, row, action);

  return 0;
}

struct ScreenData formInstrumentAY = {
  .rows = 7,
  .cursorRow = 0,
  .cursorCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
};