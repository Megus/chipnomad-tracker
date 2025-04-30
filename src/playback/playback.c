#include <playback.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
//
// Common logic
//

static void resetTrack(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  track->songRow = EMPTY_VALUE_16;
  track->chainRow = 0;
  track->phraseRow = 0;
  track->frameCounter = 0;
  track->grooveIdx = 0;
  track->grooveRow = 0;
  track->note.noteBase = EMPTY_VALUE_8;
  track->note.noteFinal = EMPTY_VALUE_8;
  track->note.noteOffset = 0;
  track->note.pitchOffset = 0;
  track->note.instrument = EMPTY_VALUE_8;
  track->note.volume = 15; // TODO: This is for AY only
}

static void tableInit(struct PlaybackState* state, struct PlaybackTableState* table, int tableIdx, int speed) {
  table->tableIdx = tableIdx;
  if (tableIdx == EMPTY_VALUE_8) return;

  for (int c = 0; c < 4; c++) {
    table->counters[c] = 0;
    table->rows[c] = 0;
    table->speed[c] = speed;
  }
}

static void tableHandleFX(struct PlaybackState* state, int trackIdx, struct PlaybackTableState* table) {
  if (table->tableIdx == EMPTY_VALUE_8) return;

}

static void tableProgres(struct PlaybackState* state, int trackIdx, struct PlaybackTableState* table) {
  if (table->tableIdx == EMPTY_VALUE_8) return;

  for (int i = 0; i < 4; i++) {
    table->counters[i]++;

    if (table->counters[i] >= table->speed[i]) {
      table->counters[i] = 0;
      table->rows[i] = (table->rows[i] + 1) & 15;
    }
  }
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
          noteOffInstrumentAY(state, trackIdx);
        } else {
          track->note.noteBase = note;
          track->note.pitchOffset = 0;
          track->note.noteOffset = 0;
          tableInit(state, &track->note.auxTable, EMPTY_VALUE_8, 1);
       }
      }

      // Instrument
      uint8_t instrument = phrase->instruments[phraseRow];
      if (instrument != EMPTY_VALUE_8) {
        track->note.instrument = instrument;
        setupInstrumentAY(state, trackIdx);
        tableInit(state, &track->note.instrumentTable, instrument, p->instruments[instrument].tableSpeed);
      }

      // Volume
      uint8_t volume = phrase->volumes[phraseRow];
      if (phrase->volumes[phraseRow] != EMPTY_VALUE_8) {
        track->note.volume = volume;
      }

      // FX
    }
  }
}

static void nextFrame(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  // Is the channel playing?
  if (track->songRow == EMPTY_VALUE_16) {
    track->note.noteFinal = EMPTY_VALUE_8;
    return;
  }

  // Instrument
  handleInstrumentAY(state, trackIdx);

  // FX

  // Tables
  tableHandleFX(state, trackIdx, &track->note.instrumentTable);
  tableHandleFX(state, trackIdx, &track->note.auxTable);

  // Final note calculation
  if (track->note.noteBase == EMPTY_VALUE_8) {
    track->note.noteFinal = EMPTY_VALUE_8;
  } else {
    // Base note
    int16_t note = track->note.noteBase;

    // Tables
    int tableIdx = track->note.instrumentTable.tableIdx;
    if (tableIdx != EMPTY_VALUE_8) {
      if (p->tables[tableIdx].pitchFlags[track->note.instrumentTable.rows[0]]) {
        note = p->tables[tableIdx].pitchOffsets[track->note.instrumentTable.rows[0]];
      } else {
        note += (int8_t)(p->tables[tableIdx].pitchOffsets[track->note.instrumentTable.rows[0]]);
      }
    }

    tableIdx = track->note.auxTable.tableIdx;
    if (tableIdx != EMPTY_VALUE_8) {
      if (p->tables[tableIdx].pitchFlags[track->note.auxTable.rows[0]]) {
        note = p->tables[tableIdx].pitchOffsets[track->note.auxTable.rows[0]];
      } else {
        note += (int8_t)(p->tables[tableIdx].pitchOffsets[track->note.auxTable.rows[0]]);
      }
    }

    // Phrase transpose
    if (track->note.instrument != EMPTY_VALUE_8 && p->instruments[track->note.instrument].transposeEnabled) {
      note += (int8_t)p->chains[p->song[track->songRow][trackIdx]].transpose[track->chainRow];
    }

    // Offset from FX
    note += track->note.noteOffset;

    // TODO: Design decision: allow note overflow or not. Can it be a setting?
    if (note < 0) note = p->pitchTable.length + (note % p->pitchTable.length);
    if (note >= p->pitchTable.length) note = note % p->pitchTable.length;
    track->note.noteFinal = note;
  }
}

static int moveToNextPhraseRow(struct PlaybackState* state, int trackIdx) {
  int stopped = 0;
  struct Project *p = state->p;
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

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
          stopped = 1;
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
        stopped = 1;
      } else {
        track->chainRow = chainRow;
      }
    }
    // Phrase playback
    else if (state->mode == playbackModePhrase) {
      track->chainRow = state->queuedChainRow;
    }
  }

  return stopped;
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
    state->tracks[c].note.noteBase = EMPTY_VALUE_8;
  }
}

int playbackNextFrame(struct PlaybackState* state, struct SoundChip* chips) {
  struct Project* p = state->p;
  int hasActiveTracks = 0;

  for (int trackIdx = 0; trackIdx < state->p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];
    // Don't do any playhead movement for phrase row
    if (state->mode != playbackModePhraseRow && track->songRow != EMPTY_VALUE_16) {
      track->frameCounter--;
      // Go to the next phrase row
      if (track->frameCounter <= 0) {
        moveToNextPhraseRow(state, trackIdx);
        processPhraseRow(state, trackIdx);

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

    nextFrame(state, trackIdx);

    // Check if the track is still playing something
    if (track->songRow != EMPTY_VALUE_16) hasActiveTracks = 1;
  }

  // TODO: Multichip setup
  outputRegistersAY(state, 0, chips);

  // Progress forward
  for (int trackIdx = 0; trackIdx < state->p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];
    tableProgres(state, trackIdx, &track->note.instrumentTable);
    tableProgres(state, trackIdx, &track->note.auxTable);
  }

  if (!hasActiveTracks) {
    state->mode = playbackModeStopped;
    return 1;
  }

  return 0;
}
