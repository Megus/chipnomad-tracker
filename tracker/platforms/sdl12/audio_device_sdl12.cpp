#include <stdio.h>
#include <SDL/SDL.h>
#include "audio_device_sdl12.h"

static void sdlAudioCallback(void* userdata, uint8_t* buffer, int bufferBytes) {
  AudioCallbacks* callbacks = (AudioCallbacks*)userdata;

  callbacks->onAudioOutput((int16_t *)buffer, bufferBytes / sizeof(int16_t) / 2); // Divide by 2 to get number of stereo samples
}

bool AudioDeviceSDL12::setup(AudioCallbacks* callbacks, int sampleRate, int bufferSize) {
  SDL_AudioSpec spec;
  SDL_memset(&spec, 0, sizeof(spec));
  spec.freq = sampleRate;
  spec.format = AUDIO_S16;
  spec.channels = 2;
  spec.samples = bufferSize;
  spec.callback = sdlAudioCallback;
  spec.userdata = (void*)callbacks;

  if (SDL_OpenAudio(&spec, NULL) < 0) {
    fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
    return 0;
  }

  SDL_PauseAudio(0);

  return true;
}

void AudioDeviceSDL12::pause(bool isPaused) {
  SDL_PauseAudio(isPaused ? 1 : 0);
}

void AudioDeviceSDL12::teardown() {
  SDL_CloseAudio();
}
