#ifndef IMPORT_VTS_H
#define IMPORT_VTS_H

#include "chipnomad_lib.h"

bool instrumentLoadVTS(chipnomad::Project& p, const char* path, int instrumentIdx);
bool instrumentLoadVTSFromMemory(chipnomad::Project& p, char** lines, int lineCount, int instrumentIdx, const char* instrumentName);

#endif


