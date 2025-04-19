#include <playback.h>
#include <stdio.h>

// Public interface
int playbackInit(struct PlaybackState* state, struct Project* project) {
  state->p = project;
  state->isPlaying = 0;
  for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
    state->tracks[c].chainRow = 0;
    state->tracks[c].frameCounter = 0;
    state->tracks[c].grooveIdx = 0;
    state->tracks[c].grooveRow = 0;
    state->tracks[c].phraseRow = 0;
    state->tracks[c].songRow = 0;
    state->tracks[c].note.baseNote = EMPTY_VALUE_8;
    state->tracks[c].note.fineOffset = 0;
    state->tracks[c].note.noteOffset = 0;
    state->tracks[c].note.instrument = EMPTY_VALUE_8;
    state->tracks[c].note.volume = 0;
  }

  return 0;
}

int playbackStartSong(struct PlaybackState* state, int songRow, int chainRow) {
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

  return 0;
}

int playbackNextFrame(struct PlaybackState* state) {
  printf("cb\n");

  return 0;
}
