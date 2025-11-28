/* Author: Peter Sovietov */
/* Modifications by Megus */

#include <string.h>
#include <math.h>
#include "ayumi.h"
#include "ayumi_filters.h"

static const float AY_dac_table[] = {
  0.0, 0.0,
  0.00999465934234, 0.00999465934234,
  0.0144502937362, 0.0144502937362,
  0.0210574502174, 0.0210574502174,
  0.0307011520562, 0.0307011520562,
  0.0455481803616, 0.0455481803616,
  0.0644998855573, 0.0644998855573,
  0.107362478065, 0.107362478065,
  0.126588845655, 0.126588845655,
  0.20498970016, 0.20498970016,
  0.292210269322, 0.292210269322,
  0.372838941024, 0.372838941024,
  0.492530708782, 0.492530708782,
  0.635324635691, 0.635324635691,
  0.805584802014, 0.805584802014,
  1.0, 1.0
};

static const float YM_dac_table[] = {
  0.0, 0.0,
  0.00465400167849, 0.00772106507973,
  0.0109559777218, 0.0139620050355,
  0.0169985503929, 0.0200198367285,
  0.024368657969, 0.029694056611,
  0.0350652323186, 0.0403906309606,
  0.0485389486534, 0.0583352407111,
  0.0680552376593, 0.0777752346075,
  0.0925154497597, 0.111085679408,
  0.129747463188, 0.148485542077,
  0.17666895552, 0.211551079576,
  0.246387426566, 0.281101701381,
  0.333730067903, 0.400427252613,
  0.467383840696, 0.53443198291,
  0.635172045472, 0.75800717174,
  0.879926756695, 1.0
};

static void reset_segment(struct ayumi* ay);

static int update_tone(struct ayumi* ay, int index) {
  struct tone_channel* ch = &ay->channels[index];
  ch->tone_counter += 1;
  if (ch->tone_counter >= ch->tone_period) {
    ch->tone_counter = 0;
    ch->tone ^= 1;
  }
  return ch->tone;
}

static int update_noise(struct ayumi* ay) {
  int bit0x3;
  ay->noise_counter += 1;
  if (ay->noise_counter >= (ay->noise_period << 1)) {
    ay->noise_counter = 0;
    bit0x3 = ((ay->noise ^ (ay->noise >> 3)) & 1);
    ay->noise = (ay->noise >> 1) | (bit0x3 << 16);
  }
  return ay->noise & 1;
}

static void slide_up(struct ayumi* ay) {
  ay->envelope += 1;
  if (ay->envelope > 31) {
    ay->envelope_segment ^= 1;
    reset_segment(ay);
  }
}

static void slide_down(struct ayumi* ay) {
  ay->envelope -= 1;
  if (ay->envelope < 0) {
    ay->envelope_segment ^= 1;
    reset_segment(ay);
  }
}

static void hold_top(struct ayumi* ay) {
  (void) ay;
}

static void hold_bottom(struct ayumi* ay) {
  (void) ay;
}

static void (* const Envelopes[][2])(struct ayumi*) = {
  {slide_down, hold_bottom},
  {slide_down, hold_bottom},
  {slide_down, hold_bottom},
  {slide_down, hold_bottom},
  {slide_up, hold_bottom},
  {slide_up, hold_bottom},
  {slide_up, hold_bottom},
  {slide_up, hold_bottom},
  {slide_down, slide_down},
  {slide_down, hold_bottom},
  {slide_down, slide_up},
  {slide_down, hold_top},
  {slide_up, slide_up},
  {slide_up, hold_top},
  {slide_up, slide_down},
  {slide_up, hold_bottom}
};

static void reset_segment(struct ayumi* ay) {
  if (Envelopes[ay->envelope_shape][ay->envelope_segment] == slide_down || Envelopes[ay->envelope_shape][ay->envelope_segment] == hold_top) {
    ay->envelope = 31;
    return;
  }
  ay->envelope = 0;
}

int update_envelope(struct ayumi* ay) {
  ay->envelope_counter += 1;
  if (ay->envelope_counter >= ay->envelope_period) {
    ay->envelope_counter = 0;
    Envelopes[ay->envelope_shape][ay->envelope_segment](ay);
  }
  return ay->envelope;
}

static void update_mixer(struct ayumi* ay) {
  int i;
  int out;
  int noise = update_noise(ay);
  int envelope = update_envelope(ay);
  ay->left = 0;
  ay->right = 0;
  for (i = 0; i < TONE_CHANNELS; i += 1) {
    out = (update_tone(ay, i) | ay->channels[i].t_off) & (noise | ay->channels[i].n_off);
    out *= ay->channels[i].e_on ? envelope : ay->channels[i].volume * 2 + 1;
    ay->left += ay->dac_table[out] * ay->channels[i].pan_left;
    ay->right += ay->dac_table[out] * ay->channels[i].pan_right;
  }
}

int ayumi_configure(struct ayumi* ay, int is_ym, float clock_rate, int sr) {
  int i;
  memset(ay, 0, sizeof(struct ayumi));
  ay->step = clock_rate / (sr * 8 * DECIMATE_FACTOR);
  ay->dac_table = is_ym ? YM_dac_table : AY_dac_table;
  ay->noise = 1;
  ay->filter_func = ayumi_filter_fast; // Default to fast filter
  ayumi_set_envelope(ay, 1);
  for (i = 0; i < TONE_CHANNELS; i += 1) {
    ayumi_set_tone(ay, i, 1);
  }
  return ay->step < 1;
}

void ayumi_set_chip_type(struct ayumi* ay, int is_ym) {
  ay->dac_table = is_ym ? YM_dac_table : AY_dac_table;
}

void ayumi_set_pan(struct ayumi* ay, int index, float pan, int is_eqp) {
  if (is_eqp) {
    ay->channels[index].pan_left = sqrt(1 - pan);
    ay->channels[index].pan_right = sqrt(pan);
  } else {
    ay->channels[index].pan_left = 1 - pan;
    ay->channels[index].pan_right = pan;
  }
}

void ayumi_set_tone(struct ayumi* ay, int index, int period) {
  period &= 0xfff;
  ay->channels[index].tone_period = (period == 0) | period;
}

void ayumi_set_noise(struct ayumi* ay, int period) {
  period &= 0x1f;
  ay->noise_period = (period == 0) | period;
}

void ayumi_set_mixer(struct ayumi* ay, int index, int t_off, int n_off, int e_on) {
  ay->channels[index].t_off = t_off & 1;
  ay->channels[index].n_off = n_off & 1;
  ay->channels[index].e_on = e_on;
}

void ayumi_set_volume(struct ayumi* ay, int index, int volume) {
  ay->channels[index].volume = volume & 0xf;
}

void ayumi_set_envelope(struct ayumi* ay, int period) {
  period &= 0xffff;
  ay->envelope_period = (period == 0) | period;
}

void ayumi_set_envelope_shape(struct ayumi* ay, int shape) {
  ay->envelope_shape = shape & 0xf;
  ay->envelope_counter = 0;
  ay->envelope_segment = 0;
  reset_segment(ay);
}

void ayumi_set_filter_quality(struct ayumi* ay, ayumi_filter_func filter_func) {
  ay->filter_func = filter_func;
}

void ayumi_process(struct ayumi* ay) {
  int i;
  float y1;
  float* c_left = ay->interpolator_left.c;
  float* y_left = ay->interpolator_left.y;
  float* c_right = ay->interpolator_right.c;
  float* y_right = ay->interpolator_right.y;
  float* fir_left = &ay->fir_left[FIR_SIZE - ay->fir_index * DECIMATE_FACTOR];
  float* fir_right = &ay->fir_right[FIR_SIZE - ay->fir_index * DECIMATE_FACTOR];
  ay->fir_index = (ay->fir_index + 1) % (FIR_SIZE / DECIMATE_FACTOR - 1);
  for (i = DECIMATE_FACTOR - 1; i >= 0; i -= 1) {
    ay->x += ay->step;
    if (ay->x >= 1) {
      ay->x -= 1;
      y_left[0] = y_left[1];
      y_left[1] = y_left[2];
      y_left[2] = y_left[3];
      y_right[0] = y_right[1];
      y_right[1] = y_right[2];
      y_right[2] = y_right[3];
      update_mixer(ay);
      y_left[3] = ay->left;
      y_right[3] = ay->right;
      y1 = y_left[2] - y_left[0];
      c_left[0] = 0.5 * y_left[1] + 0.25 * (y_left[0] + y_left[2]);
      c_left[1] = 0.5 * y1;
      c_left[2] = 0.25 * (y_left[3] - y_left[1] - y1);
      y1 = y_right[2] - y_right[0];
      c_right[0] = 0.5 * y_right[1] + 0.25 * (y_right[0] + y_right[2]);
      c_right[1] = 0.5 * y1;
      c_right[2] = 0.25 * (y_right[3] - y_right[1] - y1);
    }
    fir_left[i] = (c_left[2] * ay->x + c_left[1]) * ay->x + c_left[0];
    fir_right[i] = (c_right[2] * ay->x + c_right[1]) * ay->x + c_right[0];
  }
  ay->left = ay->filter_func(fir_left);
  ay->right = ay->filter_func(fir_right);
}

static float dc_filter(struct dc_filter* dc, int index, float x) {
  dc->sum += -dc->delay[index] + x;
  dc->delay[index] = x;
  return x - dc->sum / DC_FILTER_SIZE;
}

void ayumi_remove_dc(struct ayumi* ay) {
  ay->left = dc_filter(&ay->dc_left, ay->dc_index, ay->left);
  ay->right = dc_filter(&ay->dc_right, ay->dc_index, ay->right);
  ay->dc_index = (ay->dc_index + 1) & (DC_FILTER_SIZE - 1);
}
