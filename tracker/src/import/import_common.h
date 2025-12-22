#ifndef IMPORT_COMMON_H
#define IMPORT_COMMON_H

#include <chipnomad_lib.h>
#include <stdint.h>

// Common parsing utilities
int parseHexChar(char c);
int parseBase26Char(char c);
int parseHexString(const char* str, int maxDigits);

// Common initialization utilities
void initTableRow(TableRow* row);
void initEmptyPhraseRow(PhraseRow* row);
void initEmptyTable(Table* table);

// Common constants
#define DEFAULT_SPEED 3
#define DEFAULT_VOLUME 15
#define MAX_FX_SLOTS 3
#define MAX_TABLE_FX_SLOTS 4
#define OCTAVE_SIZE 12
#define NOTES_PER_OCTAVE 12
#define PT3_TABLE_NOTES 96
#define PT3_TABLE_OCTAVES 8

#endif // IMPORT_COMMON_H
