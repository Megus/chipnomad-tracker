#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

#include <project.h>
#include <chips.h>

enum PlaybackMode {
  playbackModeStopped,
  playbackModeSong,
  playbackModeChain,
  playbackModePhrase,
  playbackModePhraseRow,
  playbackModeLoop,
};

struct PlaybackFXState {

};

struct PlaybackTableState {

};

struct PlaybackNoteState {
  uint8_t finalNote; // Calculated value
  uint8_t baseNote;
  uint8_t noteOffset;
  int16_t fineOffset;

  uint8_t instrument;
  uint8_t volume;

  struct PlaybackTableState instrumentTable;
  struct PlaybackTableState auxTable;
  struct PlaybackFXState fx[3];

  // TODO: Chip-specific state
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
  enum PlaybackMode mode;
  struct PlaybackTrackState tracks[PROJECT_MAX_TRACKS];
  uint16_t queuedChainRow; // For playbackModePhrase
};

void playbackInit(struct PlaybackState* state, struct Project* project);
void playbackStartSong(struct PlaybackState* state, int songRow, int chainRow);
void playbackStartChain(struct PlaybackState* state, int trackIdx, int songRow, int chainRow);
void playbackStartPhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow);
void playbackQueuePhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow);
void playbackStartPhraseRow(struct PlaybackState* state, int trackIdx, int songRow, int chainRow, int phraseRow);
void playbackStop(struct PlaybackState* state);

int playbackNextFrame(struct PlaybackState* state, struct SoundChip* chips);

#endif
