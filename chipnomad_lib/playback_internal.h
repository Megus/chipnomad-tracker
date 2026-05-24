#ifndef __PLAYBACK_INTERNAL_H__
#define __PLAYBACK_INTERNAL_H__

#include "chipnomad_lib.h"

void handleNoteOff(PlaybackState* state, int trackIdx);
void readPhraseRow(PlaybackState* state, int trackIdx, int skipDelCheck);
void readPhraseRowDirect(PlaybackState* state, int trackIdx, PhraseRow* phraseRow, int skipDelCheck);
void tableInit(PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int tableIdx, int row, int speed);
void tableReadFX(PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int fxIdx);
void initFX(PlaybackState* state, int trackIdx, uint8_t* fx, PlaybackTableState* tableState, int tableFXColumn);
int handleFX(PlaybackState* state, int trackIdx, int chipIdx);
int restartFX(PlaybackState* state, int trackIdx);
void hopToTableRow(PlaybackState* state, int trackIdx, PlaybackTableState* table, int tableRow);
int vibratoCommonLogic(PlaybackFXState *pvbState, int scale);
void resetOffsets(PlaybackState* state, int trackIdx);

// FX handler table
extern PlaybackFXHandler fxHandlers[fxTotalCount];
void initFXHandlers(void);
void registerFXHandlers_AY(void);
void registerFXHandlers_Modulation(void);

// Chip-specific functions

// AY-3-8910/YM2149F
int timerFunctionAY(struct SoundChip* chip, void* userdata);
void setupInstrumentAY1(PlaybackState* state, int trackIdx);
void setupInstrumentAY2(PlaybackState* state, int trackIdx);
void setupInstrumentAYSample(PlaybackState* state, int trackIdx);
void setupInstrumentAYWavetable(PlaybackState* state, int trackIdx);
void setupInstrument(PlaybackState* state, int trackIdx);
void handleInstrumentAY1(PlaybackState* state, int trackIdx);
void handleInstrumentAY2(PlaybackState* state, int trackIdx);
void handleInstrumentAYSample(PlaybackState* state, int trackIdx);
void handleInstrumentAYWavetable(PlaybackState* state, int trackIdx);
void outputRegistersAY(ChipNomadState* state, int trackIdx, int chipIdx);
void resetTrackAY(PlaybackState* state, int trackIdx);
void resetOffsetsAY(PlaybackState* state, int trackIdx);

// Convert frequency to AY period
int frequencyToAYPeriod(float frequency, int clockHz);

#endif
