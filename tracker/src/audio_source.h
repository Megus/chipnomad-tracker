#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include <stdint.h>

// Abstract audio source for streaming preview playback.
// Implementations provide mono 16-bit samples at their native sample rate;
// the AudioManager handles resampling and mixing into the output.
class AudioSource {
public:
  virtual ~AudioSource() {}

  // Read next N sample frames as signed 16-bit mono.
  // Returns actual number of samples written (0 = finished / end of stream).
  virtual uint32_t readSamples16(int16_t* buffer, uint32_t count) = 0;

  // Native sample rate of this source (used to compute the resampling ratio).
  virtual uint32_t getSampleRate() const = 0;

  // Whether the source has no more samples to produce.
  virtual bool isFinished() const = 0;
};

#endif // AUDIO_SOURCE_H
