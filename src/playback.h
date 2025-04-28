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
  uint8_t tableIdx;
  struct PlaybackFXState fx[4];
};

struct PlaybackAYNoteState {
  uint8_t mixer; // bit 0 - Tone, bit 1 - Noise, bit 2 - Envelope
  uint8_t tonePeriod;
  uint8_t adsrStep;
  uint8_t adsrCounter;
  uint8_t adsrFrom;
  uint8_t adsrTo;
  uint16_t envBase;
  uint16_t envOffset;
  uint8_t noiseBase;
  uint8_t noiseOffset;
};

union PlaybackChipNoteState {
  struct PlaybackAYNoteState ay;
};

struct PlaybackNoteState {
  uint8_t noteBase;
  uint8_t instrument;
  uint8_t volume;

  uint8_t noteFinal; // Calculated value
  int8_t noteOffset; // Calculated when processing FX
  int16_t pitchOffset; // Calculated when processing FX

  int8_t volumeOffset; // Calculated when processing FX

  struct PlaybackTableState instrumentTable;
  struct PlaybackTableState auxTable;
  struct PlaybackFXState fx[3];
  union PlaybackChipNoteState chip;
};

struct PlaybackTrackState {
  // Position in the song
  int songRow;
  int chainRow;
  int phraseRow;

  // Groove
  uint8_t grooveIdx;
  int grooveRow;

  int frameCounter;

  // Currently playing note
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
