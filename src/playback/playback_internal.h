#ifndef __PLAYBACK_INTERNAL_H__
#define __PLAYBACK_INTERNAL_H__

#include <playback.h>

void handleNoteOff(struct PlaybackState* state, int trackIdx);
void readPhraseRow(struct PlaybackState* state, int trackIdx, int skilDelCheck);
void tableInit(struct PlaybackState* state, struct PlaybackTableState* table, int tableIdx, int speed);
void tableReadFX(struct PlaybackState* state, struct PlaybackTableState* table, int fxIdx, int forceRead);
int handleFX(struct PlaybackState* state, int trackIdx);

// Chip-specific functions

// AY-3-8910/YM2149F
void setupInstrumentAY(struct PlaybackState* state, int trackIdx);
void noteOffInstrumentAY(struct PlaybackState* state, int trackIdx);
void handleInstrumentAY(struct PlaybackState* state, int trackIdx);
void outputRegistersAY(struct PlaybackState* state, int trackIdx, int chipIdx, struct SoundChip* chip);
void resetTrackAY(struct PlaybackState* state, int trackIdx);
int handleFX_AY(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState);


#endif
