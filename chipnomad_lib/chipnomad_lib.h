#ifndef __CHIPNOMAD_LIB_H__
#define __CHIPNOMAD_LIB_H__

// Main ChipNomad library header
// Include this file to access all ChipNomad library functionality

#include "project.h"
#include "playback.h"
#include "chips/chips.h"
#include "utils.h"

/**
 * Initialize the ChipNomad library
 * Call this once before using any other library functions
 */
void chipnomadInit(void);

/**
 * Initialize chips with current project settings
 * Can be called multiple times to reinitialize with new settings
 */
void chipnomadInitChips(int sampleRate);

/**
 * Get chip by index
 */
struct SoundChip* chipnomadGetChip(int index);

#endif