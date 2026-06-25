#pragma once

#include "chipnomad_lib.h"

const char* helpFXHint(uint8_t* fx, int isTable, uint8_t instrumentIdx);
const char* helpFXDescription(enum FX fxIdx, uint8_t instrumentIdx);
void drawFXHelp(enum FX fxIdx, uint8_t instrumentIdx);
