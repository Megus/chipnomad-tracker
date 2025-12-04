#ifndef IMPORT_VTS_H
#define IMPORT_VTS_H

int instrumentLoadVTS(const char* path, int instrumentIdx);

// Import VTS instrument from memory buffer (for embedded VTS data in VT2 files)
// Returns 0 on success, non-zero on error
// lines: array of VTS format lines (without the [SampleN] header)
// lineCount: number of lines in the array
// instrumentName: name for the instrument (can be NULL for default)
int instrumentLoadVTSFromMemory(char** lines, int lineCount, int instrumentIdx, const char* instrumentName);

#endif


