#ifndef __CHIPNOMAD_LIB__CHIPS_H__
#define __CHIPNOMAD_LIB__CHIPS_H__

#include <stdint.h>
#include "../project.h"

/**
* Chip emulation quality levels
*/
enum class ChipNomadQuality : int {
  low,
  medium,
  high,
  best
};

struct ayumi;

class SoundChip {
  protected:
    int (*timerFunc)(SoundChip* self, void* userdata);
    void* timerUserdata;

  public:
    SoundChip() : timerFunc(nullptr), timerUserdata(nullptr) {}
    virtual ~SoundChip() {}

    virtual void setTimerFunc(int (*timerFunc)(SoundChip* self, void* userdata), void* timerUserdata) {
      this->timerFunc = timerFunc;
      this->timerUserdata = timerUserdata;
    }

    virtual void setRegister(uint16_t reg, uint8_t value) {};
    virtual uint8_t getRegister(uint16_t reg) { return 0; };
    virtual void setQuality(ChipNomadQuality quality) {};
    virtual void render(float* buffer, int samples) {};
    /**
     * @brief Render the chip and optionally capture per-channel output
     *
     * @param buffer Interleaved stereo output (left, right, ...)
     * @param channels Per-channel float buffers, or NULL to skip capture
     * @param samples Number of stereo sample pairs to render
     */
    virtual void renderChannels(float* buffer, float** channels, int samples) {
      render(buffer, samples);
      (void)channels;
    }
};

class SoundChipAY : public SoundChip {
  private:
    int sampleRate;
    uint8_t registers[16];
    ayumi* ay;

  public:
    SoundChipAY(int sampleRate, ChipSetup setup);
    ~SoundChipAY() override;

    void setRegister(uint16_t reg, uint8_t value) override;
    uint8_t getRegister(uint16_t reg) override;

    void updateType(uint8_t isYM);
    void updateStereoMode(StereoModeAY stereoMode, uint8_t separation);
    void updateClock(int clockRate);

    void setTimerFunc(int (*timerFunc)(SoundChip* self, void* userdata), void* timerUserdata) override;
    void render(float* buffer, int samples) override;
    void renderChannels(float* buffer, float** channels, int samples) override;
    void setQuality(ChipNomadQuality quality) override;
};

#endif // __CHIPNOMAD_LIB__CHIPS_H__
