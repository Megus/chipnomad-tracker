#ifndef __PLAYBACK_INTERNAL_H__
#define __PLAYBACK_INTERNAL_H__

#include "playback.h"

void handleNoteOff(PlaybackState* state, int trackIdx);
void readPhraseRow(PlaybackState* state, int trackIdx, int skilDelCheck);
void readPhraseRowDirect(PlaybackState* state, int trackIdx, PhraseRow* phraseRow, int skipDelCheck);
void tableInit(PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int tableIdx, int speed);
void tableReadFX(PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int fxIdx, int forceRead);
void initFX(PlaybackState* state, int trackIdx, uint8_t* fx, PlaybackTableState* tableState, int tableFXColumn, PhraseRow* phraseRow, int forceCleanState);
int handleFX(PlaybackState* state, int trackIdx, int chipIdx);
int restartFX(PlaybackState* state, int trackIdx);
void hopToTableRow(PlaybackState* state, int trackIdx, PlaybackTableState* table, int tableRow);
int vibratoCommonLogic(PlaybackFXState *pvbState, int scale);
void resetOffsets(PlaybackState* state, int trackIdx);

// FX handler table
extern PlaybackFXHandler fxHandlers[fxTotalCount];
void initFXHandlers(void);
void registerFXHandlers_AY(void);

// Chip-specific functions

// AY-3-8910/YM2149F
void setupInstrumentAY(PlaybackState* state, int trackIdx);
void noteOffInstrumentAY(PlaybackState* state, int trackIdx);
void handleInstrumentAY(PlaybackState* state, int trackIdx);
void outputRegistersAY(PlaybackState* state, int trackIdx, int chipIdx, SoundChip* chip);
void resetTrackAY(PlaybackState* state, int trackIdx);


// Convert frequency to AY period with optimal accuracy
int frequencyToAYPeriod(float frequency, int clockHz);

#endif
