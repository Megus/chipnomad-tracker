#include "playback.h"
#include "playback_internal.h"
#include "utils.h"
#include <stdio.h>

// AYM - AY Mixer
static void handleFX_AYM(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  uint8_t value = fx->value;
  value = ~value; // Invert mixer bits to match AY behavior
  track->note.chip.ay.mixer = (value & 0x1) + ((value & 0x2) << 2);
  track->note.chip.ay.envShape = (fx->value & 0xf0) >> 4;
}

// NOA - Absolute noise period value
static void handleFX_NOA(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  if (fx->value == EMPTY_VALUE_8) {
    track->note.chip.ay.noiseBase = EMPTY_VALUE_8;
  } else {
    track->note.chip.ay.noiseBase = fx->value & 0x1f;
  }
}

// NOI - Relative noise period value
static void handleFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  if (track->note.chip.ay.noiseBase == EMPTY_VALUE_8) track->note.chip.ay.noiseBase = 0;
  track->note.chip.ay.noiseOffsetAcc += (int8_t)fx->value;
}

// ERT - Envelope retrigger
static void handleFX_ERT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  state->chips[chipIdx].ay.envShape = 0;
}

// EAU - Auto-env settings
static void handleFX_EAU(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  uint8_t n = (fx->value & 0xf0) >> 4;
  uint8_t d = (fx->value & 0x0f);
  if (d == 0) d = 1;
  track->note.chip.ay.envAutoN = n;
  track->note.chip.ay.envAutoD = d;
}

// ENT - Envelope not
static void handleFX_ENT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  struct Project *p = state->p;
  fx->fx = EMPTY_VALUE_8;
  int note = fx->value + p->pitchTable.octaveSize * 4;
  if (note >= p->pitchTable.length) note = p->pitchTable.length - 1;

  if (p->linearPitch) {
    // Linear pitch mode: convert cents to frequency, then to period
    int cents = p->pitchTable.values[note];
    float frequency = centsToFrequency(cents);
    track->note.chip.ay.envBase = frequencyToAYPeriod(frequency, p->chipSetup.ay.clock);
  } else {
    // Traditional period mode
    track->note.chip.ay.envBase = p->pitchTable.values[note];
  }
}

// EPT - Envelope period offset
static void handleFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.envOffsetAcc += (int8_t)fx->value;
}

// EPL - Envelope period Low
static void handleFX_EPL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.envBase = (track->note.chip.ay.envBase & 0xff00) + fx->value;
}

// EPH - Envelope period High
static void handleFX_EPH(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->note.chip.ay.envBase = (track->note.chip.ay.envBase & 0x00ff) + (fx->value << 8);
}

// EBN - Envelope pitch bend
static void handleFX_EBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
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
static void handleFX_EVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  track->note.chip.ay.envOffset += vibratoCommonLogic(fx, 1);
}

// ESL - Pitch slide (portamento
static void handleFX_ESL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  if (fx->data.psl.startPeriod == 0 || (fx->data.psl.counter == 0 && track->note.chip.ay.envBase == 0) || fx->data.psl.counter >= fx->value) {
    fx->fx = EMPTY_VALUE_8;
    return;
  } else if (fx->data.psl.counter == 0) {
    fx->data.psl.endPeriod = track->note.chip.ay.envBase;
  }
  int distance = fx->data.psl.endPeriod - fx->data.psl.startPeriod;
  int offset = (distance * fx->data.psl.counter) / fx->value;
  track->note.chip.ay.envOffset += distance - offset;

  fx->data.psl.counter++;
}

int handleFX_AY(PlaybackState* state, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  if (fx->fx == fxAYM) handleFX_AYM(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxNOA) handleFX_NOA(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxNOI) handleFX_NOI(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxERT) handleFX_ERT(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxEAU) handleFX_EAU(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxENT) handleFX_ENT(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxEPT) handleFX_EPT(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxEPL) handleFX_EPL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxEPH) handleFX_EPH(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxEBN) handleFX_EBN(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxEVB) handleFX_EVB(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxESL) handleFX_ESL(state, track, trackIdx, chipIdx, fx, tableState);

  return 0;
}
