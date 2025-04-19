#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

#include <project.h>

struct PlaybackFXState {

};

struct PlaybackTableState {

};

struct PlaybackNoteState {
  uint8_t baseNote;
  uint8_t noteOffset;
  uint16_t fineOffset;

  uint8_t instrument;
  uint8_t volume;

  struct PlaybackTableState instrumentTable;
  struct PlaybackTableState auxTable;
  struct PlaybackFXState fx[3];
};

struct PlaybackTrackState {
  // Position in the song
  uint8_t grooveIdx;
  int songRow;
  int chainRow;
  int phraseRow;
  int grooveRow;
  int frameCounter;

  struct PlaybackNoteState note;
};

struct PlaybackState {
  struct Project* p;
  int isPlaying;

  struct PlaybackTrackState tracks[PROJECT_MAX_TRACKS];
};

int playbackInit(struct PlaybackState* state, struct Project* project);
int playbackStartSong(struct PlaybackState* state, int songRow, int chainRow);
int playbackStartChain(struct PlaybackState* state, int track, int songRow);
int playbackStartPhrase(struct PlaybackState* state, int track, int songRow, int chainRow);
int playbackStartNote(struct PlaybackState* state, int track, int phrase, int phraseRow);
int playbackQueuePhrase(struct PlaybackState* state, int track, int songRow, int chainRow);
int playbackStop(struct PlaybackState* state);

int playbackNextFrame(struct PlaybackState* state);

#endif
