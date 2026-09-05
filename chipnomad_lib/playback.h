#ifndef __CHIPNOMAD_LIB__PLAYBACK_H__
#define __CHIPNOMAD_LIB__PLAYBACK_H__

#include "project.h"
#include "chips/chips.h"
#include "playback_fx.h"
#include "playback_chips.h"
#include "playback_modulation.h"

struct ChipNomadState;

enum class PlaybackMode {
  none, // For queue
  stopped,
  song,
  chain,
  phrase,
  phraseRow,
  loop,
};

struct PlaybackTableState {
  uint8_t tableIdx;
  uint8_t rows[4];
  uint8_t counters[4];
  uint8_t speed[4];
  uint8_t fxAuxState[16][4]; // Used for stateful effects like HOP
};

struct PlaybackNoteState {
  uint8_t pitchBase;
  uint8_t instrument;
  uint8_t volume;

  uint8_t pitchFinal; // Calculated pitch value
  int8_t pitchOffset; // Pitch offset (semitones)
  int16_t fineOffset; // Fine pitch offset (cents or periods, depending on linearPitch setting)
  int16_t periodOffset; // Period offset
  uint8_t volume1; // Instrument volume
  uint8_t volume2; // Instrument table volume
  uint8_t volume3; // Aux table volume
  int8_t volumeOffset; // Volume offset

  PlaybackTableState instrumentTable;
  PlaybackTableState auxTable;
  PlaybackFXState fx[256]; // Active FX on this note, indexed by FX enum

  PlaybackModState modulation[4]; // Modulation states

  PlaybackChipNoteState chip;
};

struct PlaybackTrackQueue {
  PlaybackMode mode;
  int songRow;
  int chainRow;
  int phraseRow;
  int loop;
};

struct PlaybackTrackState {
  PlaybackTrackQueue queue;

  PlaybackMode mode;
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

  // Currently playing note
  PlaybackNoteState note;
  // Cached phrase row data
  PhraseRow currentPhraseRow;
  // FX auxillary state data for the phrase (used by HOP)
  uint8_t fxAuxState[16][3];
};

struct PlaybackAYChipState {
  uint8_t envShape;
};

union PlaybackChipState {
  PlaybackAYChipState ay;
};

struct LoopRange {
  int enabled;
  int level; // 0 = song, 1 = chain, 2 = phrase
  int startSongRow;
  int startChainRow;
  int startPhraseRow;
  int endSongRow;
  int endChainRow;
  int endPhraseRow;
};

struct PlaybackState {
  Project* p;
  PlaybackTrackState tracks[PROJECT_MAX_TRACKS];
  PlaybackChipState chips[PROJECT_MAX_CHIPS];
  uint8_t trackEnabled[PROJECT_MAX_TRACKS];
  LoopRange loopRange;
};

// FX typedefs
typedef void (*PlaybackFXInitFunc)(
  PlaybackState* state,
  PlaybackTrackState* track,
  int trackIdx,
  PlaybackFXState* fx,
  PlaybackTableState* tableState,
  int tableFXColumn
);
typedef void (*PlaybackFXRestartFunc)(
  PlaybackState* state,
  PlaybackTrackState* track,
  int trackIdx,
  PlaybackFXState* fx
);
typedef void (*PlaybackFXHandleFunc)(
  PlaybackState* state,
  PlaybackTrackState* track,
  int trackIdx,
  int chipIdx,
  PlaybackFXState* fx
);

struct PlaybackFXHandler {
  PlaybackFXInitFunc init;
  PlaybackFXHandleFunc handle;
  PlaybackFXRestartFunc restart;
};

#endif // __CHIPNOMAD_LIB__PLAYBACK_H__
