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

  class Engine {
    public:
      // TODO: Some of these should become private or even grouped to structs
      ChipFactory chipFactory;
      Project* project;
      Player player;
      SoundChip* chips[PROJECT_MAX_CHIPS];
      int sampleRate;
      float frameSampleCounter;
      float mixVolume;
      int audioOverload;
      int trackWarnings[PROJECT_MAX_TRACKS];
      float* mixBuffer;
      int mixBufferSize;
      int aySampleDithering;

      Engine(ChipFactory factory, int sampleRate);
      ~Engine();

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

    private:
      void detectAYPitchConflicts();
  };
};

#endif // __CHIPNOMAD_LIB_H__
