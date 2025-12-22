#ifndef __PLAYBACK_INTERNAL_H__
#define __PLAYBACK_INTERNAL_H__

#include "playback.h"

void handleNoteOff(PlaybackState* state, int trackIdx);
void readPhraseRow(PlaybackState* state, int trackIdx, int skilDelCheck);
void readPhraseRowDirect(PlaybackState* state, int trackIdx, PhraseRow* phraseRow, int skipDelCheck);
void tableInit(PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int tableIdx, int speed);
void tableReadFX(PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int fxIdx, int forceRead);
void initFX(PlaybackState* state, int trackIdx, uint8_t* fx, PlaybackFXState *fxState, int forceCleanState);
int handleFX(PlaybackState* state, int trackIdx, int chipIdx);
int vibratoCommonLogic(PlaybackFXState *fxState, int scale);

// Chip-specific functions

// AY-3-8910/YM2149F
void setupInstrumentAY(PlaybackState* state, int trackIdx);
void noteOffInstrumentAY(PlaybackState* state, int trackIdx);
void handleInstrumentAY(PlaybackState* state, int trackIdx);
void outputRegistersAY(PlaybackState* state, int trackIdx, int chipIdx, SoundChip* chip);
void resetTrackAY(PlaybackState* state, int trackIdx);
int handleFX_AY(PlaybackState* state, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState);

// Convert frequency to AY period with optimal accuracy
int frequencyToAYPeriod(float frequency, int clockHz);

#endif
