#ifndef __AUDIO_DEVICE_H__
#define __AUDIO_DEVICE_H__

#include <stdint.h>

// Audio callbacks. Currently only output. Later MIDI can be added here as well
class AudioCallbacks {
  public:
    virtual ~AudioCallbacks() = default;

    // Fill 16-bit interleaved stereo audio output buffer
    virtual void onAudioOutput(int16_t* buffer, int stereoSamples) = 0;
};

// Audio device interface
class AudioDevice {
  public:
    virtual ~AudioDevice() = default;

    // Open the audio device
    virtual bool setup(AudioCallbacks* callbacks, int sampleRate, int bufferSize) = 0;

    // Pause or resume audio output
    virtual void pause(bool isPaused) = 0;

    // Close the audio device
    virtual void teardown() = 0;
};

#endif // __AUDIO_DEVICE_H__
