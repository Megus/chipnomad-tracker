#include "chipnomad_lib.h"
#include "playback.h"
#include <stdlib.h>
#include <string.h>

static SoundChip defaultChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  return createChipAY(sampleRate, setup);
}

ChipNomadState* chipnomadCreate(void) {
  ChipNomadState* state = malloc(sizeof(ChipNomadState));
  if (!state) return NULL;

  memset(state, 0, sizeof(ChipNomadState));
  fillFXNames();
  projectInit(&state->project);
  playbackInit(&state->playbackState, &state->project);
  state->mixVolume = 0.6f;

  return state;
}

void chipnomadDestroy(ChipNomadState* state) {
  if (!state) return;

  // Cleanup chips
  for (int i = 0; i < PROJECT_MAX_CHIPS; i++) {
    if (state->chips[i].cleanup) {
      state->chips[i].cleanup(&state->chips[i]);
    }
  }

  free(state);
}

void chipnomadInitChips(ChipNomadState* state, int sampleRate, ChipFactory factory) {
  if (!state) return;

  // Cleanup existing chip if already initialized
  if (state->sampleRate > 0 && state->chips[0].cleanup) {
    state->chips[0].cleanup(&state->chips[0]);
  }
  state->sampleRate = sampleRate;

  // Use provided factory or default
  ChipFactory chipFactory = factory ? factory : defaultChipFactory;
  state->chips[0] = chipFactory(0, sampleRate, state->project.chipSetup);
}

int chipnomadRender(ChipNomadState* state, float* buffer, int samples) {
  if (!state) return 0;

  int samplesLeft = samples;
  int allTracksStopped = 0;

  while (samplesLeft > 0 && !allTracksStopped) {
    if ((int)state->frameSampleCounter == 0) {
      state->frameSampleCounter += state->sampleRate / state->project.tickRate;
      allTracksStopped = playbackNextFrame(&state->playbackState, state->chips);
    }

    int samplesToRender = ((int)state->frameSampleCounter < samplesLeft) ?
    (int)state->frameSampleCounter : samplesLeft;
    int bufferOffset = (samples - samplesLeft) * 2;

    SoundChip* chip = &state->chips[0];
    if (chip) {
      chip->render(chip, buffer + bufferOffset, samplesToRender);
      // Apply mix volume
      for (int i = 0; i < samplesToRender * 2; i++) {
        buffer[bufferOffset + i] *= state->mixVolume;
      }
    } else {
      // Fill with silence if no chip
      for (int i = 0; i < samplesToRender * 2; i++) {
        buffer[bufferOffset + i] = 0.0f;
      }
    }

    samplesLeft -= samplesToRender;
    state->frameSampleCounter -= (float)samplesToRender;
  }

  // Fill remaining buffer with silence if playback stopped early
  if (samplesLeft > 0) {
    int bufferOffset = (samples - samplesLeft) * 2;
    for (int i = 0; i < samplesLeft * 2; i++) {
      buffer[bufferOffset + i] = 0.0f;
    }
  }

  return samples - samplesLeft;
}