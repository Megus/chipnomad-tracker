#ifndef __CHIPNOMAD_LIB_H__
#define __CHIPNOMAD_LIB_H__

#include "chips/chips.h"
#include "project.h"
#include "playback.h"
#include "utils.h"

#define AUDIO_OVERLOAD_COOLDOWN_FRAMES 5
#define PITCH_CONFLICT_COOLDOWN_FRAMES 5

namespace chipnomad {
  // Chip factory function type. Returns a SoundChip instance for the given chip index
  typedef SoundChip* (*ChipFactory)(int chipIndex, int sampleRate, ChipSetup setup);

  class Player {
    public:
      // TODO: Some of these should become private or even grouped to structs
      ChipFactory chipFactory;
      Project* project;
      PlaybackState playbackState;
      SoundChip* chips[PROJECT_MAX_CHIPS];
      int sampleRate;
      float frameSampleCounter;
      float mixVolume;
      int audioOverload;
      int trackWarnings[PROJECT_MAX_TRACKS];
      float* mixBuffer;
      int mixBufferSize;
      int aySampleDithering;

      Player(ChipFactory factory, int sampleRate);
      ~Player();

      // Set the project. Performs all initialization needed for playback and chip setup.
      void setProject(Project* project);

      // Initialize chip objects based on the project configuration
      void initChips();

      // Set quality
      void setQuality(ChipNomadQuality quality);

      /**
       * Render audio samples
       * @param buffer Output buffer
       * @param samples Number of samples to render
       * @return Number of samples actually rendered
       */
      int render(float* buffer, int samples);

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
      int nextFrame();

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

    private:
      void detectAYPitchConflicts();
  };
}

#endif // __CHIPNOMAD_LIB_H__
