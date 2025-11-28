#include <stdio.h>
#include <audio_manager.h>
#include <corelib_audio.h>

#include <chipnomad_lib.h>

static int aSampleRate;
static int aBufferSize;

static void audioCallback(int16_t* buffer, int stereoSamples) {
  static float* floatBuffer = NULL;
  static int floatBufferSize = 0;

  if (floatBufferSize < stereoSamples * 2) {
    floatBuffer = realloc(floatBuffer, stereoSamples * 2 * sizeof(float));
    floatBufferSize = stereoSamples * 2;
  }

  chipnomadRender(chipnomadState, floatBuffer, stereoSamples);

  // Convert float to int16_t with volume
  for (int i = 0; i < stereoSamples * 2; i++) {
    int sample = floatBuffer[i] * appSettings.volume * appSettings.mixVolume * 32767;
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    buffer[i] = sample;
  }
}

static int start(int sampleRate, int bufferSize) {
  aSampleRate = sampleRate;
  aBufferSize = bufferSize;

  audioSetup(audioCallback, sampleRate, bufferSize);

  return 0;
}

static void pause(void) {
  audioPause(1);
}

static void resume(void) {
  audioPause(0);
}

static void stop() {
  audioCleanup();
}

// Singleton AudioManager struct
struct AudioManager audioManager = {
  .start = start,
  .pause = pause,
  .resume = resume,
  .stop = stop
};
