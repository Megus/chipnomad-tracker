#ifndef WAVETABLE_SOURCE_H
#define WAVETABLE_SOURCE_H

#include <stdint.h>
#include "audio_source.h"

// Audio source that plays AY wavetables loaded from a .aywave file.
// Each wavetable is 32 samples of 4-bit values. Wavetables are played at a
// fixed pitch (C-4), each repeated a number of cycles before advancing to the
// next, looping back to the first at the end. 4-bit levels are converted to
// amplitude using the AY/YM DAC volume curve.
class WavetableSource : public AudioSource {
public:
  // Load wavetables from a .aywave file. isYM selects the volume curve.
  // Check isValid() after construction.
  WavetableSource(const char* path, bool isYM);
  ~WavetableSource();

  bool isValid() const { return wavetableCount > 0; }

  // AudioSource interface
  uint32_t readSamples16(int16_t* buffer, uint32_t count) override;
  uint32_t getSampleRate() const override { return sourceSampleRate; }
  bool isFinished() const override { return false; } // Loops forever

private:
  static const int WAVETABLE_LENGTH = 32;
  static const int MAX_WAVETABLES = 256;
  static const int CYCLES_PER_WAVETABLE = 64;

  uint8_t wavetables[MAX_WAVETABLES][WAVETABLE_LENGTH];
  int wavetableCount;
  uint32_t sourceSampleRate;

  // Precomputed signed 16-bit amplitude for each 4-bit level
  int16_t levelToSample[16];

  // Playback position
  int currentWavetable;  // Which wavetable we're playing
  int currentCycle;      // Which repetition of that wavetable
  int currentSample;     // Position within the 32-sample cycle

  // Prevent copying
  WavetableSource(const WavetableSource&);
  WavetableSource& operator=(const WavetableSource&);
};

#endif // WAVETABLE_SOURCE_H
