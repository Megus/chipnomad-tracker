#include <playback.h>
#include <playback_internal.h>
#include <stdio.h>

static int handleFXInternal(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState);

static int handleAllTableFX(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  // Instrument table FX
  if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
    for (int i = 0; i < 4; i++) {
      handleFXInternal(state, trackIdx, &track->note.instrumentTable.fx[i], &track->note.instrumentTable);
    }
  }

  // Aux table FX
  if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
    for (int i = 0; i < 4; i++) {
      handleFXInternal(state, trackIdx, &track->note.auxTable.fx[i], &track->note.auxTable);
    }
  }

  return 0;
}

static int handleFXInternal(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
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
    fx->fx = EMPTY_VALUE_8;
    if (tableState == NULL) {
      // FX is in Phrase
      if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
        for (int i = 0; i < 4; i++) {
          track->note.instrumentTable.counters[i] = 0;
          track->note.instrumentTable.rows[i] = fx->value & 0xf;
          tableReadFX(state, &track->note.instrumentTable, i, 0);
        }
      }
      if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
        for (int i = 0; i < 4; i++) {
          track->note.auxTable.counters[i] = 0;
          track->note.auxTable.rows[i] = fx->value & 0xf;
          tableReadFX(state, &track->note.auxTable, i, 0);
        }
      }
      handleAllTableFX(state, trackIdx);
    }
  }
  // TIC - Table speed
  else if (fx->fx == fxTIC) {
    fx->fx = EMPTY_VALUE_8;
    if (tableState == NULL) {
      // TIC in Phrase - set TIC speed for all FX lanes in both instrument and aux tables
      if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
        for (int i = 0; i < 4; i++) {
          track->note.instrumentTable.speed[i] = fx->value;
        }
      }
      if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
        for (int i = 0; i < 4; i++) {
          track->note.auxTable.speed[i] = fx->value;
        }
      }
    } else {
      // TIC in table - set it only for the current FX lane
      for (int c = 0; c < 4; c++) {
        if (&tableState->fx[c] == fx) {
          tableState->speed[c] = fx->value;
          break;
        }
      }
    }
  }
  // Chip specific FX
  else {

  }

  return 0;
}

int handleFX(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  handleAllTableFX(state, trackIdx);

  // Phrase FX
  for (int i = 0; i < 3; i++) {
    handleFXInternal(state, trackIdx, &track->note.fx[i], NULL);
  }

  return 0;
}
