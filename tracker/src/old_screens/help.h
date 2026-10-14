#ifndef __HELP_H__
#define __HELP_H__

#include "chipnomad_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* helpFXHint(uint8_t* fx, int isTable, uint8_t instrumentIdx);
const char* helpFXDescription(enum FX fxIdx, uint8_t instrumentIdx);
void drawFXHelp(enum FX fxIdx, uint8_t instrumentIdx);

#ifdef __cplusplus
}
#endif

#endif // __HELP_H__