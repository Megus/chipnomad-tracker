#include "playback.h"
#include "playback_internal.h"
#include "utils.h"
#include <stdio.h>

// AYM - AY Mixer
static void handleFX_AYM(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  uint8_t value = fx->fxValue;
  value = ~value; // Invert mixer bits to match AY behavior
  track->note.chip.ay.mixer = (value & 0x1) + ((value & 0x2) << 2);
  track->note.chip.ay.envShape = (fx->fxValue & 0xf0) >> 4;
}

// NOA - Absolute noise period value
static void handleFX_NOA(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (fx->fxValue == EMPTY_VALUE_8) {
    track->note.chip.ay.noiseBase = EMPTY_VALUE_8;
  } else {
    track->note.chip.ay.noiseBase = fx->fxValue & 0x1f;
  }
}

// NOI - Relative noise period value
static void initFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (track->note.chip.ay.noiseBase == EMPTY_VALUE_8) track->note.chip.ay.noiseBase = 0;
  fx->acc += fx->fxValue;
}

static void restartFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - NOI should keep the accumulated offset
}

static void handleFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.noiseOffset += fx->acc;
}

// ERT - Envelope retrigger
static void handleFX_ERT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  state->chips[chipIdx].ay.envShape = 0;
}

// EAU - Auto-env settings
static void handleFX_EAU(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  uint8_t n = (fx->fxValue & 0xf0) >> 4;
  uint8_t d = (fx->fxValue & 0x0f);
  if (d == 0) d = 1;
  track->note.chip.ay.envAutoN = n;
  track->note.chip.ay.envAutoD = d;
}

// ENT - Envelope note
static void handleFX_ENT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect

  struct Project *p = state->p;
  int note = fx->fxValue + p->pitchTable.octaveSize * 4;  // AY env period is 4 octaves lower
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
static void initFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - EPT should keep the accumulated offset
}

static void handleFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.envOffset += fx->acc;
}

// EPL - Envelope period Low
static void handleFX_EPL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  track->note.chip.ay.envBase = (track->note.chip.ay.envBase & 0xff00) + fx->fxValue;
}

// EPH - Envelope period High
static void handleFX_EPH(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  track->note.chip.ay.envBase = (track->note.chip.ay.envBase & 0x00ff) + (fx->fxValue << 8);
}

// EBN - Envelope pitch bend
static void initFX_EBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  // Calculate per-frame change
  int speed = 1;
  if (tableFXColumn >= 0) {
    speed = tableState->speed[tableFXColumn];
  } else {
    speed = state->p->grooves[track->grooveIdx].speed[track->grooveRow];
  }
  if (speed == 0) speed = 1;
  int value = (int8_t)(fx->fxValue) << 8; // Use 24.8 fixed point math
  fx->d.bend.speed = value / speed;
}

static void handleFX_EBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->acc += fx->d.bend.speed;
  track->note.chip.ay.envOffset += fx->acc >> 8;
}

// EVB - Envelope vibrato
static void restartFX_EVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - EVB should continue uninterrupted
}

static void handleFX_EVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.envOffset += vibratoCommonLogic(fx, 1);
}

// ESL - Pitch slide (portamento
static void initFX_ESL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (track->note.chip.ay.envBase != 0) {
    fx->d.slide.startPeriod = track->note.chip.ay.envBase;
  } else {
    fx->isOn = 0;
  }
}

static void handleFX_ESL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (fx->d.slide.startPeriod == 0 || fx->counter >= fx->fxValue) {
    fx->isOn = 0; // Reached the end or no valid start, turn off FX
    return;
  } else if (fx->counter == 0) {
    fx->d.slide.endPeriod = track->note.chip.ay.envBase;
  }
  int distance = fx->d.slide.endPeriod - fx->d.slide.startPeriod;
  int offset = (distance * fx->counter) / fx->fxValue;
  track->note.chip.ay.envOffset += distance - offset;
}

void registerFXHandlers_AY(void) {
  fxHandlers[fxAYM] = (PlaybackFXHandler){NULL, handleFX_AYM, NULL};
  fxHandlers[fxNOA] = (PlaybackFXHandler){NULL, handleFX_NOA, NULL};
  fxHandlers[fxNOI] = (PlaybackFXHandler){initFX_NOI, handleFX_NOI, restartFX_NOI};
  fxHandlers[fxERT] = (PlaybackFXHandler){NULL, handleFX_ERT, NULL};
  fxHandlers[fxEAU] = (PlaybackFXHandler){NULL, handleFX_EAU, NULL};
  fxHandlers[fxENT] = (PlaybackFXHandler){NULL, handleFX_ENT, NULL};
  fxHandlers[fxEPT] = (PlaybackFXHandler){initFX_EPT, handleFX_EPT, restartFX_EPT};
  fxHandlers[fxEPL] = (PlaybackFXHandler){NULL, handleFX_EPL, NULL};
  fxHandlers[fxEPH] = (PlaybackFXHandler){NULL, handleFX_EPH, NULL};
  fxHandlers[fxEBN] = (PlaybackFXHandler){initFX_EBN, handleFX_EBN, NULL};
  fxHandlers[fxEVB] = (PlaybackFXHandler){NULL, handleFX_EVB, restartFX_EVB};
  fxHandlers[fxESL] = (PlaybackFXHandler){initFX_ESL, handleFX_ESL, NULL};
}
