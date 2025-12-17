#include "playback.h"
#include "playback_internal.h"
#include <stdio.h>
#include <string.h>

static int handleFXInternal(PlaybackState* state, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState);
static int handleAllTableFX(PlaybackState* state, int trackIdx, int chipIdx);

static int iabs(int v) {
  return (v < 0) ? -v : v;
}

int vibratoCommonLogic(PlaybackFXState *fx, int scale) {
  int period = 32 - ((fx->value & 0xf0) >> 3);
  int depth = (fx->value & 0xf) * scale;
  int x = (fx->data.count_fx.counter + period / 4) % period;
  int value = depth - (4 * depth * iabs(x - period / 2)) / period;
  fx->data.count_fx.counter++;
  return value;
}

static uint8_t calculateArpUpDownOffset(const uint8_t *arp, const uint8_t period, const int cycleCounter, const int stepsTotal) {
  const int currentStep = (period + cycleCounter * 3) % stepsTotal;
  int octave, idx;

  switch (stepsTotal) {
  case 4:
    return currentStep < 3 ? arp[currentStep] : arp[1];
  case 10:
    if (currentStep < 3) return arp[currentStep];
    if (currentStep < 8) {
      idx = (currentStep <= 5) ? (currentStep - 3) : (7 - currentStep);
      return arp[idx] + 0x0C;
    }
    return arp[10 - currentStep];
  default:
    if (currentStep < stepsTotal / 2) {
      idx = currentStep % 3;
      octave = currentStep / 3;
      return arp[idx] + (octave * 0x0C);
    }
    int s = currentStep - stepsTotal / 2;
    idx = 2 - (s % 3);
    octave = 2 - (s / 3);
    return arp[idx] + (octave * 0x0C);
  }
}

static int8_t calculateArpModeOffset(uint8_t arp[3], const uint8_t period, const int cycleCounter, const enum PlaybackArpType arpType) {
  uint8_t noteOffset = 0;

  switch (arpType) {
  case arpTypeUpDown4Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 28);
    break;
  case arpTypeUpDown3Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 22);
    break;
  case arpTypeUpDown2Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 16);
    break;
  case arpTypeUpDown1Oct:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 10);
    break;
  case arpTypeUpDown:
    noteOffset = calculateArpUpDownOffset(arp, period, cycleCounter, 4);
    break;
  case arpTypeDown4Oct:
    noteOffset = (cycleCounter % 5) * -0x0c + arp[2-period];
    break;
  case arpTypeDown3Oct:
    noteOffset = (cycleCounter % 4) * -0x0c + arp[2-period];
    break;
  case arpTypeDown2Oct:
    noteOffset = (cycleCounter % 3) * -0x0c + arp[2-period];
    break;
  case arpTypeDown1Oct:
    if (cycleCounter % 2 == 1) {
      noteOffset = -0x0c;
    }
  case arpTypeDown:
    noteOffset += arp[2-period];
    break;
  case arpTypeUp5Oct:
    noteOffset = (cycleCounter % 6) * 0x0c + arp[period];
    break;
  case arpTypeUp4Oct:
    noteOffset = (cycleCounter % 5) * 0x0c + arp[period];
    break;
  case arpTypeUp3Oct:
    noteOffset = (cycleCounter % 4) * 0x0c + arp[period];
    break;
  case arpTypeUp2Oct:
    noteOffset = (cycleCounter % 3) * 0x0c + arp[period];
    break;
  case arpTypeUp1Oct:
    if (cycleCounter % 2 == 1) {
      noteOffset = 0x0c;
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
static void handleFX_PBN(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
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
    if (state->p->linearPitch) {
      // Linear pitch mode: multiply by 25 for cents
      value *= 25;
    }
    fx->data.pbn.value = value / speed;
    fx->data.pbn.lowByte = 0;
  }
  int value = (track->note.pitchOffsetAcc << 8) + fx->data.pbn.lowByte;
  value += fx->data.pbn.value;
  track->note.pitchOffsetAcc = value >> 8;
  fx->data.pbn.lowByte = value & 0xff;
}

// ARP - arpeggio
static void handleFX_ARP(struct PlaybackState *state, PlaybackTrackState *track, int trackIdx, int chipIdx, PlaybackFXState *fx, PlaybackTableState *tableState) {
  uint8_t arp[3] = {0, (fx->value & 0xF0) >> 4, fx->value & 0x0F};
  const uint8_t period = fx->data.count_fx.counter / track->arpSpeed % 3;
  const uint8_t cycles = fx->data.count_fx.counter / track->arpSpeed / 3;
  track->note.noteOffset += calculateArpModeOffset(arp, period, cycles, track->arpType);
  fx->data.count_fx.counter++;
}

// ARC - Arp settings
static void handleFX_ARC(struct PlaybackState *state, PlaybackTrackState *track, int trackIdx, int chipIdx, PlaybackFXState *fx, PlaybackTableState *tableState) {
  track->arpSpeed = fx->value & 0x0F;
  track->arpType = (fx->value & 0xF0) >> 4;
}

// PIT - Pitch offset
static void handleFX_PIT(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  track->note.pitchOffsetAcc += (int8_t)fx->value;
  fx->fx = EMPTY_VALUE_8;
}

// PRD - Period offset
static void handleFX_PRD(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  track->note.periodOffsetAcc += (int8_t)fx->value;
  fx->fx = EMPTY_VALUE_8;
}

// TBX - aux table
static void handleFX_TBX(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  tableInit(state, trackIdx, &track->note.auxTable, fx->value, 1);
  fx->fx = EMPTY_VALUE_8;
}

// TBL - instrument table
static void handleFX_TBL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  tableInit(state, trackIdx, &track->note.instrumentTable, fx->value, 1);
  fx->fx = EMPTY_VALUE_8;
}

// THO - Table hop (instrument table only)
static void handleFX_THO(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  if (tableState == NULL) {
    // FX is in Phrase - hop only in instrument table
    if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
      for (int i = 0; i < 4; i++) {
        track->note.instrumentTable.counters[i] = 0;
        track->note.instrumentTable.rows[i] = fx->value & 0xf;
        tableReadFX(state, trackIdx, &track->note.instrumentTable, i, 0);
      }
    }
    handleAllTableFX(state, trackIdx, chipIdx);
  }
}

// TXH - Aux table hop
static void handleFX_TXH(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  if (tableState == NULL) {
    // FX is in Phrase - hop only in aux table
    if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
      for (int i = 0; i < 4; i++) {
        track->note.auxTable.counters[i] = 0;
        track->note.auxTable.rows[i] = fx->value & 0xf;
        tableReadFX(state, trackIdx, &track->note.auxTable, i, 0);
      }
    }
    handleAllTableFX(state, trackIdx, chipIdx);
  }
}

// TIC - Table speed
static void handleFX_TIC(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
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
static void handleFX_VOL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  int volumeOffset = track->note.volumeOffsetAcc + (int8_t)fx->value;
  if (volumeOffset < -128) volumeOffset = -128;
  if (volumeOffset > 127) volumeOffset = 127;
  track->note.volumeOffsetAcc = volumeOffset;
}

// GRV - Track groove
static void handleFX_GRV(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  track->grooveIdx = fx->value & (PROJECT_MAX_GROOVES - 1);
  track->pendingGrooveIdx = track->grooveIdx;
  track->grooveRow = 0;
}

// GGR - Global groove
static void handleFX_GGR(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  fx->fx = EMPTY_VALUE_8;
  uint8_t grooveIdx = fx->value & (PROJECT_MAX_GROOVES - 1);
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
static void handleFX_OFF(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  if (tableState != NULL || fx->data.count_fx.counter >= fx->value) {
    fx->fx = EMPTY_VALUE_8;
    handleNoteOff(state, trackIdx);
  } else {
    fx->data.count_fx.counter++;
  }
}

// KIL - Kill note
static void handleFX_KIL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  if (tableState != NULL || fx->data.count_fx.counter >= fx->value) {
    fx->fx = EMPTY_VALUE_8;
    track->note.noteBase = EMPTY_VALUE_8;
  } else {
    fx->data.count_fx.counter++;
  }
}

// DEL - Delay note
static void handleFX_DEL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  if (tableState != NULL) {
    fx->fx = EMPTY_VALUE_8;
  } else if (fx->data.count_fx.counter >= fx->value) {
    fx->fx = EMPTY_VALUE_8;
    readPhraseRow(state, trackIdx, 1);
    handleAllTableFX(state, trackIdx, chipIdx);
  } else {
    fx->data.count_fx.counter++;
  }
}

// RET - Note retrigger
static void handleFX_RET(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  uint8_t delay = fx->value & 0xf;
  int8_t volumeOffset = (fx->value & 0xf0) >> 4;

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

  if (tableState != NULL) {
    fx->fx = EMPTY_VALUE_8;
    return;
  } else if (fx->data.count_fx.counter >= delay) {
    readPhraseRow(state, trackIdx, 1);
    handleAllTableFX(state, trackIdx, chipIdx);
    // TODO: Accumulate volume offset separately just for this FX because readPhraseRow resets it
    track->note.volumeOffsetAcc += volumeOffset;
  }
  fx->data.count_fx.counter++;
}

// PVB - Pitch vibrato
static void handleFX_PVB(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  int scale = state->p->linearPitch ? 10 : 1;
  track->note.pitchOffset += vibratoCommonLogic(fx, scale);
}

// PSL - Pitch slide (portamento
static void handleFX_PSL(PlaybackState* state, PlaybackTrackState* track, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  if (fx->data.psl.startPeriod == 0 || (fx->data.psl.counter == 0 && (track->note.noteBase == EMPTY_VALUE_8 || track->note.noteBase == NOTE_OFF)) || fx->data.psl.counter >= fx->value) {
    fx->fx = EMPTY_VALUE_8;
    return;
  } else if (fx->data.psl.counter == 0) {
    fx->data.psl.endPeriod = state->p->pitchTable.values[track->note.noteBase];
  }
  int distance = fx->data.psl.endPeriod - fx->data.psl.startPeriod;
  int offset = (distance * fx->data.psl.counter) / fx->value;
  track->note.pitchOffset += state->p->linearPitch ? offset - distance : distance - offset;

  fx->data.psl.counter++;
}



///////////////////////////////////////////////////////////////////////////////
//
// General FX handling functions
//

static int handleAllTableFX(PlaybackState* state, int trackIdx, int chipIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  // Instrument table FX
  if (track->note.instrumentTable.tableIdx != EMPTY_VALUE_8) {
    for (int i = 0; i < 4; i++) {
      handleFXInternal(state, trackIdx, chipIdx, &track->note.instrumentTable.fx[i], &track->note.instrumentTable);
    }
  }

  // Aux table FX
  if (track->note.auxTable.tableIdx != EMPTY_VALUE_8) {
    for (int i = 0; i < 4; i++) {
      handleFXInternal(state, trackIdx, chipIdx, &track->note.auxTable.fx[i], &track->note.auxTable);
    }
  }

  return 0;
}

static int handlePriorityFXInternal(PlaybackState* state, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  if (fx->fx == EMPTY_VALUE_8) return 0;
  else if (fx->fx == fxTBX) handleFX_TBX(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTBL) handleFX_TBL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTHO) handleFX_THO(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTXH) handleFX_TXH(state, track, trackIdx, chipIdx, fx, tableState);

  return 0;
}

static int handleFXInternal(PlaybackState* state, int trackIdx, int chipIdx, struct PlaybackFXState* fx, PlaybackTableState *tableState) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  // Common FX
  if (fx->fx == EMPTY_VALUE_8) return 0;
  else if (fx->fx == fxARP) handleFX_ARP(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxARC) handleFX_ARC(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxPBN) handleFX_PBN(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxPIT) handleFX_PIT(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxPRD) handleFX_PRD(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTIC) handleFX_TIC(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxVOL) handleFX_VOL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxGRV) handleFX_GRV(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxGGR) handleFX_GGR(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxOFF) handleFX_OFF(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxKIL) handleFX_KIL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxDEL) handleFX_DEL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxRET) handleFX_RET(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxPVB) handleFX_PVB(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxPSL) handleFX_PSL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTBX) handleFX_TBX(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTBL) handleFX_TBL(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTHO) handleFX_THO(state, track, trackIdx, chipIdx, fx, tableState);
  else if (fx->fx == fxTXH) handleFX_TXH(state, track, trackIdx, chipIdx, fx, tableState);

  // Chip specific FX
  else {
    handleFX_AY(state, trackIdx, chipIdx, fx, tableState);
  }

  return 0;
}

void initFX(PlaybackState* state, int trackIdx, uint8_t* fx, PlaybackFXState *fxState, int forceCleanState) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  uint8_t fxIdx = fx[0];
  if (fxIdx == fxDEL) fxIdx = EMPTY_VALUE_8;

  // Generic clean state
  if (forceCleanState || !((fxState->fx == fxPVB || fxState->fx == fxEVB) && fxState->fx == fxIdx)) {
    memset(fxState, 0, sizeof(PlaybackFXState));
  }

  // Special handling for some FX
  if (fxIdx == fxPSL) {
    // PSL - Pitch slide (portamento)
    fxState->data.psl.counter = 0;
    if (track->note.noteBase != EMPTY_VALUE_8) {
      fxState->data.psl.startPeriod = state->p->pitchTable.values[track->note.noteBase];
    } else {
      fxState->data.psl.startPeriod = 0;
    }
  } else if (fxIdx == fxESL) {
    // ESL - AY Envelope slide (portamento)
    fxState->data.psl.counter = 0;
    fxState->data.psl.startPeriod = track->note.chip.ay.envBase;
  }

  if (fxIdx == fxARP) {
    if (track->arpSpeed == 0) track->arpSpeed = 1;
  }

  fxState->fx = fxIdx;
  fxState->value = fx[1];
}

int handleFX(PlaybackState* state, int trackIdx, int chipIdx) {
  PlaybackTrackState* track = &state->tracks[trackIdx];

  // Handle priority Phrase FX
  for (int i = 0; i < 3; i++) {
    handlePriorityFXInternal(state, trackIdx, chipIdx, &track->note.fx[i], NULL);
  }

  handleAllTableFX(state, trackIdx, chipIdx);

  // Phrase FX
  for (int i = 0; i < 3; i++) {
    handleFXInternal(state, trackIdx, chipIdx, &track->note.fx[i], NULL);
  }

  return 0;
}
