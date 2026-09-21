#ifndef __CHIPNOMAD_LIB__CHIP_AY_H__
#define __CHIPNOMAD_LIB__CHIP_AY_H__

#include "chips.h"
#include "../external/ayumi/ayumi.h"

namespace chipnomad {

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
    void setQuality(EmulationQuality quality) override;
};

}; // namespace chipnomad

#endif // __CHIPNOMAD_LIB__CHIP_AY_H__