#ifndef __CHIPNOMAD_LIB_H__
#define __CHIPNOMAD_LIB_H__

#include "chips/chips.h"
#include "project.h"
#include "playback.h"
#include "utils.h"

#define AUDIO_OVERLOAD_COOLDOWN_FRAMES 5
#define PITCH_CONFLICT_COOLDOWN_FRAMES 5

/**
* Chip factory function type
* Returns a SoundChip pointer for the given chip index
*/
typedef SoundChip* (*ChipFactory)(int chipIndex, int sampleRate, ChipSetup setup);

/**
* ChipNomad state encapsulating all library state
*/
struct ChipNomadState {
  Project project;
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
 * Render audio with automatic tick rate handling, and capture per-track output.
 *
 * @param state ChipNomad state
 * @param buffer Interleaved stereo float buffer (left, right, left, right...)
 * @param samples Number of stereo sample pairs to render
 * @param trackBuffers Array of trackCount float buffers (mono, length `samples`)
 *        that receive each track's (chip channel's) output. May be NULL.
 * @param trackCount Number of tracks (== project.tracksCount)
 * @return Number of samples actually rendered
 */
int chipnomadRenderTracks(ChipNomadState* state, float* buffer, int samples, float** trackBuffers, int trackCount);

/**
* Set emulation quality for all chips
* @param state ChipNomad state
* @param quality Quality level
*/
void chipnomadSetQuality(ChipNomadState* state, ChipNomadQuality quality);

#endif
