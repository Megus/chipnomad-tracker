#include <playback.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
//
// AY-specific logic
//

void setupInstrumentAY(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  //struct Project* p = state->p;

  track->note.chip.ay.adsrCounter = 0;
  track->note.chip.ay.envBase = 0;
  track->note.chip.ay.envOffset = 0;
  track->note.chip.ay.noiseBase = 0;
  track->note.chip.ay.noiseOffset = 0;
  track->note.chip.ay.mixer = 1;
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

void outputRegistersAY(struct PlaybackState* state, int trackIdx, struct SoundChip* chip) {
  struct Project* p = state->p;
  int ayChannel = 0;

  uint8_t mixer = 0;

  for (int t = trackIdx; t < trackIdx + 3; t++) {
    struct PlaybackTrackState* track = &state->tracks[t];

    if (track->note.noteFinal == EMPTY_VALUE_8) {
      // Silence channel
      chip->setRegister(chip, 8 + ayChannel, 0);
    } else {
      int16_t period = p->pitchTable.values[track->note.noteFinal] - track->note.pitchOffset;
      // TODO: Design decision: allow period overflow or not. Can it be a setting?
      //if (period < 0) period = 0;
      //if (period > 4095) period = 4095;
      chip->setRegister(chip, ayChannel * 2, period & 0xff);
      chip->setRegister(chip, ayChannel * 2 + 1, (period & 0xf00) >> 8);

      // Volume
      int volume = track->note.volume * track->note.chip.ay.adsrVolume;

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

      chip->setRegister(chip, 8 + ayChannel, volume);

      // Mixer
      mixer = mixer & (~(9 << ayChannel));
      mixer = mixer | (8 << ayChannel);
    }

    ayChannel++;
  }

  chip->setRegister(chip, 7, mixer);
}
