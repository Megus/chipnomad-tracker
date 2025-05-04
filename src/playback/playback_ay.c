#include <playback.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
//
// AY-specific logic
//

void resetTrackAY(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  track->note.chip.ay.mixer = 0; // All disabled
  track->note.chip.ay.noiseBase = EMPTY_VALUE_8;
  track->note.chip.ay.noiseOffsetAcc = 0;
  track->note.chip.ay.envShape = 0;
  track->note.chip.ay.envBase = EMPTY_VALUE_16;
  track->note.chip.ay.envOffsetAcc = 0;
}

void setupInstrumentAY(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  track->note.chip.ay.adsrCounter = 0;
  track->note.chip.ay.envAutoN = p->instruments[track->note.instrument].chip.ay.autoEnvN;
  track->note.chip.ay.envAutoD = p->instruments[track->note.instrument].chip.ay.autoEnvD;
  track->note.chip.ay.envBase = 0;
  track->note.chip.ay.envOffsetAcc = 0;
  track->note.chip.ay.noiseBase = EMPTY_VALUE_8;
  track->note.chip.ay.noiseOffsetAcc = 0;
  track->note.chip.ay.mixer = 8;
  track->note.chip.ay.adsrStep = 0; // ADSR: Attack
  track->note.chip.ay.adsrFrom = 1;
  track->note.chip.ay.adsrTo = 15;
}

void noteOffInstrumentAY(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  if (track->note.instrument == EMPTY_VALUE_8 || track->note.chip.ay.adsrStep == 3) return;

  // ADSR: Release
  track->note.chip.ay.adsrStep = 3;
  track->note.chip.ay.adsrFrom = track->note.chip.ay.adsrVolume;
  track->note.chip.ay.adsrTo = 0;
  track->note.chip.ay.adsrCounter = 0;
}

void handleInstrumentAY(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  if (track->note.noteBase == EMPTY_VALUE_8 || track->note.instrument == EMPTY_VALUE_8) {
    track->note.chip.ay.adsrVolume = 0;
    return;
  }

  // Volume ADSR
  uint8_t adsrVolume = 0;
  while (1) {
    if (track->note.chip.ay.adsrStep > 3) break; // Just in case...

    // Sustain phase
    if (track->note.chip.ay.adsrStep == 2) {
      adsrVolume = p->instruments[track->note.instrument].chip.ay.veS;
      break;
    }

    int duration = 0;
    // Attack
    if (track->note.chip.ay.adsrStep == 0) duration = p->instruments[track->note.instrument].chip.ay.veA;
    // Decay
    else if (track->note.chip.ay.adsrStep == 1) duration = p->instruments[track->note.instrument].chip.ay.veD;
    // Release
    else if (track->note.chip.ay.adsrStep == 3) duration = p->instruments[track->note.instrument].chip.ay.veR;

    if (duration == 0 || track->note.chip.ay.adsrCounter >= duration) {
      // Move to the next ADSR step
      if (track->note.chip.ay.adsrStep == 3) {
        // Release phase end
        adsrVolume = 0;
        track->note.noteBase = EMPTY_VALUE_8;
        break;
      } else {
        track->note.chip.ay.adsrStep++;
        track->note.chip.ay.adsrCounter = 0;
        // Decay to sustain
        if (track->note.chip.ay.adsrStep == 1) {
          track->note.chip.ay.adsrFrom = 15;
          track->note.chip.ay.adsrTo = p->instruments[track->note.instrument].chip.ay.veS;
        }
      }
    } else {
      // LERP between adsrFrom and adsrTo
      track->note.chip.ay.adsrCounter++;
      int from = track->note.chip.ay.adsrFrom;
      int to = track->note.chip.ay.adsrTo;
      int delta = to - from;
      adsrVolume = from + ((delta * ((track->note.chip.ay.adsrCounter << 8) / (duration + 1))) >> 8);
      break;
    }
  }
  track->note.chip.ay.adsrVolume = adsrVolume;
}

void outputRegistersAY(struct PlaybackState* state, int trackIdx, int chipIdx, struct SoundChip* chip) {
  struct Project* p = state->p;
  int ayChannel = 0;

  uint8_t mixer = 0;
  uint8_t noise = 0;
  uint8_t envShape = 0;
  uint16_t envPeriod = 0;

  for (int t = trackIdx; t < trackIdx + 3; t++) {
    struct PlaybackTrackState* track = &state->tracks[t];

    if (track->note.noteFinal == EMPTY_VALUE_8) {
      // Silence channel
      chip->setRegister(chip, 8 + ayChannel, 0);
    } else {
      int16_t period = p->pitchTable.values[track->note.noteFinal] - track->note.pitchOffset - track->note.pitchOffsetAcc;
      // TODO: Design decision: allow period overflow or not. Can it be a setting?
      //if (period < 0) period = 0;
      //if (period > 4095) period = 4095;
      chip->setRegister(chip, ayChannel * 2, period & 0xff);
      chip->setRegister(chip, ayChannel * 2 + 1, (period & 0xf00) >> 8);

      // Volume register
      int volume = 0;
      if (track->note.chip.ay.envShape != 0) {
        // Envelope on
        envShape = track->note.chip.ay.envShape;

        // Env period:
        if (track->note.chip.ay.envAutoN != 0) {
          // 1. Auto envelope
          int n = track->note.chip.ay.envAutoN;
          int d = track->note.chip.ay.envAutoD;
          int tempPeriod = period * n / d;
          envPeriod = ((tempPeriod & 0xf) >= 8) ? (tempPeriod >> 4) + 1 : (tempPeriod >> 4);
        } else {
          // 2. Manual envelope
          envPeriod = track->note.chip.ay.envBase;
        }

        // Envelope period modification
        envPeriod += track->note.chip.ay.envOffsetAcc;

        volume = 16;
      } else {
        // Envelope off
        volume = track->note.volume + track->note.volumeOffsetAcc;
        if (volume < 0) volume = 0;
        if (volume > 15) volume = 15;
        volume *= track->note.chip.ay.adsrVolume;

        // Instrument table volume
        int tableIdx = track->note.instrumentTable.tableIdx;
        if (tableIdx != EMPTY_VALUE_8 && p->tables[tableIdx].volumes[track->note.instrumentTable.rows[0]] != EMPTY_VALUE_8) {
          volume *= p->tables[tableIdx].volumes[track->note.instrumentTable.rows[0]];
        } else {
          volume *= 15;
        }

        // Aux table volume
        tableIdx = track->note.auxTable.tableIdx;
        if (tableIdx != EMPTY_VALUE_8 && p->tables[tableIdx].volumes[track->note.auxTable.rows[0]] != EMPTY_VALUE_8) {
        } else {
          volume *= 15;
        }

        // Normalize volume
        volume = volume / (15 * 15 * 15);

        // Sanity limits (even though it should never happen)
        if (volume < 0) volume = 0;
        if (volume > 15) volume = 15;
      }

      chip->setRegister(chip, 8 + ayChannel, volume);

      // Noise
      if ((track->note.chip.ay.mixer & 8) == 0 && track->note.chip.ay.noiseBase != EMPTY_VALUE_8) {
        noise = track->note.chip.ay.noiseBase + track->note.chip.ay.noiseOffsetAcc;
      }

      // Mixer
      mixer = mixer & (~(9 << ayChannel));
      mixer = mixer | ((track->note.chip.ay.mixer & 9) << ayChannel);
    }

    ayChannel++;
  }

  if (envShape != 0) {
    if (envShape != state->chips[chipIdx].ay.envShape) {
      chip->setRegister(chip, 13, envShape);
      state->chips[chipIdx].ay.envShape = envShape;
    }
    chip->setRegister(chip, 11, envPeriod & 0xff);
    chip->setRegister(chip, 12, (envPeriod & 0xff00) >> 8);
  }

  chip->setRegister(chip, 6, noise);
  chip->setRegister(chip, 7, mixer);
}
