#ifndef __PROJECT_UTILS_H__
#define __PROJECT_UTILS_H__

#include "chipnomad_lib.h"

void projectInitAY(chipnomad::Project* p);
// Does chain have notes?
int8_t chainHasNotes(chipnomad::Project* p, int chain);
// Does phrase have notes?
int8_t phraseHasNotes(chipnomad::Project* p, int phrase);
// Instrument name
const char* instrumentName(chipnomad::Project* p, uint8_t instrument);
// Instrument type name
const char* instrumentTypeName(chipnomad::InstrumentType type);
// Get first note used with an instrument
uint8_t instrumentFirstNote(chipnomad::Project* p, uint8_t instrument);
// Swap two instruments and their default tables
void instrumentSwap(chipnomad::Project* p, uint8_t inst1, uint8_t inst2);
// Find empty slots
int findEmptyChain(chipnomad::Project* p, int start);
int findEmptyPhrase(chipnomad::Project* p, int start);
int findEmptyInstrument(chipnomad::Project* p, int start);

// Cleanup functions
int cleanupUnusedPhrases(chipnomad::Project* p);
int cleanupUnusedChains(chipnomad::Project* p);
int cleanupUnusedInstruments(chipnomad::Project* p);
int cleanupUnusedTables(chipnomad::Project* p);
int removeDuplicateChains(chipnomad::Project* p);
int removeDuplicatePhrases(chipnomad::Project* p);

// Combined cleanup functions for UI
void cleanupPhrasesAndChains(chipnomad::Project* p, int* phrasesFreed, int* chainsFreed);
void cleanupInstrumentsAndTables(chipnomad::Project* p, int* instrumentsFreed, int* tablesFreed);

// Usage checking functions
int isChainUsedElsewhere(chipnomad::Project* p, int chainIdx, int excludeTrack, int excludeRow);
int isPhraseUsedElsewhere(chipnomad::Project* p, int phraseIdx, int excludeChain, int excludeRow);

// Lookup functions
// Look up instrument value at the given position or by searching upward through the track
// Parameters: project, song row, chain row, phrase row, track index
// Returns: instrument number (0-255) or EMPTY_VALUE_8 if not found
uint8_t lookupInstrument(chipnomad::Project* p, int songRow, int chainRow, int phraseRow, int track);

#endif
