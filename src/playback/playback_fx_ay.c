#include <playback.h>
#include <playback_internal.h>
#include <stdio.h>

// AYM - AY Mixer
static void handleFX_AYM(struct PlaybackState* state, struct PlaybackTrackState* track, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  uint8_t value = fx->value;
  //uint8_t env = fx->value >> 4;
  value = ~value; // Invert mixer bits to match AY behavior
  track->note.chip.ay.mixer = (value & 0x1) + ((value & 0x2) << 2);
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


int handleFX_AY(struct PlaybackState* state, int trackIdx, struct PlaybackFXState* fx, struct PlaybackTableState *tableState) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  if (fx->fx == fxAYM) handleFX_AYM(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxNOA) handleFX_NOA(state, track, trackIdx, fx, tableState);
  else if (fx->fx == fxNOI) handleFX_NOI(state, track, trackIdx, fx, tableState);

  return 0;
}
