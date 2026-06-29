#include "audio.h"
#include <SDL2/SDL.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 1024
#define MAX_AUDIO_BUFFER_SIZE (BUFFER_SIZE * 2)

static AudioState* audioState;

void audioCallback(void* userdata, Uint8* stream, int len) {
  (void)userdata;
  static float floatBuffer[MAX_AUDIO_BUFFER_SIZE];

  if (!*audioState->isPlaying) {
    SDL_memset(stream, 0, len);
    return;
  }

  int stereoSamples = len / sizeof(int16_t) / 2;
  int16_t* output = (int16_t*)stream;

  int samplesRendered = chipnomadRender(audioState->chipnomadState, floatBuffer, stereoSamples);
  if (samplesRendered < stereoSamples) {
    *audioState->isPlaying = 0;
    // Fill remaining buffer with silence
    for (int i = samplesRendered * 2; i < stereoSamples * 2; i++) {
      floatBuffer[i] = 0.0f;
    }
  }

  // Convert float to int16
  for (int i = 0; i < stereoSamples * 2; i++) {
    float sample = floatBuffer[i];
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    output[i] = (int16_t)(sample * 32767.0f);
  }
}

int audioInit(AudioState* state) {
  audioState = state;

  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = SAMPLE_RATE;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = BUFFER_SIZE;
  want.callback = audioCallback;
  want.userdata = NULL;

  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (dev == 0) {
    fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
    return -1;
  }

  SDL_PauseAudioDevice(dev, 0);
  return 0;
}

void audioStart(AudioState* state) {
  playbackStartSong(&state->chipnomadState->playbackState, 0, 0, 1);
  *state->isPlaying = 1;
}