#include <screen_instrument.h>


static int getColumnCount(int row) {
  if (row < 3) return formInstrumentCommonColumnCount(row);
  return 1;
}

static void drawForm(void) {
  formInstrumentCommonDrawForm();

}

static void drawCursor(int col, int row) {
  if (row < 3) return formInstrumentCommonDrawCursor(col, row);
}

static void drawField(int col, int row) {
  if (row < 3) return formInstrumentCommonDrawField(col, row);
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (row < 3) return formInstrumentCommonOnEdit(col, row, action);

  return 0;
}


struct FormScreenData formInstrumentAY = {
  .rows = 7,
  .cursorRow = 0,
  .cursorCol = 0,
  .getColumnCount = getColumnCount,
  .drawForm = drawForm,
  .drawCursor = drawCursor,
  .drawField = drawField,
  .onEdit = onEdit,
};