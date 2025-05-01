#ifndef __PLAYBACK_INTERNAL_H__
#define __PLAYBACK_INTERNAL_H__

#include <playback.h>

// Chip-specific functions

// AY-3-8910/YM2149F
void setupInstrumentAY(struct PlaybackState* state, int trackIdx);
void noteOffInstrumentAY(struct PlaybackState* state, int trackIdx);
void handleInstrumentAY(struct PlaybackState* state, int trackIdx);
void outputRegistersAY(struct PlaybackState* state, int trackIdx, struct SoundChip* chip);
void resetTrackAY(struct PlaybackState* state, int trackIdx);

void tableInit(struct PlaybackState* state, struct PlaybackTableState* table, int tableIdx, int speed);
int handleFX(struct PlaybackState* state, int trackIdx);

#endif
