#include <playback.h>
#include <stdio.h>
#include <string.h>
#include <playback_internal.h>

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
  track->note.noteOffsetAcc = 0;
  track->note.pitchOffset = 0;
  track->note.pitchOffsetAcc = 0;
  track->note.instrument = EMPTY_VALUE_8;
  track->note.volume = 0;
  track->note.volumeOffsetAcc = 0;
  track->note.volume1 = 0;
  track->note.volume2 = 0;
  track->note.volume3 = 0;
  track->note.instrumentTable.tableIdx = EMPTY_VALUE_8;
  track->note.auxTable.tableIdx = EMPTY_VALUE_8;
  track->mode = playbackModeStopped;

  for (int i = 0; i < 3; i++) {
    track->note.fx[i].fx = EMPTY_VALUE_8;
  }

  // Clear cached phrase row
  memset(&track->currentPhraseRow, EMPTY_VALUE_8, sizeof(track->currentPhraseRow));

  resetTrackAY(state, trackIdx);
}

void tableInit(struct PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int tableIdx, int speed) {
  table->tableIdx = tableIdx;
  if (tableIdx == EMPTY_VALUE_8) return;

  for (int i = 0; i < 4; i++) {
    table->counters[i] = 0;
    table->rows[i] = 0;
    table->speed[i] = speed;

    tableReadFX(state, trackIdx, table, i, 1);
  }
}

void tableReadFX(struct PlaybackState* state, int trackIdx, struct PlaybackTableState* table, int fxIdx, int forceRead) {
  uint8_t tableIdx = table->tableIdx;
  if (tableIdx == EMPTY_VALUE_8) return;
  struct Project* p = state->p;

  int tableRow = table->rows[fxIdx];
  if (forceRead || p->tables[tableIdx].rows[tableRow].fx[fxIdx][0] != EMPTY_VALUE_8) {
    initFX(state, trackIdx, p->tables[tableIdx].rows[tableRow].fx[fxIdx], &table->fx[fxIdx], 0);
  }
}

static void tableProgress(struct PlaybackState* state, int trackIdx, struct PlaybackTableState* table) {
  if (table->tableIdx == EMPTY_VALUE_8) return;
  struct Project* p = state->p;

  for (int i = 0; i < 4; i++) {
    table->counters[i]++;

    if (table->counters[i] >= table->speed[i]) {
      table->counters[i] = 0;

      uint8_t fxType = p->tables[table->tableIdx].rows[table->rows[i]].fx[i][0];
      uint8_t fxValue = p->tables[table->tableIdx].rows[table->rows[i]].fx[i][1];

      // Special case for THO/HOP pointing to the same row - we should not progress further
      if (fxType == fxTHO && (fxValue & 0xf) == table->rows[i]) {
        for (int c = 0; c < 4; c++) {
          table->counters[c] = 0;
          table->rows[c] = fxValue & 0xf;
          tableReadFX(state, trackIdx, table, c, 0);
        }
        break;
      } else if (fxType == fxHOP && (fxValue & 0xf) == table->rows[i]) {
        // Do nothing here, we just don't progres further and stay on the same row
      } else {
        // Progres further in the table
        table->rows[i] = (table->rows[i] + 1) & 15;

        // Handle THO and HOP table FX right nere
        fxType = p->tables[table->tableIdx].rows[table->rows[i]].fx[i][0];
        fxValue = p->tables[table->tableIdx].rows[table->rows[i]].fx[i][1];
        if (fxType == fxTHO) {
          // Hop on all FX lanes
          for (int c = 0; c < 4; c++) {
            table->counters[c] = 0;
            table->rows[c] = fxValue & 0xf;
            tableReadFX(state, trackIdx, table, c, 0);
          }
          break;
        } else if (fxType == fxHOP) {
          // Hop only on the current lane
          table->rows[i] = fxValue & 0xf;
        }
      }
      tableReadFX(state, trackIdx, table, i, 0);
    }
  }
}

void handleNoteOff(struct PlaybackState* state, int trackIdx) {
  noteOffInstrumentAY(state, trackIdx);
}



void readPhraseRowDirect(struct PlaybackState* state, int trackIdx, struct PhraseRow* phraseRow, int skipDelCheck) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;
  uint8_t note = phraseRow->note;

  // Pre-scan FX if there's a DEL FX
  if (!skipDelCheck) {
    for (int i = 0; i < 3; i++) {
      if (phraseRow->fx[i][0] == fxDEL && phraseRow->fx[i][1] != 0) {
        track->note.fx[i].fx = fxDEL;
        track->note.fx[i].value = phraseRow->fx[i][1];
        return;
      }
    }
  }

  // FX
  for (int i = 0; i < 3; i++) {
    if (phraseRow->fx[i][0] != EMPTY_VALUE_8 || note != EMPTY_VALUE_8) {
      initFX(state, trackIdx, phraseRow->fx[i], &track->note.fx[i], (note != EMPTY_VALUE_8 && note != NOTE_OFF));
    }
  }

  // Note
  if (note != EMPTY_VALUE_8) {
    if (note == NOTE_OFF) {
      handleNoteOff(state, trackIdx);
    } else {
      track->note.noteBase = note;
      track->note.pitchOffsetAcc = 0;
      track->note.noteOffsetAcc = 0;
      track->note.volumeOffsetAcc = 0;
      tableInit(state, trackIdx, &track->note.auxTable, EMPTY_VALUE_8, 1);
    }
  }

  // Instrument
  uint8_t instrument = phraseRow->instrument;
  if (instrument != EMPTY_VALUE_8) {
    track->note.instrument = instrument;
    setupInstrumentAY(state, trackIdx);
    tableInit(state, trackIdx, &track->note.instrumentTable, instrument, p->instruments[instrument].tableSpeed);
  }

  // Volume
  uint8_t volume = phraseRow->volume;
  if (phraseRow->volume != EMPTY_VALUE_8) {
    track->note.volume = volume;
  }
}

void readPhraseRow(struct PlaybackState* state, int trackIdx, int skipDelCheck) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  // If using phrase row mode, use cached phrase row
  if (track->mode == playbackModePhraseRow) {
    readPhraseRowDirect(state, trackIdx, &track->currentPhraseRow, skipDelCheck);
    return;
  }

  // If nothing is playing, skip it
  if (track->mode == playbackModeStopped || track->songRow == EMPTY_VALUE_16) return;

  uint16_t chainIdx = p->song[track->songRow][trackIdx];
  if (chainIdx != EMPTY_VALUE_16) {
    uint16_t phraseIdx = p->chains[chainIdx].rows[track->chainRow].phrase;
    if (phraseIdx != EMPTY_VALUE_16) {
      int phraseRow = track->phraseRow;
      struct Phrase* phrase = &p->phrases[phraseIdx];
      readPhraseRowDirect(state, trackIdx, &phrase->rows[phraseRow], skipDelCheck);
    } else {
      // Safeguard for phrase in chain
      resetTrack(state, trackIdx);
    }
  } else {
    // Safeguard for chain in song
    resetTrack(state, trackIdx);
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

  // Clear re-calculated fields
  track->note.noteOffset = 0;
  track->note.pitchOffset = 0;
  track->note.chip.ay.envOffset = 0;

  // FX
  handleFX(state, trackIdx);

  // Instrument
  enum InstrumentType instType = (track->note.instrument != EMPTY_VALUE_8) ? p->instruments[track->note.instrument].type : instNone;
  switch (instType) {
    case instAY:
      handleInstrumentAY(state, trackIdx);
      break;
    case instNone:
      break;
  }

  // Final note calculation
  if (track->note.noteBase == EMPTY_VALUE_8) {
    track->note.noteFinal = EMPTY_VALUE_8;
  } else {
    // Base note
    int16_t note = track->note.noteBase;

    // Tables
    int tableIdx = track->note.instrumentTable.tableIdx;
    if (tableIdx != EMPTY_VALUE_8) {
      if (p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].pitchFlag) {
        note = p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].pitchOffset;
      } else {
        note += (int8_t)(p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].pitchOffset);
      }
    }

    tableIdx = track->note.auxTable.tableIdx;
    if (tableIdx != EMPTY_VALUE_8) {
      if (p->tables[tableIdx].rows[track->note.auxTable.rows[0]].pitchFlag) {
        note = p->tables[tableIdx].rows[track->note.auxTable.rows[0]].pitchOffset;
      } else {
        note += (int8_t)(p->tables[tableIdx].rows[track->note.auxTable.rows[0]].pitchOffset);
      }
    }

    // Phrase transpose
    if (track->note.instrument != EMPTY_VALUE_8 && p->instruments[track->note.instrument].transposeEnabled) {
      note += (int8_t)p->chains[p->song[track->songRow][trackIdx]].rows[track->chainRow].transpose;
    }

    // Offset from FX
    note += track->note.noteOffset;
    note += track->note.noteOffsetAcc;

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

  // Reset OFF/KIL/DEL FX
  for (int c = 0; c < 3; c++) {
    if (track->note.fx[c].fx == fxOFF || track->note.fx[c].fx == fxKIL || track->note.fx[c].fx == fxDEL) {
      track->note.fx[c].fx = EMPTY_VALUE_8;
    }
  }

  if (track->phraseRow >= 16) {
    track->phraseRow = 0;
    // Play mode logic:
    // Song playback
    if (track->mode == playbackModeSong) {
      // Next chain row
      int chain = p->song[track->songRow][trackIdx];
      if (chain != EMPTY_VALUE_16) {
        int chainRow = track->chainRow + 1;
        if (chainRow >= 16 || p->chains[chain].rows[chainRow].phrase == EMPTY_VALUE_16) {
          // Next song row
          int songRow = track->songRow + 1;
          track->chainRow = 0;
          if (songRow >= PROJECT_MAX_LENGTH || p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
            if (track->loop) {
              while (songRow > 0) {
                songRow--;
                if (p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
                  songRow++;
                  break;
                }
              }
            } else {
              songRow = -1;
            }
          }
          if (songRow < 0 || p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
            resetTrack(state, trackIdx);
            stopped = 1;
          } else {
            track->songRow = songRow;
          }
        } else {
          track->chainRow = chainRow;
        }
      } else {
        resetTrack(state, trackIdx);
      }
    }
    // Chain playback
    else if (track->mode == playbackModeChain) {
      int chain = p->song[track->songRow][trackIdx];
      int chainRow = track->chainRow + 1;
      if (chainRow >= 16 || p->chains[chain].rows[chainRow].phrase == EMPTY_VALUE_16) {
        chainRow = track->loop ? 0 : -1;
      }
      if (chainRow < 0 || p->chains[chain].rows[chainRow].phrase == EMPTY_VALUE_16) {
        resetTrack(state, trackIdx);
        stopped = 1;
      } else {
        track->chainRow = chainRow;
      }
    }
    // Phrase playback
    else if (track->mode == playbackModePhrase) {
      if (track->loop) {
        track->chainRow = track->queue.chainRow;
      } else {
        resetTrack(state, trackIdx);
        stopped = 1;
      }
    }
  }

  return stopped;
}

static int skipZeroGrooveRows(struct PlaybackState* state, int trackIdx) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  struct Project* p = state->p;

  int curGrooveRow = track->grooveRow;
  while (p->grooves[track->grooveIdx].speed[track->grooveRow] == 0) {
    moveToNextPhraseRow(state, trackIdx);
    track->grooveRow++;
    if (track->grooveRow == 16 || p->grooves[track->grooveIdx].speed[track->grooveRow] == EMPTY_VALUE_8) {
      track->grooveRow = 0;
    }
    if (track->grooveRow == curGrooveRow) {
      // All rows are zero, stop playback
      resetTrack(state, trackIdx);
      return 1;
    }
  }

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// Public interface
//

void playbackInit(struct PlaybackState* state, struct Project* project) {
  state->p = project;

  for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
    resetTrack(state, c);
    state->tracks[c].queue.mode = playbackModeNone;
    state->tracks[c].queue.loop = 0;
  }

  // TODO: Properly initialize other global chip states, but for now it's AY only
  for (int c = 0; c < PROJECT_MAX_CHIPS; c++) {
    state->chips[c].ay.envShape = 0;
  }
}

int playbackIsPlaying(struct PlaybackState* state) {
  for (int trackIdx = 0; trackIdx < state->p->tracksCount; trackIdx++) {
    if (state->tracks[trackIdx].mode != playbackModeStopped) return 1;
  }
  return 0;
}


void playbackStartSong(struct PlaybackState* state, int songRow, int chainRow, int loop) {
  if (playbackIsPlaying(state)) return;

  struct Project* p = state->p;

  for (int trackIdx = 0; trackIdx < p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];

    if (p->song[songRow][trackIdx] != EMPTY_VALUE_16 && p->chains[p->song[songRow][trackIdx]].rows[chainRow].phrase != EMPTY_VALUE_16) {
      track->queue.mode = playbackModeSong;
      track->queue.songRow = songRow;
      track->queue.chainRow = chainRow;
      track->queue.phraseRow = 0;
      track->queue.loop = loop;
    }
  }
}

void playbackStartChain(struct PlaybackState* state, int trackIdx, int songRow, int chainRow, int loop) {
  if (playbackIsPlaying(state)) return;

  struct Project* p = state->p;
  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  if (p->chains[p->song[songRow][trackIdx]].rows[chainRow].phrase != EMPTY_VALUE_16) {
    track->queue.mode = playbackModeChain;
    track->queue.songRow = songRow;
    track->queue.chainRow = chainRow;
    track->queue.phraseRow = 0;
    track->queue.loop = loop;
  }
}

void playbackStartPhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow, int loop) {
  if (playbackIsPlaying(state)) return;

  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  track->queue.mode = playbackModePhrase;
  track->queue.songRow = songRow;
  track->queue.chainRow = chainRow;
  track->queue.phraseRow = 0;
  track->queue.loop = loop;
}

void playbackStartPhraseRow(struct PlaybackState* state, int trackIdx, struct PhraseRow* phraseRow) {
  resetTrack(state, trackIdx);

  struct PlaybackTrackState* track = &state->tracks[trackIdx];

  // Set up phrase row playback
  track->queue.mode = playbackModePhraseRow;
  track->songRow = 0;
  track->currentPhraseRow = *phraseRow;
}

void playbackQueuePhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow) {
  struct PlaybackTrackState* track = &state->tracks[trackIdx];
  if (track->mode != playbackModePhrase) return;
  if (track->songRow != songRow) return;
  track->queue.mode = playbackModePhrase;
  track->queue.songRow = songRow;
  track->queue.chainRow = chainRow;
  track->queue.phraseRow = 0;
  track->queue.loop = track->loop;
}

void playbackStop(struct PlaybackState* state) {
  for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
    resetTrack(state, c);
    state->tracks[c].queue.mode = playbackModeNone;
  }
}

int playbackNextFrame(struct PlaybackState* state, struct SoundChip* chips) {
  struct Project* p = state->p;
  int hasActiveTracks = 0;

  for (int trackIdx = 0; trackIdx < state->p->tracksCount; trackIdx++) {
    struct PlaybackTrackState* track = &state->tracks[trackIdx];

    // Check queued play event for stopped track or when a track is in phrase row playback mode
    if ((track->mode == playbackModeStopped && track->queue.mode != playbackModeNone) ||
      (track->mode == playbackModePhraseRow && track->queue.mode == playbackModePhraseRow)) {
      track->mode = track->queue.mode;
      track->songRow = track->queue.songRow;
      track->chainRow = track->queue.chainRow;
      track->phraseRow = track->queue.phraseRow;
      track->loop = track->queue.loop;

      // "Consume" queued event
      track->queue.mode = playbackModeNone;

      skipZeroGrooveRows(state, trackIdx);
      readPhraseRow(state, trackIdx, 0);
    }
    // Advance further in the track
    else {
      tableProgress(state, trackIdx, &track->note.instrumentTable);
      tableProgress(state, trackIdx, &track->note.auxTable);

      // Don't do any playhead movement for phrase row
      if (track->mode != playbackModePhraseRow && track->songRow != EMPTY_VALUE_16) {
        uint8_t grooveValue = p->grooves[track->grooveIdx].speed[track->grooveRow];

        if (grooveValue == EMPTY_VALUE_8) {
          // The current groove row doesn't have a value, stop playback
          resetTrack(state, trackIdx);
        } else {
          track->frameCounter++;

          if (track->frameCounter >= grooveValue) {
            // Go to the next groove row
            track->grooveRow++;
            if (track->grooveRow == 16 || p->grooves[track->grooveIdx].speed[track->grooveRow] == EMPTY_VALUE_8) {
              track->grooveRow = 0;
            }

            // Go to the next phrase row
            track->frameCounter = 0;
            moveToNextPhraseRow(state, trackIdx);
            skipZeroGrooveRows(state, trackIdx);
            readPhraseRow(state, trackIdx, 0);
          }
        }
      }
    }

    nextFrame(state, trackIdx);

    // Check if the track is still playing something
    if (track->songRow == EMPTY_VALUE_16) {
      track->mode = playbackModeStopped;
    } else {
      hasActiveTracks = 1;
    }
  }

  // TODO: Multichip setup
  outputRegistersAY(state, 0, 0, chips);

  return !hasActiveTracks;
}

void playbackPreviewNote(struct PlaybackState* state, int trackIdx, uint8_t note, uint8_t instrument) {
  // Create a phrase row for preview
  struct PhraseRow phraseRow = {0};
  phraseRow.note = note;
  phraseRow.instrument = instrument;
  phraseRow.volume = 15;

  // Set up empty FX
  for (int i = 0; i < 3; i++) {
    phraseRow.fx[i][0] = EMPTY_VALUE_8;
    phraseRow.fx[i][1] = EMPTY_VALUE_8;
  }

  // Use unified phrase row playback
  playbackStartPhraseRow(state, trackIdx, &phraseRow);
}

void playbackStopPreview(struct PlaybackState* state, int trackIdx) {
  if (state->tracks[trackIdx].mode == playbackModePhraseRow) {
    resetTrack(state, trackIdx);
  }
}
