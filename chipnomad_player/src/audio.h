#ifndef AUDIO_H
#define AUDIO_H

#include <project.h>
#include <playback.h>
#include <chips.h>

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