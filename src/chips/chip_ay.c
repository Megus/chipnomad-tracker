#include <stdlib.h>
#include <ayumi.h>
#include <chips.h>

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

static int cleanup(struct SoundChip* self) {
  return 0;
}


struct SoundChip createAyumi(int sampleRate) {

  struct ayumi* ay = malloc(sizeof(struct ayumi));
  ayumi_configure(ay, 1, 1750000, sampleRate);
  ayumi_set_pan(ay, 0, 0.2, 1);
  ayumi_set_pan(ay, 1, 0.5, 1);
  ayumi_set_pan(ay, 2, 0.8, 1);

  struct SoundChip chip = {
    .userdata = ay,
    .init = init,
    .render = render,
    .cleanup = cleanup,
  };

  return chip;
}