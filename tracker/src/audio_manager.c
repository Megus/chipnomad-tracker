#include <stdio.h>
#include <audio_manager.h>
#include <corelib_audio.h>

#include <chipnomad_lib.h>

static int aSampleRate;
static int aBufferSize;
static FrameCallback *aFrameCallback;
static void* aFrameCallbackUserdata;

static float frameSampleCounter;

static void audioCallback(int16_t* buffer, int stereoSamples) {
  static float* floatBuffer = NULL;
  static int floatBufferSize = 0;
  
  if (floatBufferSize < stereoSamples * 2) {
    floatBuffer = realloc(floatBuffer, stereoSamples * 2 * sizeof(float));
    floatBufferSize = stereoSamples * 2;
  }
  
  int samplesLeft = stereoSamples;

  while (samplesLeft != 0) {
    if ((int)frameSampleCounter == 0) {
      frameSampleCounter += aSampleRate / audioManager.tickRate;
      if (aFrameCallback != NULL) {
        aFrameCallback(aFrameCallbackUserdata);
      }
    }

    int samplesToRender = ((int)frameSampleCounter < samplesLeft) ? (int)frameSampleCounter : samplesLeft;
    int bufferOffset = (stereoSamples - samplesLeft) * 2;

    struct SoundChip* chip = chipnomadGetChip(0);
    if (chip) {
      chip->render(chip, floatBuffer + bufferOffset, samplesToRender);
    }

    samplesLeft -= samplesToRender;
    frameSampleCounter -= (float)samplesToRender;
  }
  
  // Convert float to int16_t with volume
  for (int i = 0; i < stereoSamples * 2; i++) {
    int sample = floatBuffer[i] * appSettings.volume * appSettings.mixVolume * 32767;
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    buffer[i] = sample;
  }
}

static int start(int sampleRate, int bufferSize, float tickRate) {
  aSampleRate = sampleRate;
  aBufferSize = bufferSize;
  audioManager.tickRate = tickRate;
  aFrameCallback = NULL;
  aFrameCallbackUserdata = NULL;
  frameSampleCounter = 0.0;

  audioSetup(audioCallback, sampleRate, bufferSize);

  return 0;
}

static void initChips(void) {
  chipnomadInitChips(aSampleRate);
}

static void setFrameCallback(FrameCallback *callback, void* userdata) {
  aFrameCallback = callback;
  aFrameCallbackUserdata = userdata;
}

static void pause(void) {
  audioPause(1);
}

static void resume(void) {
  audioPause(0);
}

static void render(float* buffer, int stereoSamples) {
  struct SoundChip* chip = chipnomadGetChip(0);
  if (chip) {
    chip->render(chip, buffer, stereoSamples);
  }
}

static void stop() {
  audioCleanup();
}

// Singleton AudioManager struct
struct AudioManager audioManager = {
  .start = start,
  .initChips = initChips,
  .setFrameCallback = setFrameCallback,
  .render = render,
  .pause = pause,
  .resume = resume,
  .stop = stop
};
