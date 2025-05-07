#include <corelib_audio.h>
#include <SDL2/SDL.h>

static void sdlAudioCallback(void* userdata, uint8_t* buffer, int bufferBytes) {
  AudioCallback* callback = userdata;

  callback((int16_t *)buffer, bufferBytes / sizeof(int16_t) / 2); // Divide by 2 to get number of stereo samples
}

int audioSetup(AudioCallback* audioCallback, int sampleRate, int bufferSize) {
  SDL_AudioSpec spec;
  SDL_memset(&spec, 0, sizeof(spec));
  spec.freq = sampleRate;
  spec.format = AUDIO_S16;
  spec.channels = 2;
  spec.samples = bufferSize;
  spec.callback = sdlAudioCallback;
  spec.userdata = audioCallback;

  if (SDL_OpenAudio(&spec, NULL) < 0) {
    fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
    return 1;
  }

  SDL_PauseAudio(0);

  return 0;
}

void audioPause(int isPaused) {
  SDL_PauseAudio(isPaused);
}

void audioCleanup(void) {
  SDL_CloseAudio();
}
