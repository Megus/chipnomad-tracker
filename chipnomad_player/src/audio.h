#ifndef AUDIO_H
#define AUDIO_H

#include <chipnomad_lib.h>

struct AudioState {
    struct Project* project;
    struct PlaybackState* playback;
    struct SoundChip* chip;
    int* isPlaying;
    float frameSampleCounter;
};

int audioInit(struct AudioState* audioState);
void audioStart(struct AudioState* audioState);

#endif