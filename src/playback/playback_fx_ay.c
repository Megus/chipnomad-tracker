#include <playback.h>
#include <playback_internal.h>
#include <stdio.h>

// AYM - AY Mixer
static void handleFX_AYM(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  uint8_t value = fx->value;
  value = ~value; // Invert mixer bits to match AY behavior
  track->note.chip.ay.mixer = (value & 0x1) + ((value & 0x2) << 2);
  track->note.chip.ay.envShape = (fx->value & 0xf0) >> 4;
}

// NOA - Absolute noise period value
static void handleFX_NOA(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.noiseBase = fx->value & 0x1f;
}

// NOI - Relative noise period value
static void handleFX_NOI(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  if (track->note.chip.ay.noiseBase == EMPTY_VALUE_8) track->note.chip.ay.noiseBase = 0;
  track->note.chip.ay.noiseOffsetAcc += (int8_t)fx->value;
}

// ERT - Envelope retrigger
static void handleFX_ERT(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  state->chips[0].ay.envShape = 0;  // TODO: Multi-chip setup
}

// EAU - Auto-env settings
static void handleFX_EAU(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  uint8_t n = (fx->value & 0xf0) >> 4;
  uint8_t d = (fx->value & 0x0f);
  if (d == 0) d = 1;
  track->note.chip.ay.envAutoN = n;
  track->note.chip.ay.envAutoD = d;
}

// ENT - Envelope not
static void handleFX_ENT(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  struct Project *p = state->p;
  fx->fx = EMPTY_VALUE_8;
  int note = fx->value + p->pitchTable.octaveSize * 4;
  if (note >= p->pitchTable.length) note = p->pitchTable.length - 1;
  track->note.chip.ay.envBase = p->pitchTable.values[note];
}

// EPT - Envelope period offset
static void handleFX_EPT(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.envOffsetAcc += (int8_t)fx->value;
}

// EPL - Envelope period Low
static void handleFX_EPL(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.envBase = (track->note.chip.ay.envBase & 0xff00) + fx->value;
}

// EPH - Envelope period High
static void handleFX_EPH(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.envBase = (track->note.chip.ay.envBase & 0x00ff) + (fx->value << 8);
}

// EBN - Envelope pitch bend
static void handleFX_EBN(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
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
  int value = (track->note.chip.ay.envOffsetAcc << 8) + fx->data.pbn.lowByte;
  value += fx->data.pbn.value;
  track->note.chip.ay.envOffsetAcc = value >> 8;
  fx->data.pbn.lowByte = value & 0xff;
}

// EVB - Envelope vibrato
static void handleFX_EVB(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  track->note.chip.ay.envOffset = vibratoCommonLogic(fx);
}

int handleFX_AY(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  if (fx->fx == fxAYM) handleFX_AYM(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxNOA) handleFX_NOA(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxNOI) handleFX_NOI(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxERT) handleFX_ERT(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEAU) handleFX_EAU(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxENT) handleFX_ENT(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEPT) handleFX_EPT(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEPL) handleFX_EPL(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEPH) handleFX_EPH(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEBN) handleFX_EBN(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEVB) handleFX_EVB(state, track, trackIdx, fx, tableState);

  return 0;
}
