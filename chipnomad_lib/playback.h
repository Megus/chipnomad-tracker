#ifndef __CHIPNOMAD_LIB__PLAYBACK_H__
#define __CHIPNOMAD_LIB__PLAYBACK_H__

#include "project.h"
#include "chips/chips.h"
#include "playback_fx.h"
#include "playback_chips.h"
#include "playback_modulation.h"

namespace chipnomad {
  class Engine;
  class Player;

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

  // FX typedefs
  typedef void (*PlaybackFXInitFunc)(
    Player* player,
    PlaybackTrackState* track,
    int trackIdx,
    PlaybackFXState* fx,
    PlaybackTableState* tableState,
    int tableFXColumn
  );
  typedef void (*PlaybackFXRestartFunc)(
    Player* player,
    PlaybackTrackState* track,
    int trackIdx,
    PlaybackFXState* fx
  );
  typedef void (*PlaybackFXHandleFunc)(
    Player* player,
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

  // Convert frequency to AY period
  int frequencyToAYPeriod(float frequency, int clockHz);

  // Calculate AY period from pitch with offsets
  int16_t calculateAYPeriod(Project* p, uint8_t basePitch, int8_t pitchOffset, int16_t fineOffset,
                            int16_t specificFineOffset, int16_t periodOffset, int useFineOffset);

  int timerFunctionAY(SoundChip* chip, void* userdata);

  class Player {
    public:
      Project* p;
      PlaybackTrackState tracks[PROJECT_MAX_TRACKS];
      PlaybackChipState chips[PROJECT_MAX_CHIPS];
      uint8_t trackEnabled[PROJECT_MAX_TRACKS];
      LoopRange loopRange;

      Player();
      ~Player();

      // Checks if any track is currently playing
      bool isPlaying();

      /**
       * Starts song playback from the specified position
       * @param songRow Starting row position in the song
       * @param chainRow Starting row position in the chain
       * @param loop Whether to loop when reaching the end
       */
      void playSong(int songRow, int chainRow, int loop);

      /**
       * Starts chain playback for a specific track
       * @param trackIdx Index of the track to play
       * @param songRow Row position in the song containing the chain
       * @param chainRow Starting row position in the chain
       * @param loop Whether to loop when reaching the end
       */
      void playChain(int trackIdx, int songRow, int chainRow, int loop);

      /**
       * Starts phrase playback for a specific track
       * @param trackIdx Index of the track to play
       * @param songRow Row position in the song containing the phrase
       * @param chainRow Row position in the chain containing the phrase
       * @param loop Whether to loop when reaching the end
       */
      void playPhrase(int trackIdx, int songRow, int chainRow, int loop);

      /**
       * Starts playback of a phrase row
       * @param trackIdx Index of the track to play
       * @param phraseRow Phrase row data to play
       */
      void playPhraseRow(int trackIdx, PhraseRow* phraseRow);

      /**
       * Queues a phrase for playback on a specific track
       * Only works if the track is currently in phrase playback mode
       * @param trackIdx Index of the track to queue the phrase on
       * @param songRow Row position in the song containing the phrase
       * @param chainRow Row position in the chain containing the phrase
       */
      void queuePhrase(int trackIdx, int songRow, int chainRow);

      // Advances playback by one frame
      int nextFrame(Engine* engine);

      // Stops playback on all tracks
      void stop();

      /**
       * Plays a single note with an instrument for preview
       * @param trackIdx Index of the track to use for preview
       * @param note Note value to play
       * @param instrument Instrument to use
       */
      void previewNote(int trackIdx, uint8_t note, uint8_t instrument);

      /**
       * Stops preview playback on a specific track
       * @param trackIdx Index of the track to stop preview on
       */
      void stopPreview(int trackIdx);

      /**
       * Sets a loop range for playback
       * @param range Loop range configuration
       */
      void setLoopRange(LoopRange range);

      // Clears the loop range, disabling ranged loop
      void clearLoopRange();

      // Initializes the player for a project. Sets the project pointer, builds
      // the FX handler table and sample tables, and resets all tracks.
      // The caller owns the project.
      void init(Project* project);

      // Internal playback helpers.
      //
      // These are used by the FX handler functions (which are plain functions
      // taking a Player*, matching the PlaybackFX*Func typedefs) and by the
      // Engine. They are public so those free functions can reach them; Player,
      // like the rest of this refactor, keeps a C-with-classes flavor where the
      // data members (tracks, p, chips, ...) are also public.
      void handleNoteOff(int trackIdx);
      void readPhraseRow(int trackIdx, int skipDelCheck);
      void readPhraseRowDirect(int trackIdx, PhraseRow* phraseRow, int skipDelCheck);
      void tableInit(int trackIdx, struct PlaybackTableState* table, int tableIdx, int row, int speed);
      void tableReadFX(int trackIdx, struct PlaybackTableState* table, int fxIdx);
      void initFX(int trackIdx, uint8_t* fx, PlaybackTableState* tableState, int tableFXColumn);
      int handleFX(int trackIdx, int chipIdx);
      int restartFX(int trackIdx);
      void hopToTableRow(int trackIdx, PlaybackTableState* table, int tableRow);
      int vibratoCommonLogic(PlaybackFXState *pvbState, int scale);
      void resetOffsets(int trackIdx);

      void initFXHandlers(void);
      void registerFXHandlers_AY(void);
      void registerFXHandlers_Modulation(void);

      // Chip-specific functions

      // AY-3-8910/YM2149F
      void initAYSampleTables(void);
      void setupInstrumentAY1(int trackIdx);
      void setupInstrumentAY2(int trackIdx);
      void setupInstrumentAYSample(int trackIdx);
      void setupInstrumentAYWavetable(int trackIdx);
      void setupInstrument(int trackIdx);
      void handleInstrumentAY1(int trackIdx);
      void handleInstrumentAY2(int trackIdx);
      void handleInstrumentAYSample(int trackIdx);
      void handleInstrumentAYWavetable(int trackIdx);
      void outputRegistersAY(Engine* engine, int trackIdx, int chipIdx);
      void resetTrackAY(int trackIdx);
      void resetOffsetsAY(int trackIdx);

    private:
      PlaybackFXHandler fxHandlers[fxTotalCount];
  };
};

#endif // __CHIPNOMAD_LIB__PLAYBACK_H__
