#ifndef AUDIO_H
#define AUDIO_H

#include "chipnomad_lib.h"

struct AudioState {
  chipnomad::Engine* engine;
  int* isPlaying;
};

int audioInit(AudioState* audioState);
void audioStart(AudioState* audioState);

#endif