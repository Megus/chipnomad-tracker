#include <playback.h>
#include <stdio.h>

static int globalFrameCounter = 0;

static void resetTrack(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  track->songRow = EMPTY_VALUE_16;
  track->chainRow = 0;
  track->phraseRow = 0;
  track->frameCounter = 0;
  track->grooveIdx = 0;
  track->grooveRow = 0;
  track->note.baseNote = EMPTY_VALUE_8;
  track->note.finalNote = EMPTY_VALUE_8;
  track->note.noteOffset = 0;
  track->note.fineOffset = 0;
  track->note.instrument = EMPTY_VALUE_8;
  track->note.volume = 15; // TODO: This is for AY only
}

static void processPhraseRow(struct PlaybackState* state, int trackIdx) {
  if (state->mode == playbackModeStopped) return;

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
  if (track->songRow == EMPTY_VALUE_16 || track->note.baseNote == EMPTY_VALUE_8) {
    track->note.finalNote = EMPTY_VALUE_8;
    chip->setRegister(chip, 8 + ayChannel, 0);  // Silence channel
    return;
  }

  // Tone period
  uint8_t phraseTranspose = p->chains[p->song[track->songRow][trackIdx]].transpose[track->chainRow];

  uint8_t note = track->note.baseNote + track->note.noteOffset + phraseTranspose;
  if (note >= p->pitchTable.length) note -= p->pitchTable.length;
  track->note.finalNote = note;

  int16_t period = p->pitchTable.values[note] + track->note.fineOffset;
  if (period < 0) period = 0;
  if (period > 4095) period = 4095;
  chip->setRegister(chip, ayChannel * 2, period & 0xff);
  chip->setRegister(chip, ayChannel * 2 + 1, (period & 0xf00) >> 8);

  // Volume
  chip->setRegister(chip, 8 + ayChannel, track->note.volume);

  if (track->note.volume > 0 && (globalFrameCounter & 1)) track->note.volume--;

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

void playbackInit(struct PlaybackState* state, struct Project* project) {
  state->p = project;

  for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
    resetTrack(state, c);
  }
}

void playbackStartSong(struct PlaybackState* state, int songRow, int chainRow) {
  if (state->mode != playbackModeStopped) return;
  int started = 0;

  struct Project* p = state->p;
  for (int trackIdx = 0; trackIdx < p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];
    resetTrack(state, trackIdx);

    // Regular setup
    if (p->song[songRow][trackIdx] == EMPTY_VALUE_16 || p->chains[p->song[songRow][trackIdx]].phrases[chainRow] == EMPTY_VALUE_16) {
      // No chain value or empty chain
      track->songRow = EMPTY_VALUE_16;
    } else {
      track->songRow = songRow;
      track->chainRow = chainRow;
      track->phraseRow = -1;
      started = 1;
    }
  }

  if (started) {
    state->mode = playbackModeSong;
  }
}

void playbackStartChain(struct PlaybackState* state, int trackIdx, int songRow, int chainRow) {
  if (state->mode != playbackModeStopped) return;
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  resetTrack(state, trackIdx);
  track->songRow = songRow;
  track->chainRow = chainRow;
  track->phraseRow = -1;
  state->mode = playbackModeChain;
}

void playbackStartPhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow) {
  if (state->mode != playbackModeStopped) return;
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  resetTrack(state, trackIdx);
  track->songRow = songRow;
  track->chainRow = chainRow;
  state->queuedChainRow = chainRow;
  track->phraseRow = -1;
  state->mode = playbackModePhrase;
}

void playbackStartPhraseRow(struct PlaybackState* state, int trackIdx, int songRow, int chainRow, int phraseRow) {
  if (!(state->mode == playbackModePhraseRow || state->mode == playbackModeStopped)) return;
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  resetTrack(state, trackIdx);
  track->songRow = songRow;
  track->chainRow = chainRow;
  track->phraseRow = phraseRow;
  state->mode = playbackModePhraseRow;
  processPhraseRow(state, trackIdx);
}

void playbackQueuePhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow) {
  if (state->mode != playbackModePhrase) return;
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  if (track->songRow != songRow) return;
  state->queuedChainRow = chainRow;
}

void playbackStop(struct PlaybackState* state) {
  for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
    state->tracks[c].songRow = EMPTY_VALUE_16;
    state->tracks[c].note.baseNote = EMPTY_VALUE_8;
  }
}

int playbackNextFrame(struct PlaybackState* state, struct SoundChip* chips) {
  globalFrameCounter++;

  struct Project* p = state->p;
  int hasActiveTracks = 0;

  for (int trackIdx = 0; trackIdx < state->p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];
    int chipIdx = 0; // TODO: Multichip setups

    // Don't do any playhead movement for phrase row
    if (state->mode != playbackModePhraseRow && track->songRow != EMPTY_VALUE_16) {
      track->frameCounter--;
      // Go to the next phrase row
      if (track->frameCounter <= 0) {
        track->phraseRow++;
        if (track->phraseRow >= 16) {
          track->phraseRow = 0;
          // Play mode logic:
          // Song playback
          if (state->mode == playbackModeSong) {
            // Next chain row
            int chain = p->song[track->songRow][trackIdx];
            int chainRow = track->chainRow;
            chainRow++;
            if (chainRow >= 16 || p->chains[chain].phrases[chainRow] == EMPTY_VALUE_16) {
              // Next song row
              int songRow = track->songRow;
              songRow++;
              track->chainRow = 0;
              if (songRow >= PROJECT_MAX_LENGTH || p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
                while (songRow > 0) {
                  songRow--;
                  if (p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
                    songRow++;
                    break;
                  }
                }
              }
              if (p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
                // No chains, stop track playback
                resetTrack(state, trackIdx);
              } else {
                track->songRow = songRow;
              }
            } else {
              track->chainRow = chainRow;
            }
          }
          // Chain playback
          else if (state->mode == playbackModeChain) {
            int chain = p->song[track->songRow][trackIdx];
            int chainRow = track->chainRow;
            chainRow++;
            if (chainRow >= 16 || p->chains[chain].phrases[chainRow] == EMPTY_VALUE_16) chainRow = 0;
            if (p->chains[chain].phrases[chainRow] == EMPTY_VALUE_16) {
              // Empty chain, stop playback
              resetTrack(state, trackIdx);
              state->mode = playbackModeStopped;
            } else {
              track->chainRow = chainRow;
            }
          }
          // Phrase playback
          else if (state->mode == playbackModePhrase) {
            track->chainRow = state->queuedChainRow;
          }
        }
        processPhraseRow(state, trackIdx);
      }
    }

    nextFrameAY(state, trackIdx, trackIdx, chips + chipIdx);

    // Check if the track is still playing something
    if (track->songRow != EMPTY_VALUE_16) hasActiveTracks = 1;
  }

  if (!hasActiveTracks) {
    state->mode = playbackModeStopped;
    return 1;
  }

  return 0;
}
