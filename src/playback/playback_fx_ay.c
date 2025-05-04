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


int handleFX_AY(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  if (fx->fx == fxAYM) handleFX_AYM(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxNOA) handleFX_NOA(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxNOI) handleFX_NOI(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxERT) handleFX_ERT(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxEAU) handleFX_EAU(state, track, trackIdx, fx, tableState);

  return 0;
}
