#include <playback.h>
#include <playback_internal.h>
#include <stdio.h>

static int handleFXInternal(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, int isTableFX) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  // Common FX
  if (fx->fx == EMPTY_VALUE_8) {
    return 0;
  }
  // PBN - pitch bend
  else if (fx->fx == fxPBN) {
    // TODO: the value is not per tick, but per row
    track->note.pitchOffsetAcc += (int8_t)fx->value;
  }
  // TBX - aux table
  else if (fx->fx == fxTBX) {
    tableInit(state, &track->note.instrumentTable, fx->value, 1);
    fx->fx = EMPTY_VALUE_8;
  }
  // THO - Table hop
  else if (fx->fx == fxTHO) {
    // TODO: Jump to another row and process that FX right now
    fx->fx = EMPTY_VALUE_8;
  }
  // Chip specific FX
  else {

  }

  return 0;
}

int handleFX(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  // Instrument table FX
  if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
    for (int i = 0; i < 4; i++) {
      handleFXInternal(state, trackIdx, &track->note.instrumentTable.fx[i], 1);
    }
  }

  // Aux table FX
  if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
    for (int i = 0; i < 4; i++) {
      handleFXInternal(state, trackIdx, &track->note.auxTable.fx[i], 1);
    }
  }

  // Phrase FX
  for (int i = 0; i < 3; i++) {
    handleFXInternal(state, trackIdx, &track->note.fx[i], 0);
  }

  return 0;
}
