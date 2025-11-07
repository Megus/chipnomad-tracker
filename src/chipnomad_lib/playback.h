#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

#include <project.h>
#include <chips.h>
#include <playback_fx.h>

enum PlaybackMode {
  playbackModeNone, // For queue
  playbackModeStopped,
  playbackModeSong,
  playbackModeChain,
  playbackModePhrase,
  playbackModePhraseRow,
  playbackModeLoop,
};

struct PlaybackTableState {
  uint8_t tableIdx;
  uint8_t rows[4];
  uint8_t counters[4];
  uint8_t speed[4];
  struct PlaybackFXState fx[4];
};

struct PlaybackAYNoteState {
  uint8_t mixer; // bit 0 - Tone, bit 1 - Noise, bit 2 - Envelope
  uint8_t adsrStep;
  uint8_t adsrCounter;
  uint8_t adsrFrom;
  uint8_t adsrTo;
  uint8_t adsrVolume;
  uint8_t envAutoN;
  uint8_t envAutoD;
  uint8_t envShape;
  uint16_t envBase;
  int16_t envOffset;
  int16_t envOffsetAcc;
  uint8_t noiseBase;
  int8_t noiseOffsetAcc;
};

union PlaybackChipNoteState {
  struct PlaybackAYNoteState ay;
};

struct PlaybackNoteState {
  uint8_t noteBase;
  uint8_t instrument;
  uint8_t volume;

  uint8_t noteFinal; // Calculated value
  int8_t noteOffset; // Re-calculated each frame
  int8_t noteOffsetAcc; // Accumulated over time
  int16_t pitchOffset; // Re-calculated each frame
  int16_t pitchOffsetAcc; // Accumulated over time
  uint8_t volume1; // Instrument volume
  uint8_t volume2; // Instrument table volume
  uint8_t volume3; // Aux table volume
  int8_t volumeOffsetAcc; // Accumulated over time

  struct PlaybackTableState instrumentTable;
  struct PlaybackTableState auxTable;
  struct PlaybackFXState fx[3];

  union PlaybackChipNoteState chip;
};

struct PlaybackTrackQueue {
  enum PlaybackMode mode;
  int songRow;
  int chainRow;
  int phraseRow;
};

enum PlaybackArpType {
  arpTypeUp,
  arpTypeDown,
  arpTypeUpDown,
  arpTypeUp1Oct,
  arpTypeDown1Oct,
  arpTypeUpDown1Oct,
  arpTypeUp2Oct,
  arpTypeDown2Oct,
  arpTypeUpDown2Oct,
  arpTypeUp3Oct,
  arpTypeDown3Oct,
  arpTypeUpDown3Oct,
  arpTypeUp4Oct,
  arpTypeDown4Oct,
  arpTypeUpDown4Oct,
  arpTypeUp5Oct,
  arpTypeMax,
};

struct PlaybackTrackState {
  struct PlaybackTrackQueue queue;

  enum PlaybackMode mode;
  // Position in the song
  int songRow;
  int chainRow;
  int phraseRow;

  // Groove
  uint8_t grooveIdx;
  int grooveRow;

  int frameCounter;

  int arpSpeed;
  enum PlaybackArpType arpType;

  // Currently playing note
  struct PlaybackNoteState note;

  // Cached phrase row data
  struct PhraseRow currentPhraseRow;
};

struct PlaybackAYChipState {
  uint8_t envShape;
};

union PlaybackChipState {
  struct PlaybackAYChipState ay;
};

struct PlaybackState {
  struct Project* p;
  struct PlaybackTrackState tracks[PROJECT_MAX_TRACKS];
  union PlaybackChipState chips[PROJECT_MAX_CHIPS];
};


/**
 * Initializes the playback state with the given project
 *
 * @param state Pointer to the playback state to initialize
 * @param project Pointer to the project data to use for playback
 */
void playbackInit(struct PlaybackState* state, struct Project* project);

/**
 * Checks if any track is currently playing
 *
 * @param state Pointer to the playback state
 * @return 1 if any track is playing, 0 if all tracks are stopped
 */
int playbackIsPlaying(struct PlaybackState* state);

/**
 * Starts song playback from the specified position
 *
 * @param state Pointer to the playback state
 * @param songRow Starting row position in the song
 * @param chainRow Starting row position in the chain
 */
void playbackStartSong(struct PlaybackState* state, int songRow, int chainRow);

/**
 * Starts chain playback for a specific track
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to play
 * @param songRow Row position in the song containing the chain
 * @param chainRow Starting row position in the chain
 */
void playbackStartChain(struct PlaybackState* state, int trackIdx, int songRow, int chainRow);

/**
 * Starts phrase playback for a specific track
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to play
 * @param songRow Row position in the song containing the phrase
 * @param chainRow Row position in the chain containing the phrase
 */
void playbackStartPhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow);

/**
 * Starts playback of a phrase row
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to play
 * @param phraseRow Phrase row data to play
 */
void playbackStartPhraseRow(struct PlaybackState* state, int trackIdx, struct PhraseRow* phraseRow);

/**
 * Queues a phrase for playback on a specific track
 * Only works if the track is currently in phrase playback mode
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to queue the phrase on
 * @param songRow Row position in the song containing the phrase
 * @param chainRow Row position in the chain containing the phrase
 */
void playbackQueuePhrase(struct PlaybackState* state, int trackIdx, int songRow, int chainRow);

/**
 * Stops playback on all tracks
 *
 * @param state Pointer to the playback state
 */
void playbackStop(struct PlaybackState* state);

/**
 * Plays a single note with an instrument for preview
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to use for preview
 * @param note Note value to play
 * @param instrument Instrument to use
 */
void playbackPreviewNote(struct PlaybackState* state, int trackIdx, uint8_t note, uint8_t instrument);



/**
 * Stops preview playback on a specific track
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to stop preview on
 */
void playbackStopPreview(struct PlaybackState* state, int trackIdx);

/**
 * Advances playback by one frame
 *
 * @param state Pointer to the playback state
 * @param chips Array of sound chips to output to
 * @return 1 if all tracks have finished playing, 0 if any track is still active
 */
int playbackNextFrame(struct PlaybackState* state, struct SoundChip* chips);




#endif
