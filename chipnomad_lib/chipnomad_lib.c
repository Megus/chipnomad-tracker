#include "chipnomad_lib.h"
#include "playback.h"
#include <stdlib.h>
#include <string.h>

static void detectAYPitchConflicts(ChipNomadState* state);

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
  
  // Initialize mix buffer
  state->mixBufferSize = 8192;
  state->mixBuffer = malloc(state->mixBufferSize * sizeof(float));
  if (!state->mixBuffer) {
    free(state);
    return NULL;
  }

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

  // Cleanup mix buffer
  free(state->mixBuffer);
  
  free(state);
}

void chipnomadInitChips(ChipNomadState* state, int sampleRate, ChipFactory factory) {
  if (!state) return;

  // Cleanup existing chips if already initialized
  if (state->sampleRate > 0) {
    for (int i = 0; i < PROJECT_MAX_CHIPS; i++) {
      if (state->chips[i].cleanup) {
        state->chips[i].cleanup(&state->chips[i]);
      }
    }
  }

  // Zero the entire chips array for safety
  memset(state->chips, 0, sizeof(state->chips));
  state->sampleRate = sampleRate;

  // Use provided factory or default
  ChipFactory chipFactory = factory ? factory : defaultChipFactory;

  // Initialize chips based on project's chipsCount
  for (int i = 0; i < state->project.chipsCount; i++) {
    state->chips[i] = chipFactory(i, sampleRate, state->project.chipSetup);
  }
}

int chipnomadRender(ChipNomadState* state, float* buffer, int samples) {
  if (!state) return 0;

  int samplesLeft = samples;
  int allTracksStopped = 0;

  while (samplesLeft > 0 && !allTracksStopped) {
    if ((int)state->frameSampleCounter == 0) {
      state->frameSampleCounter += state->sampleRate / state->project.tickRate;
      allTracksStopped = playbackNextFrame(&state->playbackState, state->chips);
      // Decrease audio overload cooldown each frame
      if (state->audioOverload > 0) {
        state->audioOverload--;
      }
      // Detect AY pitch conflicts each frame
      detectAYPitchConflicts(state);
    }

    int samplesToRender = ((int)state->frameSampleCounter < samplesLeft) ?
    (int)state->frameSampleCounter : samplesLeft;
    int bufferOffset = (samples - samplesLeft) * 2;

    // Clear buffer section
    for (int i = 0; i < samplesToRender * 2; i++) {
      buffer[bufferOffset + i] = 0.0f;
    }

    // Ensure mix buffer is large enough
    int requiredSize = samplesToRender * 2;
    if (requiredSize > state->mixBufferSize) {
      state->mixBufferSize = requiredSize;
      state->mixBuffer = realloc(state->mixBuffer, state->mixBufferSize * sizeof(float));
      if (!state->mixBuffer) return 0; // Out of memory
    }

    // Mix all chips
    for (int chipIdx = 0; chipIdx < state->project.chipsCount; chipIdx++) {
      SoundChip* chip = &state->chips[chipIdx];
      if (chip->render) {
        // Render chip to mix buffer
        chip->render(chip, state->mixBuffer, samplesToRender);

        // Mix into main buffer
        for (int i = 0; i < samplesToRender * 2; i++) {
          buffer[bufferOffset + i] += state->mixBuffer[i];
        }
      }
    }

    // Apply mix volume and detect overload
    for (int i = 0; i < samplesToRender * 2; i++) {
      buffer[bufferOffset + i] *= state->mixVolume;
      // Check for audio overload (values beyond -1.0 to 1.0 range)
      if (buffer[bufferOffset + i] > 1.0f || buffer[bufferOffset + i] < -1.0f) {
        state->audioOverload = AUDIO_OVERLOAD_COOLDOWN_FRAMES;
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

static void detectAYPitchConflicts(ChipNomadState* state) {
  if (!state || state->project.chipType != chipAY) return;

  // Decrease existing warning cooldowns
  for (int i = 0; i < state->project.tracksCount; i++) {
    if (state->trackWarnings[i] > 0) {
      state->trackWarnings[i]--;
    }
  }

  // Collect tone periods for all tracks (0xFFFF = not using tone)
  uint16_t trackPeriods[PROJECT_MAX_TRACKS];
  for (int chipIdx = 0; chipIdx < state->project.chipsCount; chipIdx++) {
    SoundChip* chip = &state->chips[chipIdx];
    int trackOffset = chipIdx * 3;
    uint8_t mixer = chip->regs[7];

    for (int i = 0; i < 3; i++) {
      uint16_t period = chip->regs[i * 2] | (chip->regs[i * 2 + 1] << 8);
      uint8_t volume = chip->regs[8 + i];
      int toneEnabled = ((mixer >> i) & 1) == 0;
      int noiseEnabled = ((mixer >> (i + 3)) & 1) == 0;
      int envelopeMode = (volume & 16) != 0;

      // Check if track uses tone generation
      int isPureNoise = !toneEnabled && noiseEnabled && !envelopeMode;
      int isPureEnvelope = !toneEnabled && !noiseEnabled && envelopeMode;
      int isZeroVolume = (volume & 0xf) == 0 && !envelopeMode;
      int usesTone = !(isPureNoise || isPureEnvelope || isZeroVolume);

      trackPeriods[trackOffset + i] = (usesTone && period != 0) ? period : 0xFFFF;
    }
  }

  // Find conflicts by comparing all track periods
  for (int i = 0; i < state->project.tracksCount; i++) {
    for (int j = i + 1; j < state->project.tracksCount; j++) {
      if (trackPeriods[i] != 0xFFFF && trackPeriods[i] == trackPeriods[j]) {
        state->trackWarnings[i] = PITCH_CONFLICT_COOLDOWN_FRAMES;
        state->trackWarnings[j] = PITCH_CONFLICT_COOLDOWN_FRAMES;
      }
    }
  }
}