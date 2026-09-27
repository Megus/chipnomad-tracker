#ifndef __AUDIO_SDL12_H__
#define __AUDIO_SDL12_H__

#include "audio_device.h"

class AudioDeviceSDL12 : public AudioDevice {
  public:
    bool setup(AudioCallbacks* callbacks, int sampleRate, int bufferSize) override;
    void pause(bool isPaused) override;
    void teardown() override;
};

#endif // __AUDIO_SDL12_H__
