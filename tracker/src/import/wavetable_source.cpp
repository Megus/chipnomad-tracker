#include "wavetable_source.h"
#include "wavetable_io.h"
#include "playback_chips.h"

// C-4 frequency in Hz (used to set the playback pitch)
#define C4_FREQUENCY 261.63

WavetableSource::WavetableSource(const char* path, bool isYM) {
  wavetableCount = 0;
  currentWavetable = 0;
  currentCycle = 0;
  currentSample = 0;

  // One wavetable cycle = 32 samples. To play at C-4, one cycle must last
  // 1/C4 seconds, so the native sample rate is 32 * C4.
  sourceSampleRate = (uint32_t)(WAVETABLE_LENGTH * C4_FREQUENCY);

  // Build the 4-bit level -> signed 16-bit amplitude table using the AY/YM
  // volume curve (cnDACTableAY/YM give unsigned 0-255 amplitude).
  const uint8_t* dacTable = isYM ? cnDACTableYM : cnDACTableAY;
  for (int i = 0; i < 16; i++) {
    // Convert unsigned 0-255 to signed 16-bit range, centered at 0
    levelToSample[i] = (int16_t)(((int)dacTable[i] - 128) * 256);
  }

  // Load wavetables from file
  wavetableCount = wavetableLoad(path, wavetables, 0);
  if (wavetableCount < 0) wavetableCount = 0;
}

WavetableSource::~WavetableSource() {
}

uint32_t WavetableSource::readSamples16(int16_t* buffer, uint32_t count) {
  if (wavetableCount <= 0) return 0;

  for (uint32_t i = 0; i < count; i++) {
    // Output current sample from current wavetable, converted to amplitude
    uint8_t level = wavetables[currentWavetable][currentSample] & 0x0F;
    buffer[i] = levelToSample[level];

    // Advance within the cycle
    currentSample++;
    if (currentSample >= WAVETABLE_LENGTH) {
      currentSample = 0;
      currentCycle++;
      if (currentCycle >= CYCLES_PER_WAVETABLE) {
        currentCycle = 0;
        currentWavetable++;
        if (currentWavetable >= wavetableCount) {
          currentWavetable = 0; // Loop back to first
        }
      }
    }
  }

  return count;
}
