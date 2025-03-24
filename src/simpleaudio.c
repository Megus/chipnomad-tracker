#include <simpleaudio.h>

static int sr = 44100;
double volume = 0.2;

struct ayumi *chip;

int audio_setup(void) {
  // Initialize Ayumi chip emulator
  chip = malloc(sizeof(struct ayumi));
  ayumi_configure(chip, 1, 1750000, sr);
  ayumi_set_pan(chip, 0, 0.5, 1);
  ayumi_set_pan(chip, 1, 0.5, 1);
  ayumi_set_pan(chip, 2, 0.5, 1);

  SDL_AudioSpec spec;
  SDL_memset(&spec, 0, sizeof(spec));
  spec.freq = sr;
  spec.format = AUDIO_S16;
  spec.channels = 2;
  spec.samples = 2048;
  spec.callback = audio_callback;
  spec.userdata = NULL;
  if (SDL_OpenAudio(&spec, NULL) < 0) {
    fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
    return 1;
  }
  SDL_PauseAudio(0);
  return 0;
}

void audio_cleanup(void) {
  free(chip);
  SDL_CloseAudio();
}


void audio_callback(void * userdata, uint8_t * stream, int len) {
  int16_t * buffer = (int16_t *)stream;
  int genlen = len / sizeof(int16_t); // Convert from bytes to the number of samples

  for (int i = 0; i < genlen; i += 2) {
    ayumi_process(chip);
    buffer[i] = chip->left * 1024;
    buffer[i + 1] = chip->right * 1024;
  }
}