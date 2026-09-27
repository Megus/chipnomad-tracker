#ifndef __AUDIO_SDL2_H__
#define __AUDIO_SDL2_H__

#include "audio_device.h"

class AudioDeviceSDL2 : public AudioDevice {
  public:
    bool setup(AudioCallbacks* callbacks, int sampleRate, int bufferSize) override;
    void pause(bool isPaused) override;
    void teardown() override;
};

#endif // __AUDIO_SDL2_H__
