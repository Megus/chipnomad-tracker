#ifndef __HELP_H__
#define __HELP_H__

#include <project.h>

char* helpFXHint(uint8_t* fx, int isTable);
char* helpFXDescription(enum FX fxIdx);
void drawFXHelp(enum FX fxIdx);

#endif