#include "playback.h"
#include "playback_internal.h"
#include <stdio.h>
#include <string.h>

PlaybackFXHandler fxHandlers[fxTotalCount] = {0};

static int iabs(int v) {
  return (v < 0) ? -v : v;
}

int vibratoCommonLogic(PlaybackFXState *fx, int scale) {
  int speed = (fx->fxValue & 0xf0) >> 4;
  int p = 32 - speed * 2;
  int step = 0x1000000 / p; // using 16.16 fixed point
  int a = (fx->fxValue & 0xf) * scale;
  int x = ((fx->acc * p / 0x1000000) + p / 4) % p;
  int value = a - (4 * a * iabs(x - p / 2)) / p;

  fx->acc += step;
  if (fx->acc >= 0x1000000) fx->acc = 0; // Stabilize vibrato and prefent overflow

  return value;
}

static uint8_t calculateArpUpDownOffset(const uint8_t *arp, const uint8_t period, const int cycleCounter, const int stepsTotal, const uint8_t octaveSize) {
  const int currentStep = (period + cycleCounter * 3) % stepsTotal;
  int octave, idx;

  switch (stepsTotal) {
  case 4:
    return currentStep < 3 ? arp[currentStep] : arp[1];
  case 10:
    if (currentStep < 3) return arp[currentStep];
    if (currentStep < 8) {
      idx = (currentStep <= 5) ? (currentStep - 3) : (7 - currentStep);
      return arp[idx] + octaveSize;
    }
    return arp[10 - currentStep];
  default:
    if (currentStep < stepsTotal / 2) {
      idx = currentStep % 3;
      octave = currentStep / 3;
      return arp[idx] + (octave * octaveSize);
    }
    int s = currentStep - stepsTotal / 2;
    idx = 2 - (s % 3);
    octave = 2 - (s / 3);
    return arp[idx] + (octave * octaveSize);
  }
}

static int8_t calculateArpModeOffset(uint8_t arp[3], const uint8_t period, const int cycleCounter, const enum PlaybackArpType arpType, const uint8_t octaveSize) {
  uint8_t noteOffset = 0;

  switch (arpType) {
  case arpTypeUpDown4Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 28, octaveSize);
    break;
  case arpTypeUpDown3Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 22, octaveSize);
    break;
  case arpTypeUpDown2Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 16, octaveSize);
    break;
  case arpTypeUpDown1Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 10, octaveSize);
    break;
  case arpTypeUpDown:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 4, octaveSize);
    break;
  case arpTypeDown4Oct:
    noteOffset = (cycleCounter % 5) * -octaveSize + arp[2-period];
    break;
  case arpTypeDown3Oct:
    noteOffset = (cycleCounter % 4) * -octaveSize + arp[2-period];
    break;
  case arpTypeDown2Oct:
    noteOffset = (cycleCounter % 3) * -octaveSize + arp[2-period];
    break;
  case arpTypeDown1Oct:
    if (cycleCounter % 2 == 1) {
      noteOffset = -octaveSize;
    }
  case arpTypeDown:
    noteOffset += arp[2-period];
    break;
  case arpTypeUp5Oct:
    noteOffset = (cycleCounter % 6) * octaveSize + arp[period];
    break;
  case arpTypeUp4Oct:
    noteOffset = (cycleCounter % 5) * octaveSize + arp[period];
    break;
  case arpTypeUp3Oct:
    noteOffset = (cycleCounter % 4) * octaveSize + arp[period];
    break;
  case arpTypeUp2Oct:
    noteOffset = (cycleCounter % 3) * octaveSize + arp[period];
    break;
  case arpTypeUp1Oct:
    if (cycleCounter % 2 == 1) {
      noteOffset = octaveSize;
    }
  case arpTypeUp:
    default:
    noteOffset += arp[period];
    break;
  }
  return noteOffset;
}

///////////////////////////////////////////////////////////////////////////////
//
// FX implementations
//

// PBN - pitch bend
static void initFX_PBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  // Calculate per-frame change
  int speed = 1;
  if (tableFXColumn >= 0) {
    speed = tableState->speed[tableFXColumn];
  } else {
    speed = state->p->grooves[track->grooveIdx].speed[track->grooveRow];
  }
  if (speed == 0) speed = 1;
  int value = (int8_t)(fx->fxValue) << 8; // Use 24.8 fixed point math
  if (state->p->linearPitch) {
    // Linear pitch mode: multiply by 25 for cents
    value *= 25;
  }
  fx->d.bend.speed = value / speed;
}

static void handleFX_PBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->acc += fx->d.bend.speed;
  track->note.pitchOffset += fx->acc >> 8;
}

// ARP - arpeggio
static void handleFX_ARP(struct PlaybackState *state, PlaybackTrackState *track, int trackIdx, int chipIdx, PlaybackFXState *fx) {
  if (fx->d.arpeggio.speed == 0) fx->d.arpeggio.speed = 1;
  uint8_t arp[3] = {0, (fx->fxValue & 0xF0) >> 4, fx->fxValue & 0x0F};
  const uint8_t period = fx->counter / fx->d.arpeggio.speed % 3;
  const uint8_t cycles = fx->counter / fx->d.arpeggio.speed / 3;
  track->note.noteOffset += calculateArpModeOffset(arp, period, cycles, fx->d.arpeggio.type, state->p->pitchTable.octaveSize);
}

static void restartFX_ARP(struct PlaybackState *state, PlaybackTrackState *track, int trackIdx, PlaybackFXState *fx) {
  // Do nothing for ARP - it should continue uninterrupted
}

// ARC - Arp settings
static void initFX_ARC(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  int speed = fx->fxValue & 0x0F;
  if (speed == 0) speed = 1;
  track->note.fx[fxARP].d.arpeggio.speed = speed;
  track->note.fx[fxARP].d.arpeggio.type = (fx->fxValue & 0xF0) >> 4;
}

// PIT - Pitch offset (semitones)
static void initFX_PIT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->acc += fx->fxValue;
}

static void restartFX_PIT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - PIT should kepp the accumulated offset
}

static void handleFX_PIT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.noteOffset += fx->acc;
}

// FIN - Fine pitch offset
static void initFX_FIN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_FIN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - FIN should keep the accumulated offset
}

static void handleFX_FIN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.pitchOffset += fx->acc;
}

// PRD - Period offset
static void initFX_PRD(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_PRD(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - PRD should keep the accumulated offset
}

static void handleFX_PRD(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.periodOffset += fx->acc;
}

// TBX - aux table
static void initFX_TBX(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  tableInit(state, trackIdx, &track->note.auxTable, fx->fxValue, 1);
  fx->isOn = 0;
}

// TBL - instrument table
static void initFX_TBL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  tableInit(state, trackIdx, &track->note.instrumentTable, fx->fxValue, 1);
  fx->isOn = 0;
}

// THO - Table hop (instrument table only)
static void initFX_THO(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->isOn = 0;
  // FX is in Phrase - hop only in instrument table
  if (tableState == NULL && track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
    hopToTableRow(state, trackIdx, &track->note.instrumentTable, fx->fxValue & 0xf);
  }
}

// TXH - Aux table hop
static void initFX_TXH(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->isOn = 0;
  // FX is in Phrase - hop only in aux table
  if (tableState == NULL && track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
    hopToTableRow(state, trackIdx, &track->note.auxTable, fx->fxValue & 0xf);
  }
}

// TIC - Table speed
static void initFX_TIC(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->isOn = 0;
  if (tableState == NULL) {
    // TIC in Phrase - set TIC speed for all FX lanes in both instrument and aux tables
    if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
      for (int i = 0; i < 4; i++) {
        track->note.instrumentTable.speed[i] = fx->fxValue;
      }
    }
    if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
      for (int i = 0; i < 4; i++) {
        track->note.auxTable.speed[i] = fx->fxValue;
      }
    }
  } else {
    // TIC in table - set it only for the current FX lane
    tableState->speed[tableFXColumn] = fx->fxValue;
  }
}

// VOL - Volume offset
static void initFX_VOL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  fx->acc += (int8_t)fx->fxValue;
}

static void restartFX_VOL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - VOL should keep the accumulated offset
}

static void handleFX_VOL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  track->note.volumeOffset += fx->acc;
}

// GRV - Track groove
static void handleFX_GRV(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0;
  track->grooveIdx = fx->fxValue & (PROJECT_MAX_GROOVES - 1);
  track->pendingGrooveIdx = track->grooveIdx;
  track->grooveRow = 0;
}

// GGR - Global groove
static void handleFX_GGR(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  fx->isOn = 0;
  uint8_t grooveIdx = fx->fxValue & (PROJECT_MAX_GROOVES - 1);
  // Current track changes immediately
  track->grooveIdx = grooveIdx;
  track->pendingGrooveIdx = grooveIdx;
  track->grooveRow = 0;
  track->frameCounter = 0;
  // Handle all other tracks
  for (int c = 0; c < state->p->tracksCount; c++) {
    if (c != trackIdx) {
      if (c < trackIdx) {
        // Previous tracks: change immediately (already processed this frame)
        state->tracks[c].grooveIdx = grooveIdx;
        state->tracks[c].pendingGrooveIdx = grooveIdx;
        state->tracks[c].grooveRow = 0;
        // Don't touch frameCounter - it was already incremented
      } else {
        // Later tracks: use pending groove
        state->tracks[c].pendingGrooveIdx = grooveIdx;
      }
    }
  }
}

// OFF - Note off
static void handleFX_OFF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (fx->counter >= fx->fxValue) {
    fx->isOn = 0;
    handleNoteOff(state, trackIdx);
  }
}

// KIL - Kill note
static void handleFX_KIL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (fx->counter >= fx->fxValue) {
    fx->isOn = 0;
    track->note.noteBase = EMPTY_VALUE_8;
  }
}

// DEL - Delay note
static void initFX_DEL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  if (tableState != NULL) {
    fx->isOn = 0;
  }
}

static void handleFX_DEL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (fx->counter >= fx->fxValue) {
    fx->isOn = 0;
    readPhraseRow(state, trackIdx, 1);
  }
}

// RET - Note retrigger
static void restartFX_RET(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  fx->counter = 0;
}

static void handleFX_RET(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  uint8_t delay = fx->fxValue & 0xf;
  int8_t volumeOffset = (fx->fxValue & 0xf0) >> 4;

  if (delay == 0) {
    // Delay 0 doesn't retrigger note or stops the effect.
    fx->isOn = 0;
    return;
  }

  if (volumeOffset == 0 || volumeOffset == 8) {
    // Zero offset
    volumeOffset = 0;
  } else if (volumeOffset < 8) {
    // Negative volume offset
    volumeOffset = -8 + volumeOffset;
  } else {
    // Positive volume offset
    volumeOffset = volumeOffset - 8;
  }

  if (fx->counter % delay == 0) {
    setupInstrumentAY(state, trackIdx);
    tableInit(state, trackIdx, &track->note.instrumentTable, track->note.instrumentTable.tableIdx, 1);
    tableInit(state, trackIdx, &track->note.auxTable, track->note.auxTable.tableIdx, 1);
    restartFX(state, trackIdx);
    fx->acc += volumeOffset;
  }
  track->note.volumeOffset += fx->acc;
}

// PVB - Pitch vibrato
static void restartFX_PVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx) {
  // Do nothing - PVB should continue uninterrupted
}

static void handleFX_PVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  int scale = state->p->linearPitch ? 10 : 1;
  track->note.pitchOffset += vibratoCommonLogic(fx, scale);
}


// PSL - Pitch slide (portamento)
static void initFX_PSL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, PlaybackFXState* fx, PlaybackTableState *tableState, int tableFXColumn) {
  if (track->note.noteBase != NOTE_OFF && track->note.noteBase != EMPTY_VALUE_8) {
    fx->d.slide.startPeriod = state->p->pitchTable.values[track->note.noteBase];
  } else {
    fx->isOn = 0;
  }
}

static void handleFX_PSL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, PlaybackFXState* fx) {
  if (fx->d.slide.startPeriod == 0 || fx->counter >= fx->fxValue) {
    fx->isOn = 0;
    return;
  } else if (fx->counter == 0) {
    fx->d.slide.endPeriod = state->p->pitchTable.values[track->note.noteBase];
  }
  int distance = fx->d.slide.endPeriod - fx->d.slide.startPeriod;
  int offset = (distance * fx->counter) / fx->fxValue;
  track->note.pitchOffset += state->p->linearPitch ? offset - distance : distance - offset;
}



///////////////////////////////////////////////////////////////////////////////
//
// General FX handling functions
//

void initFX(PlaybackState* state, int trackIdx, uint8_t* fx, PlaybackTableState* tableState, int tableFXColumn, PhraseRow* phraseRow) {
  if (fx[0] == EMPTY_VALUE_8 || fx[0] >= fxTotalCount) return;

  PlaybackTrackState* track = &state->tracks[trackIdx];
  uint8_t fxIdx = fx[0];
  PlaybackFXState* fxState = &track->note.fx[fxIdx];

  fxState->isOn = 1;
  fxState->fxValue = fx[1];

  if (fxHandlers[fxIdx].init) {
    fxHandlers[fxIdx].init(state, track, trackIdx, fxState, tableState, tableFXColumn);
  }
}

void initFXHandlers(void) {
  memset(fxHandlers, 0, sizeof(fxHandlers));
  fxHandlers[fxARP] = (PlaybackFXHandler){NULL, handleFX_ARP, restartFX_ARP};
  fxHandlers[fxARC] = (PlaybackFXHandler){initFX_ARC, NULL, NULL};
  fxHandlers[fxPVB] = (PlaybackFXHandler){NULL, handleFX_PVB, restartFX_PVB};
  fxHandlers[fxPBN] = (PlaybackFXHandler){initFX_PBN, handleFX_PBN, NULL};
  fxHandlers[fxPSL] = (PlaybackFXHandler){initFX_PSL, handleFX_PSL, NULL};
  fxHandlers[fxPIT] = (PlaybackFXHandler){initFX_PIT, handleFX_PIT, restartFX_PIT};
  fxHandlers[fxFIN] = (PlaybackFXHandler){initFX_FIN, handleFX_FIN, restartFX_FIN};
  fxHandlers[fxPRD] = (PlaybackFXHandler){initFX_PRD, handleFX_PRD, restartFX_PRD};
  fxHandlers[fxVOL] = (PlaybackFXHandler){initFX_VOL, handleFX_VOL, restartFX_VOL};
  fxHandlers[fxRET] = (PlaybackFXHandler){NULL, handleFX_RET, restartFX_RET};
  fxHandlers[fxDEL] = (PlaybackFXHandler){initFX_DEL, handleFX_DEL, NULL};
  fxHandlers[fxOFF] = (PlaybackFXHandler){NULL, handleFX_OFF, NULL};
  fxHandlers[fxKIL] = (PlaybackFXHandler){NULL, handleFX_KIL, NULL};
  fxHandlers[fxTIC] = (PlaybackFXHandler){initFX_TIC, NULL, NULL};
  fxHandlers[fxTHO] = (PlaybackFXHandler){initFX_THO, NULL, NULL};
  fxHandlers[fxTXH] = (PlaybackFXHandler){initFX_TXH, NULL, NULL};
  fxHandlers[fxTBL] = (PlaybackFXHandler){initFX_TBL, NULL, NULL};
  fxHandlers[fxTBX] = (PlaybackFXHandler){initFX_TBX, NULL, NULL};
  fxHandlers[fxGRV] = (PlaybackFXHandler){NULL, handleFX_GRV, NULL};
  fxHandlers[fxGGR] = (PlaybackFXHandler){NULL, handleFX_GGR, NULL};
  registerFXHandlers_AY();
}

int handleFX(PlaybackState* state, int trackIdx, int chipIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  // All FX on this note
  for (int i = 0; i < fxTotalCount; i++) {
    if (track->note.fx[i].isOn && fxHandlers[i].handle) {
      fxHandlers[i].handle(state, track, trackIdx, chipIdx, &track->note.fx[i]);
      track->note.fx[i].counter++;
    }
  }

  return 0;
}

int restartFX(PlaybackState* state, int trackIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  for (int i = 0; i < fxTotalCount; i++) {
    if (track->note.fx[i].isOn) {
      if (fxHandlers[i].restart) {
        fxHandlers[i].restart(state, track, trackIdx, &track->note.fx[i]);
      } else {
        track->note.fx[i].counter = 0;
        track->note.fx[i].acc = 0;
      }
    }
  }

  return 0;
}
