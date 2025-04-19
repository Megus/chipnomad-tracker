#include <stdio.h>
#include <audio_manager.h>
#include <corelib_audio.h>
#include <chips.h>
#include <project.h>

static int aSampleRate;
static int aBufferSize;
static FrameCallback *aFrameCallback;
static void* aFrameCallbackUserdata;

static float frameSampleCounter;

static void audioCallback(int16_t* buffer, int stereoSamples) {
  int samplesLeft = stereoSamples;

  while (samplesLeft != 0) {
    if ((int)frameSampleCounter == 0) {
      frameSampleCounter += aSampleRate / audioManager.frameRate;
      if (aFrameCallback != NULL) {
        aFrameCallback(aFrameCallbackUserdata);
      }
    }

    int samplesToRender = ((int)frameSampleCounter < samplesLeft) ? (int)frameSampleCounter : samplesLeft;

    audioManager.chips[0].render(&audioManager.chips[0], buffer, samplesToRender);

    buffer += samplesToRender * 2;
    samplesLeft -= samplesToRender;
    frameSampleCounter -= (float)samplesToRender;
  }
}

static int start(int sampleRate, int bufferSize, float frameRate) {
  aSampleRate = sampleRate;
  aBufferSize = bufferSize;
  audioManager.frameRate = frameRate;
  aFrameCallback = NULL;
  aFrameCallbackUserdata = NULL;
  frameSampleCounter = 0.0;

  audioSetup(audioCallback, sampleRate, bufferSize);

  return 0;
}

static void initChips(void) {
  audioManager.chips[0] = createChipAY(aSampleRate, project.chipSetup);
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

static void stop() {
  audioCleanup();
}

// Singleton AudioManager struct
struct AudioManager audioManager = {
  .start = start,
  .initChips = initChips,
  .setFrameCallback = setFrameCallback,
  .pause = pause,
  .resume = resume,
  .stop = stop
};
