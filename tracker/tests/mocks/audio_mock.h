#ifndef __AUDIO_MOCK_H__
#define __AUDIO_MOCK_H__

#include "i_audio.h"

// AudioMock: IAudio stub for tests. Records calls; never opens a real device.
class AudioMock : public IAudio {
  public:
    int setupCalls = 0;
    int pauseCalls = 0;
    int cleanupCalls = 0;
    int lastSampleRate = 0;
    int lastBufferSize = 0;
    int lastPaused = -1;
    AudioCallback* lastCallback = nullptr;
    int setupResult = 1;

    int setup(AudioCallback* callback, int sampleRate, int bufferSize) override {
      setupCalls++;
      lastCallback = callback;
      lastSampleRate = sampleRate;
      lastBufferSize = bufferSize;
      return setupResult;
    }

    void pause(int isPaused) override {
      pauseCalls++;
      lastPaused = isPaused;
    }

    void cleanup() override {
      cleanupCalls++;
    }
};

#endif // __AUDIO_MOCK_H__
