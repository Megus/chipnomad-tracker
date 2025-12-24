#include "import_common.h"
#include <ctype.h>
#include <string.h>

int parseHexChar(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

int parseBase26Char(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  return 0;
}

int parseHexString(const char* str, int maxDigits) {
  int value = 0;
  for (int i = 0; i < maxDigits && str[i] != '\0'; i++) {
    if (!isxdigit(str[i])) break;
    value = value * 16 + parseHexChar(str[i]);
  }
  return value;
}

void initTableRow(TableRow* row) {
  row->pitchFlag = 0;
  row->pitchOffset = 0;
  row->volume = EMPTY_VALUE_8;
  for (int i = 0; i < MAX_TABLE_FX_SLOTS; i++) {
    row->fx[i][0] = EMPTY_VALUE_8;
    row->fx[i][1] = 0;
  }
}

void initEmptyPhraseRow(PhraseRow* row) {
  row->note = EMPTY_VALUE_8;
  row->instrument = EMPTY_VALUE_8;
  row->volume = EMPTY_VALUE_8;
  for (int i = 0; i < MAX_FX_SLOTS; i++) {
    row->fx[i][0] = EMPTY_VALUE_8;
    row->fx[i][1] = 0;
  }
}

void initEmptyTable(Table* table) {
  for (int row = 0; row < 16; row++) {
    initTableRow(&table->rows[row]);
  }
}
