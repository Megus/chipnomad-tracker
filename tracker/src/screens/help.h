#ifndef __HELP_H__
#define __HELP_H__

#include "chipnomad_lib.h"

char* helpFXHint(uint8_t* fx, int isTable, uint8_t instrumentIdx);
char* helpFXDescription(enum FX fxIdx, uint8_t instrumentIdx);
void drawFXHelp(enum FX fxIdx, uint8_t instrumentIdx);

#endif