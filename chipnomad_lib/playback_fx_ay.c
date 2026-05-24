#include "playback.h"
#include "playback_internal.h"
#include "utils.h"
#include <stdio.h>

static int getInstrumentType(PlaybackState* state, int trackIdx) {
  int instrumentIdx = state->tracks[trackIdx].note.instrument;
  return state->p->instruments[instrumentIdx].type;
}

static int isAYInstrument(PlaybackState* state, int trackIdx) {
  uint8_t type = getInstrumentType(state, trackIdx);

  if (type == instAY1 || type == instAY2 || type == instAYSample || type == instAYWavetable) {
    return 1;
  }
  return 0;
}

// =================
// AY Common Effects
// =================

// AYM - AY Mixer
static void handleFX_AYM(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (!isAYInstrument(state, trackIdx)) return;

  uint8_t value = fx->fxValue;
  value = ~value; // Invert mixer bits to match AY behavior
  track->note.chip.ay.mixer = (value & 0x1) + ((value & 0x2) << 2);
  track->note.chip.ay.envShape = (fx->fxValue & 0xf0) >> 4;
}

// NOA - Absolute noise period value
static void handleFX_NOA(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (!isAYInstrument(state, trackIdx)) return;

  if (fx->fxValue == EMPTY_VALUE_8) {
    // Bypass setting noise period
    track->note.chip.ay.noiseBase = EMPTY_VALUE_8;
  } else {
    track->note.chip.ay.noiseBase = fx->fxValue & 0x1f;
  }
}

// NOI - Relative noise period value
static void initFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (!isAYInstrument(state, trackIdx)) return;

  if (track->note.chip.ay.noiseBase == EMPTY_VALUE_8) track->note.chip.ay.noiseBase = 0;
  fx->acc += fx->fxValue;
}

static void restartFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - NOI should keep the accumulated offset
}

static void handleFX_NOI(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (!isAYInstrument(state, trackIdx)) return;

  track->note.chip.ay.noiseOffset += fx->acc;
}

// ERT - Envelope retrigger
static void handleFX_ERT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (!isAYInstrument(state, trackIdx)) return;

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


// ====================
// AY1 Specific Effects
// ====================

// ENT - Envelope note
static void handleFX_ENT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY1) return;

  struct Project *p = state->p;
  int note = fx->fxValue + p->pitchTable.octaveSize * 4;  // AY env period is 4 octaves lower
  if (note >= p->pitchTable.length) note = p->pitchTable.length - 1;

  if (p->linearPitch) {
    // Linear pitch mode: convert cents to frequency, then to period
    int cents = p->pitchTable.values[note];
    float frequency = centsToFrequency(cents);
    track->note.chip.ay.envPeriodBase = frequencyToAYPeriod(frequency, p->chipSetup.ay.clock);
  } else {
    // Traditional period mode
    track->note.chip.ay.envPeriodBase = p->pitchTable.values[note];
  }
}

// EPT - Envelope period offset
static void initFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY1) {
    fx->isOn = 0;
    return;
  }
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - EPT should keep the accumulated offset
}

static void handleFX_EPT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.envPeriodOffset += fx->acc;
}

// EPL - Envelope period Low
static void handleFX_EPL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY1) return;

  track->note.chip.ay.envPeriodBase = (track->note.chip.ay.envPeriodBase & 0xff00) + fx->fxValue;
}

// EPH - Envelope period High
static void handleFX_EPH(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY1) return;

  track->note.chip.ay.envPeriodBase = (track->note.chip.ay.envPeriodBase & 0x00ff) + (fx->fxValue << 8);
}

// EBN - Envelope pitch bend
static void initFX_EBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY1) {
    fx->isOn = 0;
    return;
  }

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
  track->note.chip.ay.envPeriodOffset += fx->acc >> 8;
}

// EVB - Envelope vibrato
static void restartFX_EVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - EVB should continue uninterrupted
}

static void handleFX_EVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (getInstrumentType(state, trackIdx) != instAY1) return;

  track->note.chip.ay.envPeriodOffset += vibratoCommonLogic(fx, 1);
}

// ESL - Pitch slide (portamento
static void initFX_ESL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY1) {
    fx->isOn = 0;
    return;
  }

  if (track->note.chip.ay.envPeriodBase != 0) {
    fx->d.slide.startPeriod = track->note.chip.ay.envPeriodBase;
  } else {
    fx->isOn = 0;
  }
}

static void handleFX_ESL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (fx->d.slide.startPeriod == 0 || fx->counter >= fx->fxValue) {
    fx->isOn = 0; // Reached the end or no valid start, turn off FX
    return;
  } else if (fx->counter == 0) {
    fx->d.slide.endPeriod = track->note.chip.ay.envPeriodBase;
  }
  int distance = fx->d.slide.endPeriod - fx->d.slide.startPeriod;
  int offset = (distance * fx->counter) / fx->fxValue;
  track->note.chip.ay.envPeriodOffset += distance - offset;
}


// ============================================
// AY2, AYSample and AYWavetable Common Effects
// ============================================

// TNN - Tone specific note
static void handleFX_TNN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (!(isAYInstrument(state, trackIdx) && getInstrumentType(state, trackIdx) != instAY1)) return;

  track->note.chip.ay.toneFixedPitch = fx->fxValue;
}

// TNP - Tone pitch offset
static void initFX_TNP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (!(isAYInstrument(state, trackIdx) && getInstrumentType(state, trackIdx) != instAY1)) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_TNP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - TNP should keep the accumulated offset
}

static void handleFX_TNP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.tonePitchOffset += fx->acc;
}

// TNF - Tone fine offset
static void initFX_TNF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (!(isAYInstrument(state, trackIdx) && getInstrumentType(state, trackIdx) != instAY1)) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_TNF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - TNF should keep the accumulated offset
}

static void handleFX_TNF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.toneFineOffset += fx->acc;
}

// TRT - Tone phase retrigger
static void handleFX_TRT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  // TODO: Implement tone oscillator phase retrigger
}


// ====================
// AY2 Specific Effects
// ====================

// ENN - Envelope specific note
static void handleFX_ENN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY2) return;

  track->note.chip.ay.envFixedPitch = fx->fxValue;
}

// ENP - Envelope pitch offset
static void initFX_ENP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY2) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_ENP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - ENP should keep the accumulated offset
}

static void handleFX_ENP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.envPitchOffset += fx->acc;
}

// ENF - Envelope fine offset
static void initFX_ENF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY2) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_ENF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - ENF should keep the accumulated offset
}

static void handleFX_ENF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.envFineOffset += fx->acc;
}

// SFT - Software oscillator type
static void handleFX_SFT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY2) return;

  // TODO: Implement software oscillator type setting
}

// SFV - Software oscillator aux value
static void handleFX_SFV(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY2) return;

  // TODO: Implement software oscillator aux value setting
}

// SFN - Software oscillator specific note
static void handleFX_SFN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAY2) return;

  track->note.chip.ay.softFixedPitch = fx->fxValue;
}

// SFP - Software oscillator pitch offset
static void initFX_SFP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY2) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_SFP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - SFP should keep the accumulated offset
}

static void handleFX_SFP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.softPitchOffset += fx->acc;
}

// SFF - Software oscillator fine offset
static void initFX_SFF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAY2) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_SFF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - SFF should keep the accumulated offset
}

static void handleFX_SFF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.softFineOffset += fx->acc;
}

// SRT - Software oscillator phase retrigger
static void handleFX_SRT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  // TODO: Implement software oscillator phase retrigger
}


// =========================
// AYSample specific effects
// =========================

// SMN - Sample specific note
static void handleFX_SMN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAYSample) return;

  track->note.chip.ay.sampleFixedPitch = fx->fxValue;
}

// SMP - Sample pitch offset
static void initFX_SMP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAYSample) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_SMP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - SMP should keep the accumulated offset
}

static void handleFX_SMP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.samplePitchOffset += fx->acc;
}

// SMF - Sample fine offset
static void initFX_SMF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAYSample) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_SMF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - SMF should keep the accumulated offset
}

static void handleFX_SMF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.sampleFineOffset += fx->acc;
}

// SMS - Sample start position
static void handleFX_SMS(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAYSample) return;

  // TODO: Implement sample start position setting
}


// ============================
// AYWavetable specific effects
// ============================

// WTX - Wavetable index offset
static void initFX_WTX(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAYWavetable) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_WTX(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - WTX should keep the accumulated offset
}

static void handleFX_WTX(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.wavetableIndexOffset += fx->acc;
}

// WTN - Wavetable specific note
static void handleFX_WTN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAYWavetable) return;

  track->note.chip.ay.wavetableFixedPitch = fx->fxValue;
}

// WTP - Wavetable pitch offset
static void initFX_WTP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAYWavetable) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_WTP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - WTP should keep the accumulated offset
}

static void handleFX_WTP(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.wavetablePitchOffset += fx->acc;
}

// WTF - Wavetable fine offset
static void initFX_WTF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState* tableState, int tableFXColumn) {
  if (getInstrumentType(state, trackIdx) != instAYWavetable) {
    fx->isOn = 0;
    return;
  }

  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_WTF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - WTF should keep the accumulated offset
}

static void handleFX_WTF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.chip.ay.wavetableFineOffset += fx->acc;
}

// WRT - Wavetable phase retrigger
static void handleFX_WRT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0; // Atomic effect
  if (getInstrumentType(state, trackIdx) != instAYWavetable) return;
  // TODO: Implement wavetable phase retrigger
}

void registerFXHandlers_AY(void) {
  // Common AY FX
  fxHandlers[fxAYM] = (PlaybackFXHandler){NULL, handleFX_AYM, NULL};
  fxHandlers[fxNOA] = (PlaybackFXHandler){NULL, handleFX_NOA, NULL};
  fxHandlers[fxNOI] = (PlaybackFXHandler){initFX_NOI, handleFX_NOI, restartFX_NOI};
  fxHandlers[fxERT] = (PlaybackFXHandler){NULL, handleFX_ERT, NULL};
  fxHandlers[fxEAU] = (PlaybackFXHandler){NULL, handleFX_EAU, NULL};

  // AY2, AYSample and AYWavetable common FX
  fxHandlers[fxTNN] = (PlaybackFXHandler){NULL, handleFX_TNN, NULL};
  fxHandlers[fxTNP] = (PlaybackFXHandler){initFX_TNP, handleFX_TNP, restartFX_TNP};
  fxHandlers[fxTNF] = (PlaybackFXHandler){initFX_TNF, handleFX_TNF, restartFX_TNF};
  fxHandlers[fxTRT] = (PlaybackFXHandler){NULL, handleFX_TRT, NULL};

  // AY1-specific FX
  fxHandlers[fxENT] = (PlaybackFXHandler){NULL, handleFX_ENT, NULL};
  fxHandlers[fxEPT] = (PlaybackFXHandler){initFX_EPT, handleFX_EPT, restartFX_EPT};
  fxHandlers[fxEPL] = (PlaybackFXHandler){NULL, handleFX_EPL, NULL};
  fxHandlers[fxEPH] = (PlaybackFXHandler){NULL, handleFX_EPH, NULL};
  fxHandlers[fxEBN] = (PlaybackFXHandler){initFX_EBN, handleFX_EBN, NULL};
  fxHandlers[fxEVB] = (PlaybackFXHandler){NULL, handleFX_EVB, restartFX_EVB};
  fxHandlers[fxESL] = (PlaybackFXHandler){initFX_ESL, handleFX_ESL, NULL};

  // AY2-specific FX (software oscillator)
  fxHandlers[fxENN] = (PlaybackFXHandler){NULL, handleFX_ENN, NULL};
  fxHandlers[fxENP] = (PlaybackFXHandler){initFX_ENP, handleFX_ENP, restartFX_ENP};
  fxHandlers[fxENF] = (PlaybackFXHandler){initFX_ENF, handleFX_ENF, restartFX_ENF};
  fxHandlers[fxSFT] = (PlaybackFXHandler){NULL, handleFX_SFT, NULL};
  fxHandlers[fxSFV] = (PlaybackFXHandler){NULL, handleFX_SFV, NULL};
  fxHandlers[fxSFN] = (PlaybackFXHandler){NULL, handleFX_SFN, NULL};
  fxHandlers[fxSFP] = (PlaybackFXHandler){initFX_SFP, handleFX_SFP, restartFX_SFP};
  fxHandlers[fxSFF] = (PlaybackFXHandler){initFX_SFF, handleFX_SFF, restartFX_SFF};
  fxHandlers[fxSRT] = (PlaybackFXHandler){NULL, handleFX_SRT, NULL};

  // AYSample-specific FX
  fxHandlers[fxSMN] = (PlaybackFXHandler){NULL, handleFX_SMN, NULL};
  fxHandlers[fxSMP] = (PlaybackFXHandler){initFX_SMP, handleFX_SMP, restartFX_SMP};
  fxHandlers[fxSMF] = (PlaybackFXHandler){initFX_SMF, handleFX_SMF, restartFX_SMF};
  fxHandlers[fxSMS] = (PlaybackFXHandler){NULL, handleFX_SMS, NULL};

  // AYWavetable-specific FX
  fxHandlers[fxWTX] = (PlaybackFXHandler){initFX_WTX, handleFX_WTX, restartFX_WTX};
  fxHandlers[fxWTN] = (PlaybackFXHandler){NULL, handleFX_WTN, NULL};
  fxHandlers[fxWTP] = (PlaybackFXHandler){initFX_WTP, handleFX_WTP, restartFX_WTP};
  fxHandlers[fxWTF] = (PlaybackFXHandler){initFX_WTF, handleFX_WTF, restartFX_WTF};
  fxHandlers[fxWRT] = (PlaybackFXHandler){NULL, handleFX_WRT, NULL};
}
