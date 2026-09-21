#ifndef __CHIPNOMAD_LIB__CHIPS_H__
#define __CHIPNOMAD_LIB__CHIPS_H__

#include <stdint.h>
#include "chipnomad_constants.h"

namespace chipnomad {
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
      virtual void setQuality(EmulationQuality quality) {};
      virtual void render(float* buffer, int samples) {};
  };
} // namespace chipnomad

#endif // __CHIPNOMAD_LIB__CHIPS_H__
