#include <playback.h>
#include <playback_internal.h>
#include <stdio.h>

static int handleFXInternal(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState);
static int handleAllTableFX(struct PlaybackState* state, int trackIdx);

///////////////////////////////////////////////////////////////////////////////
//
// FX implementations
//

// PBN - pitch bend
static void handleFX_PBN(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  if (fx->data.pbn.value == 0) {
    // Calculate per-frame change
    int speed = 1;
    if (tableState != NULL) {
      speed = tableState->speed[fx - tableState->fx];
    } else {
      speed = state->p->grooves[track->grooveIdx].speed[track->grooveRow];
    }
    if (speed == 0) speed = 1;
    int value = (int8_t)(fx->value) << 8; // Use 24.8 fixed point math
    fx->data.pbn.value = value / speed;
    fx->data.pbn.lowByte = 0;
  }
  int value = (track->note.pitchOffsetAcc << 8) + fx->data.pbn.lowByte;
  value += fx->data.pbn.value;
  track->note.pitchOffsetAcc = value >> 8;
  fx->data.pbn.lowByte = value & 0xff;
  printf("pitch offset: %d\n", track->note.pitchOffsetAcc);
}

// PIT - Pitch offset
static void handleFX_PIT(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  track->note.pitchOffsetAcc += (int8_t)fx->value;
  fx->fx = EMPTY_VALUE_8;
}

// TBX - aux table
static void handleFX_TBX(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  tableInit(state, &track->note.auxTable, fx->value, 1);
  fx->fx = EMPTY_VALUE_8;
}

// THO - Table hop
static void handleFX_THO(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
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
static void handleFX_TIC(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
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

// VOL - Volume offset
static void handleFX_VOL(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  int volumeOffset = track->note.volumeOffsetAcc + (int8_t)fx->value;
  if (volumeOffset < -128) volumeOffset = -128;
  if (volumeOffset > 127) volumeOffset = 127;
  track->note.volumeOffsetAcc = volumeOffset;
}


///////////////////////////////////////////////////////////////////////////////
//
// General FX handling functions
//

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
  if (fx->fx == EMPTY_VALUE_8) return 0;
  else if (fx->fx == fxPBN) handleFX_PBN(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxPIT) handleFX_PIT(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxTBX) handleFX_TBX(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxTHO) handleFX_THO(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxTIC) handleFX_TIC(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxVOL) handleFX_VOL(state, track, trackIdx, fx, tableState);
  // Chip specific FX
  else {
    handleFX_AY(state, trackIdx, fx, tableState);
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
