#ifndef AUDIO_H
#define AUDIO_H

#include <chipnomad_lib.h>

typedef struct AudioState {
  ChipNomadState* chipnomadState;
  int* isPlaying;
} AudioState;

int audioInit(AudioState* audioState);
void audioStart(AudioState* audioState);

#endif