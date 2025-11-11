#include <stdlib.h>
#include <ayumi.h>
#include <chips.h>
#include <stdio.h>
#include <project.h>

static int init(struct SoundChip* self) {
  return 0;
}

static void render(struct SoundChip* self, float* buffer, int samples) {
  struct ayumi* ay = self->userdata;

  for (int c = 0; c < samples; c++) {
    ayumi_process(ay);
    ayumi_remove_dc(ay);
    
    *buffer++ = ay->left;
    *buffer++ = ay->right;
  }
}

static void setRegister(struct SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg > 13) return;
  struct ayumi* ay = (struct ayumi*)self->userdata;
  self->regs[reg] = value;

  if (reg == 0 || reg == 1) {
    ayumi_set_tone(ay, 0, (self->regs[1] << 8) | self->regs[0]);
  } else if (reg == 2 || reg == 3) {
    ayumi_set_tone(ay, 1, (self->regs[3] << 8) | self->regs[2]);
  } else if (reg == 4 || reg == 5) {
    ayumi_set_tone(ay, 2, (self->regs[5] << 8) | self->regs[4]);
  } else if (reg == 6) {
    ayumi_set_noise(ay, self->regs[6]);
  } else if (reg >= 7 && reg <= 10) {
    ayumi_set_mixer(ay, 0, self->regs[7] & 1, (self->regs[7] >> 3) & 1, self->regs[8] >> 4);
    ayumi_set_mixer(ay, 1, (self->regs[7] >> 1) & 1, (self->regs[7] >> 4) & 1, self->regs[9] >> 4);
    ayumi_set_mixer(ay, 2, (self->regs[7] >> 2) & 1, (self->regs[7] >> 5) & 1, self->regs[10] >> 4);
    ayumi_set_volume(ay, 0, self->regs[8] & 0xf);
    ayumi_set_volume(ay, 1, self->regs[9] & 0xf);
    ayumi_set_volume(ay, 2, self->regs[10] & 0xf);
  } else if (reg == 11 || reg == 12) {
    ayumi_set_envelope(ay, (self->regs[12] << 8) | self->regs[11]);
  } else if (reg == 13) {
    ayumi_set_envelope_shape(ay, self->regs[13]);
  }
}

void updateChipAYType(struct SoundChip* self, uint8_t isYM) {
  ayumi_set_chip_type((struct ayumi*)self->userdata, isYM);
}

static void setPanning(struct ayumi* ay, enum StereoModeAY stereoMode, uint8_t separation) {
  float sep = (float)separation / 200.0;
  float panA = 0.5, panB = 0.5, panC = 0.5;

  switch (stereoMode) {
    case ayStereoABC:
      panA = 0.5 - sep; panB = 0.5; panC = 0.5 + sep;
      break;
    case ayStereoACB:
      panA = 0.5 - sep; panB = 0.5 + sep; panC = 0.5;
      break;
    case ayStereoBAC:
      panA = 0.5; panB = 0.5 - sep; panC = 0.5 + sep;
      break;
  }

  ayumi_set_pan(ay, 0, panA, 1);
  ayumi_set_pan(ay, 1, panB, 1);
  ayumi_set_pan(ay, 2, panC, 1);
}

void updateChipAYStereoMode(struct SoundChip* self, enum StereoModeAY stereoMode, uint8_t separation) {
  setPanning((struct ayumi*)self->userdata, stereoMode, separation);
}

void updateChipAYClock(struct SoundChip* self, int clockRate, int sampleRate) {
  struct ayumi* ay = (struct ayumi*)self->userdata;
  ay->step = (float)clockRate / (sampleRate * 8 * 8); // 8 * DECIMATE_FACTOR
}

static int cleanup(struct SoundChip* self) {
  free(self->userdata);
  return 0;
}

struct SoundChip createChipAY(int sampleRate, union ChipSetup setup) {
  struct ayumi* ay = malloc(sizeof(struct ayumi));
  ayumi_configure(ay, setup.ay.isYM, setup.ay.clock, sampleRate);

  setPanning(ay, setup.ay.stereoMode, setup.ay.stereoSeparation);

  struct SoundChip chip = {
    .userdata = ay,
    .init = init,
    .render = render,
    .setRegister = setRegister,
    .cleanup = cleanup,
  };

  for (int c = 0; c < 256; c++) {
    chip.regs[c] = 0;
  }
  chip.regs[7] = 0x3f;

  return chip;
}
