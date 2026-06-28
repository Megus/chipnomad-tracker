#ifndef __CHIPNOMAD_LIB_H__
#define __CHIPNOMAD_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

// Main ChipNomad library header
// Include this file to access all ChipNomad library functionality

#include "project.h"
#include "playback.h"
#include "chips/chips.h"
#include "utils.h"

#define AUDIO_OVERLOAD_COOLDOWN_FRAMES 20
#define PITCH_CONFLICT_COOLDOWN_FRAMES 5

/**
* Chip factory function type
* Returns a SoundChip struct for the given chip index
*/
typedef SoundChip (*ChipFactory)(int chipIndex, int sampleRate, ChipSetup setup);

/**
* ChipNomad state encapsulating all library state
*/
struct ChipNomadState {
  Project project;
  PlaybackState playbackState;
  SoundChip chips[PROJECT_MAX_CHIPS];
  int sampleRate;
  float frameSampleCounter;
  float mixVolume;
  int audioOverload;
  int trackWarnings[PROJECT_MAX_TRACKS];
  float* mixBuffer;
  int mixBufferSize;
  int aySampleDithering;
};

/**
* Create and initialize ChipNomad state
* @return Pointer to initialized state, or NULL on failure
*/
ChipNomadState* chipnomadCreate(void);

/**
* Destroy ChipNomad state and cleanup resources
* @param state State to destroy
*/
void chipnomadDestroy(ChipNomadState* state);

/**
* Initialize chips with project settings
* @param state ChipNomad state
* @param sampleRate Audio sample rate
* @param factory Chip factory function, or NULL to use default implementations
*/
void chipnomadInitChips(ChipNomadState* state, int sampleRate, ChipFactory factory);

/**
* Render audio with automatic tick rate handling
* @param state ChipNomad state
* @param buffer Interleaved stereo float buffer (left, right, left, right...)
* @param samples Number of stereo sample pairs to render
* @return Number of samples actually rendered (may be less if playback stops)
*/
int chipnomadRender(ChipNomadState* state, float* buffer, int samples);

/**
* Set emulation quality for all chips
* @param state ChipNomad state
* @param quality Quality level
*/
void chipnomadSetQuality(ChipNomadState* state, ChipNomadQuality quality);

#ifdef __cplusplus
}
#endif

#endif