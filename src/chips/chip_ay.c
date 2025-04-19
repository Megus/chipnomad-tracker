#include <stdlib.h>
#include <ayumi.h>
#include <chips.h>

struct ChipAYData {
  struct ayumi ay;
  uint8_t regs[13];
};

static int init(struct SoundChip* self) {
  return 0;
}

static void render(struct SoundChip* self, int16_t* buffer, int samples) {
  struct ayumi* ay = self->userdata;

  for (int c = 0; c < samples; c++) {
    ayumi_process(ay);
    *buffer++ = ay->left * 10000;
    *buffer++ = ay->right * 10000;
  }
}

static void setRegister(struct SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg > 13) return;
  struct ChipAYData* data = (struct ChipAYData*)self->userdata;
  data->regs[reg] = value;

  if (reg == 0 || reg == 1) {
    ayumi_set_tone(&data->ay, 0, (data->regs[1] << 8) | data->regs[0]);
  } else if (reg == 2 || reg == 3) {
    ayumi_set_tone(&data->ay, 1, (data->regs[3] << 8) | data->regs[2]);
  } else if (reg == 4 || reg == 5) {
    ayumi_set_tone(&data->ay, 2, (data->regs[5] << 8) | data->regs[4]);
  } else if (reg == 6) {
    ayumi_set_noise(&data->ay, data->regs[6]);
  } else if (reg >= 7 && reg <= 10) {
    ayumi_set_mixer(&data->ay, 0, data->regs[7] & 1, (data->regs[7] >> 3) & 1, data->regs[8] >> 4);
    ayumi_set_mixer(&data->ay, 1, (data->regs[7] >> 1) & 1, (data->regs[7] >> 4) & 1, data->regs[9] >> 4);
    ayumi_set_mixer(&data->ay, 2, (data->regs[7] >> 2) & 1, (data->regs[7] >> 5) & 1, data->regs[10] >> 4);
    ayumi_set_volume(&data->ay, 0, data->regs[8] & 0xf);
    ayumi_set_volume(&data->ay, 1, data->regs[9] & 0xf);
    ayumi_set_volume(&data->ay, 2, data->regs[10] & 0xf);
  } else if (reg == 11 || reg == 12) {
    ayumi_set_envelope(&data->ay, (data->regs[12] << 8) | data->regs[11]);
  } else if (reg == 13) {
    ayumi_set_envelope_shape(&data->ay, data->regs[13]);
  }
}

static int cleanup(struct SoundChip* self) {
  free(self->userdata);
  return 0;
}

struct SoundChip createChipAY(int sampleRate, union ChipSetup setup) {
  struct ChipAYData* data = malloc(sizeof(struct ChipAYData));
  ayumi_configure(&data->ay, setup.ay.isYM, setup.ay.clock, sampleRate);
  ayumi_set_pan(&data->ay, 0, (float)setup.ay.panA / 255., 1);
  ayumi_set_pan(&data->ay, 1, (float)setup.ay.panB / 255., 1);
  ayumi_set_pan(&data->ay, 2, (float)setup.ay.panC / 255., 1);

  struct SoundChip chip = {
    .userdata = data,
    .init = init,
    .render = render,
    .setRegister = setRegister,
    .cleanup = cleanup,
  };

  return chip;
}
