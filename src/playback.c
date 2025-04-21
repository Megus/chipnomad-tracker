#include <playback.h>
#include <stdio.h>

static void processPhraseRow(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  // If nothing is playing, skip it
  if (track->songRow == EMPTY_VALUE_16) return;

  uint16_t chainIdx = p->song[track->songRow][trackIdx];
  if (chainIdx != EMPTY_VALUE_16) {
    uint16_t phraseIdx = p->chains[chainIdx].phrases[track->chainRow];
    if (phraseIdx != EMPTY_VALUE_16) {
      int phraseRow = track->phraseRow;
      struct Phrase* phrase = &p->phrases[phraseIdx];
      // Note
      uint8_t note = phrase->notes[phraseRow];
      if (note != EMPTY_VALUE_8) {
        if (note == NOTE_OFF) {
          // TODO: Note off logic
          track->note.baseNote = EMPTY_VALUE_8;
        } else {
          track->note.baseNote = note;
          track->note.fineOffset = 0;
          track->note.noteOffset = 0;
          //track->note.auxTable - reset aux table;
          uint8_t instrument = phrase->instruments[phraseRow];
          if (instrument != EMPTY_VALUE_8) {
            track->note.instrument = instrument;
            // track->note.instrumentTable - reset instrument table
          }
          uint8_t volume = phrase->volumes[phraseRow];
          if (instrument != EMPTY_VALUE_8) {
            track->note.volume = volume;
          }
        }
      }

      // FX

      // Get next value from the groove
      // EDGE CASE: groove value = 0 (skip rows until a )
      // EDGE CASE: completely empty groove
      uint8_t grooveValue = p->grooves[track->grooveIdx].speed[track->grooveRow];
      track->frameCounter = grooveValue;

      // Go to the next groove row, loop over
      track->grooveRow++;
      if (track->grooveRow == 16 || p->grooves[track->grooveIdx].speed[track->grooveRow] == EMPTY_VALUE_8) {
        track->grooveRow = 0;
      }
    }
  }
}

static void nextFrameAY(struct PlaybackState* state, int trackIdx, int ayChannel, struct SoundChip* chip) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  // Is the channel playing?
  if (track->note.baseNote == EMPTY_VALUE_8) {
    chip->setRegister(chip, 8 + ayChannel, 0);  // Silence channel
  }

  // Tone period
  uint8_t note = track->note.baseNote + track->note.noteOffset;
  if (note >= p->pitchTable.length) note -= p->pitchTable.length;
  int16_t period = p->pitchTable.values[note] + track->note.fineOffset;
  if (period < 0) period = 0;
  if (period > 4095) period = 4095;
  chip->setRegister(chip, ayChannel * 2, period & 0xff);
  chip->setRegister(chip, ayChannel * 2 + 1, (period & 0xf00) >> 8);

  // Volume
  chip->setRegister(chip, 8 + ayChannel, track->note.volume);

  // Mixer
  uint8_t mixer = chip->regs[7];
  mixer = mixer & (~(9 << ayChannel));
  mixer = mixer | (8 << ayChannel); // Enable noise
  chip->setRegister(chip, 7, mixer);
}


///////////////////////////////////////////////////////////////////////////////
//
// Public interface
//

int playbackInit(struct PlaybackState* state, struct Project* project) {
  state->p = project;
  for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
    state->tracks[c].songRow = EMPTY_VALUE_16;
    state->tracks[c].chainRow = 0;
    state->tracks[c].phraseRow = 0;
    state->tracks[c].frameCounter = 0;
    state->tracks[c].grooveIdx = 0;
    state->tracks[c].grooveRow = 0;
    state->tracks[c].note.baseNote = EMPTY_VALUE_8;
    state->tracks[c].note.noteOffset = 0;
    state->tracks[c].note.fineOffset = 0;
    state->tracks[c].note.instrument = EMPTY_VALUE_8;
    state->tracks[c].note.volume = 15; // TODO: This is for AY only
  }

  return 0;
}

int playbackIsPlaying(struct PlaybackState* state) {
  for (int c = 0; c < state->p->tracksCount; c++) {
    if (state->tracks[c].songRow != EMPTY_VALUE_16) return 1;
  }

  return 0;
}

int playbackStartSong(struct PlaybackState* state, int songRow, int chainRow) {
  if (playbackIsPlaying(state)) return 1;
  printf("Start: %d %d\n", songRow, chainRow);

  struct Project* p = state->p;
  for (int trackIdx = 0; trackIdx < p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];
    // As we're starting, resetting the groove
    track->grooveIdx = 0;
    track->grooveRow = 0;

    // Regular setup
    if (p->song[songRow][trackIdx] == EMPTY_VALUE_16 || p->chains[p->song[songRow][trackIdx]].phrases[0] == EMPTY_VALUE_16) {
      // No chain value or empty chain
      track->songRow = EMPTY_VALUE_16;
    } else {
      track->songRow = songRow;
    }
    track->chainRow = 0;
    track->phraseRow = 0;
    // TODO: Reset notes, FX, etc in the track

    processPhraseRow(state, trackIdx);
  }

  return 0;
}

int playbackStartChain(struct PlaybackState* state, int track, int songRow) {
  return 0;
}

int playbackStartPhrase(struct PlaybackState* state, int track, int songRow, int chainRow) {

  return 0;
}

int playbackStartNote(struct PlaybackState* state, int track, int phrase, int phraseRow) {

  return 0;
}

int playbackQueuePhrase(struct PlaybackState* state, int track, int songRow, int chainRow) {

  return 0;
}

int playbackStop(struct PlaybackState* state) {
  printf("Stop\n");

  return 0;
}

int playbackNextFrame(struct PlaybackState* state, struct SoundChip* chips) {
  struct Project* p = state->p;

  for (int trackIdx = 0; trackIdx < state->p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];
    int chipIdx = 0; // TODO: Multichip setups
    nextFrameAY(state, trackIdx, trackIdx, chips + chipIdx);
    track->frameCounter--;
    if (track->frameCounter <= 0) {
      track->phraseRow++;
      if (track->phraseRow == 16) {
        track->phraseRow = 0;
      }
      processPhraseRow(state, trackIdx);
    }
  }

  return 0;
}
