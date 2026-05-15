#ifndef __PROJECT_IO_COMMON_H__
#define __PROJECT_IO_COMMON_H__

#include "project.h"
#include "corelib/corelib_file.h"

// Shared state for I/O operations
extern char projectFileError[41];
extern int projectFileVersion;  // 1 = legacy (1.0), 2 = current (2.0)

// Peek/consume pattern for line reading
char* peekLine(int fileId);    // Look at next non-empty line without consuming
void consumeLine(int fileId);  // Mark current line as consumed, advance to next

// Reset peek/consume state (useful for tests)
void resetPeekConsume(void);

#endif
