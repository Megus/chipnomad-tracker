#ifndef __PROJECT_IO_COMMON_H__
#define __PROJECT_IO_COMMON_H__

#include "project.h"
#include <stdio.h>
#include <stdint.h>

// Shared state for I/O operations
extern char projectFileError[41];
extern int projectFileVersion;

// Peek/consume pattern for line reading
char* peekLine(FILE* file); // Look at next non-empty line without consuming
void consumeLine(FILE* file); // Mark current line as consumed, advance to next
// Reset peek/consume state (useful for tests)
void resetPeekConsume(void);

// Binary data encoding/decoding (6-bit text encoding)
int saveBinaryData(FILE* file, const uint8_t* data, uint16_t dataLen);
int loadBinaryData(FILE* file, uint8_t** outData, uint16_t* outLen, uint16_t maxLen);

// Internal I/O functions (used between project_io.c and project_instruments_io.c)
int instrumentSaveData(FILE* file, int idx, Instrument* instrument);
int instrumentLoadData(FILE* file, Instrument* instrument, Project* p);
int saveTable(FILE* file, int idx, Table* table);

#endif
