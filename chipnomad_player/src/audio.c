#include "audio.h"
#include <SDL2/SDL.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 1024
#define MAX_AUDIO_BUFFER_SIZE (BUFFER_SIZE * 2)

static struct AudioState* audioState;

void audioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    static float floatBuffer[MAX_AUDIO_BUFFER_SIZE];
    
    if (!*audioState->isPlaying) {
        SDL_memset(stream, 0, len);
        return;
    }
    
    int stereoSamples = len / sizeof(int16_t) / 2;
    int16_t* output = (int16_t*)stream;
    
    int samplesLeft = stereoSamples;
    
    while (samplesLeft != 0) {
        if ((int)audioState->frameSampleCounter == 0) {
            audioState->frameSampleCounter += SAMPLE_RATE / audioState->project->tickRate;
            if (playbackNextFrame(audioState->playback, audioState->chip)) {
                *audioState->isPlaying = 0;
                break;
            }
        }
        
        int samplesToRender = ((int)audioState->frameSampleCounter < samplesLeft) ? 
                              (int)audioState->frameSampleCounter : samplesLeft;
        int bufferOffset = (stereoSamples - samplesLeft) * 2;
        
        audioState->chip->render(audioState->chip, floatBuffer + bufferOffset, samplesToRender);
        
        samplesLeft -= samplesToRender;
        audioState->frameSampleCounter -= (float)samplesToRender;
    }
    
    // Convert float to int16
    for (int i = 0; i < stereoSamples * 2; i++) {
        float sample = floatBuffer[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        output[i] = (int16_t)(sample * 32767.0f);
    }
}

int audioInit(struct AudioState* state) {
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

void audioStart(struct AudioState* state) {
    playbackStartSong(state->playback, 0, 0, 1);
    state->frameSampleCounter = 0.0f;
    *state->isPlaying = 1;
}