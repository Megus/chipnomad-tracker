#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chips.h"
#include "../external/ayumi/ayumi.h"
#include "../external/ayumi/ayumi_filters.h"

static constexpr float ayVolumeScale = 0.6f; // Scale AY volume to avoid clipping when mixing multiple chips

static void setPanning(struct ayumi* ay, StereoModeAY stereoMode, uint8_t separation) {
  float sep = (float)separation / 200.0;
  float panA = 0.5, panB = 0.5, panC = 0.5;

  switch (stereoMode) {
  case StereoModeAY::ABC:
    panA = 0.5 - sep; panB = 0.5; panC = 0.5 + sep;
    break;
  case StereoModeAY::ACB:
    panA = 0.5 - sep; panB = 0.5 + sep; panC = 0.5;
    break;
  case StereoModeAY::BAC:
    panA = 0.5; panB = 0.5 - sep; panC = 0.5 + sep;
    break;
  }

  ayumi_set_pan(ay, 0, panA, 1);
  ayumi_set_pan(ay, 1, panB, 1);
  ayumi_set_pan(ay, 2, panC, 1);
}

SoundChipAY::SoundChipAY(int sampleRate, ChipSetup setup) {
  this->sampleRate = sampleRate;
  memset(registers, 0, sizeof(registers));
  registers[7] = 0x3f;

  ay = (struct ayumi*)malloc(sizeof(struct ayumi));
  ayumi_configure(ay, setup.ay.isYM, setup.ay.clock, sampleRate);
  setPanning(ay, setup.ay.stereoMode, setup.ay.stereoSeparation);
}

SoundChipAY::~SoundChipAY() {
  free(ay);
}

void SoundChipAY::setTimerFunc(int (*func)(SoundChip* self, void* userdata), void* userdata) {
  this->timerFunc = func;
  this->timerUserdata = userdata;

  if (func) {
    // Set up ayumi timer with a C callback that bridges to our method
    ayumi_set_timer_func(ay, [](struct ayumi* ayPtr, void* ud) -> int {
      SoundChipAY* self = (SoundChipAY*)ud;
      if (self->timerFunc) {
        return self->timerFunc(self, self->timerUserdata);
      }
      return 0;
    }, this);
  } else {
    ayumi_set_timer_func(ay, NULL, NULL);
  }
}

void SoundChipAY::setRegister(uint16_t reg, uint8_t value) {
  if (reg > 13) return;
  registers[reg] = value;

  if (reg == 0 || reg == 1) {
    ayumi_set_tone(ay, 0, (registers[1] << 8) | registers[0]);
  } else if (reg == 2 || reg == 3) {
    ayumi_set_tone(ay, 1, (registers[3] << 8) | registers[2]);
  } else if (reg == 4 || reg == 5) {
    ayumi_set_tone(ay, 2, (registers[5] << 8) | registers[4]);
  } else if (reg == 6) {
    ayumi_set_noise(ay, registers[6]);
  } else if (reg >= 7 && reg <= 10) {
    ayumi_set_mixer(ay, 0, registers[7] & 1, (registers[7] >> 3) & 1, registers[8] >> 4);
    ayumi_set_mixer(ay, 1, (registers[7] >> 1) & 1, (registers[7] >> 4) & 1, registers[9] >> 4);
    ayumi_set_mixer(ay, 2, (registers[7] >> 2) & 1, (registers[7] >> 5) & 1, registers[10] >> 4);
    ayumi_set_volume(ay, 0, registers[8] & 0xf);
    ayumi_set_volume(ay, 1, registers[9] & 0xf);
    ayumi_set_volume(ay, 2, registers[10] & 0xf);
  } else if (reg == 11 || reg == 12) {
    ayumi_set_envelope(ay, (registers[12] << 8) | registers[11]);
  } else if (reg == 13) {
    ayumi_set_envelope_shape(ay, registers[13]);
  }
}

uint8_t SoundChipAY::getRegister(uint16_t reg) {
  if (reg > 15) return 0;
  return registers[reg];
}

void SoundChipAY::updateType(uint8_t isYM) {
  ayumi_set_chip_type(ay, isYM);
}

void SoundChipAY::updateStereoMode(StereoModeAY stereoMode, uint8_t separation) {
  setPanning(ay, stereoMode, separation);
}

void SoundChipAY::updateClock(int clockRate) {
  ay->step = (float)clockRate / (sampleRate * 8 * 8); // 8 * DECIMATE_FACTOR
}

void SoundChipAY::render(float* buffer, int samples) {
  for (int c = 0; c < samples; c++) {
    ayumi_process(ay);
    ayumi_remove_dc(ay);
    *buffer++ = ay->left * ayVolumeScale;
    *buffer++ = ay->right * ayVolumeScale;
  }
}

void SoundChipAY::setQuality(ChipNomadQuality quality) {
  ayumi_filter_func filter_func;
  switch (quality) {
    case ChipNomadQuality::low:
      filter_func = ayumi_filter_low;
      break;
    case ChipNomadQuality::medium:
      filter_func = ayumi_filter_medium;
      break;
    case ChipNomadQuality::high:
      filter_func = ayumi_filter_high;
      break;
    case ChipNomadQuality::best:
      filter_func = ayumi_filter_best;
      break;
    default:
      filter_func = ayumi_filter_medium;
      break;
  }

  ayumi_set_filter_quality(ay, filter_func);
}
