#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

#include "project.h"
#include "chips/chips.h"
#include "playback_fx.h"

enum PlaybackMode {
  playbackModeNone, // For queue
  playbackModeStopped,
  playbackModeSong,
  playbackModeChain,
  playbackModePhrase,
  playbackModePhraseRow,
  playbackModeLoop,
};

typedef struct PlaybackTableState {
  uint8_t tableIdx;
  uint8_t rows[4];
  uint8_t counters[4];
  uint8_t speed[4];
  PlaybackFXState fx[4];
} PlaybackTableState;

typedef struct PlaybackAYNoteState {
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
} PlaybackAYNoteState;

typedef struct PlaybackChipNoteState {
  PlaybackAYNoteState ay;
} PlaybackChipNoteState;

typedef struct PlaybackNoteState {
  uint8_t noteBase;
  uint8_t instrument;
  uint8_t volume;

  uint8_t noteFinal; // Calculated value
  int8_t noteOffset; // Re-calculated each frame
  int8_t noteOffsetAcc; // Accumulated over time
  int16_t pitchOffset; // Re-calculated each frame
  int16_t pitchOffsetAcc; // Accumulated over time
  int16_t periodOffsetAcc; // Accumulated period offset
  uint8_t volume1; // Instrument volume
  uint8_t volume2; // Instrument table volume
  uint8_t volume3; // Aux table volume
  int8_t volumeOffsetAcc; // Accumulated over time

  PlaybackTableState instrumentTable;
  PlaybackTableState auxTable;
  PlaybackFXState fx[3];

  PlaybackChipNoteState chip;
} PlaybackNoteState;

typedef struct PlaybackTrackQueue {
  enum PlaybackMode mode;
  int songRow;
  int chainRow;
  int phraseRow;
  int loop;
} PlaybackTrackQueue;

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

typedef struct PlaybackTrackState {
  PlaybackTrackQueue queue;

  enum PlaybackMode mode;
  // Position in the song
  int songRow;
  int chainRow;
  int phraseRow;
  int loop;

  // Groove
  uint8_t grooveIdx;
  int grooveRow;
  uint8_t pendingGrooveIdx; // For GGR synchronization

  int frameCounter;

  int arpSpeed;
  enum PlaybackArpType arpType;

  // Currently playing note
  PlaybackNoteState note;

  // Cached phrase row data
  PhraseRow currentPhraseRow;
} PlaybackTrackState;

typedef struct PlaybackAYChipState {
  uint8_t envShape;
} PlaybackAYChipState;

typedef struct PlaybackChipState {
  PlaybackAYChipState ay;
} PlaybackChipState;

typedef struct PlaybackState {
  Project* p;
  PlaybackTrackState tracks[PROJECT_MAX_TRACKS];
  PlaybackChipState chips[PROJECT_MAX_CHIPS];
  uint8_t trackEnabled[PROJECT_MAX_TRACKS];
} PlaybackState;


/**
 * Initializes the playback state with the given project
 *
 * @param state Pointer to the playback state to initialize
 * @param project Pointer to the project data to use for playback
 */
void playbackInit(PlaybackState* state, Project* project);

/**
 * Checks if any track is currently playing
 *
 * @param state Pointer to the playback state
 * @return 1 if any track is playing, 0 if all tracks are stopped
 */
int playbackIsPlaying(PlaybackState* state);

/**
 * Starts song playback from the specified position
 *
 * @param state Pointer to the playback state
 * @param songRow Starting row position in the song
 * @param chainRow Starting row position in the chain
 * @param loop Whether to loop when reaching the end
 */
void playbackStartSong(PlaybackState* state, int songRow, int chainRow, int loop);

/**
 * Starts chain playback for a specific track
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to play
 * @param songRow Row position in the song containing the chain
 * @param chainRow Starting row position in the chain
 * @param loop Whether to loop when reaching the end
 */
void playbackStartChain(PlaybackState* state, int trackIdx, int songRow, int chainRow, int loop);

/**
 * Starts phrase playback for a specific track
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to play
 * @param songRow Row position in the song containing the phrase
 * @param chainRow Row position in the chain containing the phrase
 * @param loop Whether to loop when reaching the end
 */
void playbackStartPhrase(PlaybackState* state, int trackIdx, int songRow, int chainRow, int loop);

/**
 * Starts playback of a phrase row
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to play
 * @param phraseRow Phrase row data to play
 */
void playbackStartPhraseRow(PlaybackState* state, int trackIdx, PhraseRow* phraseRow);

/**
 * Queues a phrase for playback on a specific track
 * Only works if the track is currently in phrase playback mode
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to queue the phrase on
 * @param songRow Row position in the song containing the phrase
 * @param chainRow Row position in the chain containing the phrase
 */
void playbackQueuePhrase(PlaybackState* state, int trackIdx, int songRow, int chainRow);

/**
 * Stops playback on all tracks
 *
 * @param state Pointer to the playback state
 */
void playbackStop(PlaybackState* state);

/**
 * Plays a single note with an instrument for preview
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to use for preview
 * @param note Note value to play
 * @param instrument Instrument to use
 */
void playbackPreviewNote(PlaybackState* state, int trackIdx, uint8_t note, uint8_t instrument);



/**
 * Stops preview playback on a specific track
 *
 * @param state Pointer to the playback state
 * @param trackIdx Index of the track to stop preview on
 */
void playbackStopPreview(PlaybackState* state, int trackIdx);

/**
 * Advances playback by one frame
 *
 * @param state Pointer to the playback state
 * @param chips Array of sound chips to output to
 * @return 1 if all tracks have finished playing, 0 if any track is still active
 */
int playbackNextFrame(PlaybackState* state, SoundChip* chips);

#endif
