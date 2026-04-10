#include "chips/chips.h"
#include <string.h>

SoundChip createChipAY(int sampleRate, ChipSetup setup) {
  SoundChip chip;
  memset(&chip, 0, sizeof(SoundChip));
  return chip;
}

void updateChipAYType(SoundChip* chip, uint8_t isYM) {}
void updateChipAYStereoMode(SoundChip* chip, enum StereoModeAY stereoMode, uint8_t separation) {}
void updateChipAYClock(SoundChip* chip, int clockRate, int sampleRate) {}
