#ifndef AUDIO_H
#define AUDIO_H

#include "chipnomad_lib.h"

struct AudioState {
  ChipNomadState* chipnomadState;
  int* isPlaying;
};

int audioInit(AudioState* audioState);
void audioStart(AudioState* audioState);

#endif