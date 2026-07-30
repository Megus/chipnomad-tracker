#ifndef IMPORT_VTS_H
#define IMPORT_VTS_H

int instrumentLoadVTS(const char* path, int instrumentIdx);
int instrumentLoadVTSFromMemory(char** lines, int lineCount, int instrumentIdx, const char* instrumentName);

#endif


