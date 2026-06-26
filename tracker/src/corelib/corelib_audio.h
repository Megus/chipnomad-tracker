#ifndef __CORELIB_AUDIO_H__
#define __CORELIB_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

// 16-bit interleaved stereo buffer
typedef void AudioCallback(int16_t* buffer, int stereoSamples);

int audioSetup(AudioCallback* audioCallback, int sampleRate, int bufferSize);
void audioPause(int isPaused);
void audioCleanup(void);


#ifdef __cplusplus
}
#endif

#endif
