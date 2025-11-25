#include <stdio.h>
#include <ayumi.h>
#include <audio_manager.h>

#include "psg_play.h"

static uint8_t* psgData = NULL;
int psgOffset = 0;

int r[14]; // AY Registers

int frameCountdown = 0;

int psgReadFile(char* path) {
  if (psgData != NULL) {
    free(psgData);
  }

  FILE* psgFile = fopen(path, "rb");
  fseek(psgFile, 0, SEEK_END);
  int psgSize = ftell(psgFile);
  fseek(psgFile, 0, SEEK_SET);

  psgData = malloc(psgSize);
  (void)!fread(psgData, psgSize, 1, psgFile);
  fclose(psgFile);

  psgOffset = 16;
  frameCountdown = 0;

  // Clear registers
  for (int c = 0; c < 14; c++) {
    r[c] = 0;
  }

  return 0;
}

void psgFrameCallback(void *userdata) {
  struct ayumi* ay = audioManager.chips[0].userdata;

  if (frameCountdown > 0) {
    frameCountdown--;
    return;
  }

  if (psgData[psgOffset] == 0xff) {
    // Frame data
    psgOffset++;
    while (psgData[psgOffset] < 0xfe) {
      if (psgData[psgOffset] < 14) {
        r[psgData[psgOffset]] = psgData[psgOffset + 1];
      }
      psgOffset += 2;
    }
  } else {
    // Delay (0xfe)
    frameCountdown = psgData[psgOffset + 1] * 4;
    psgOffset += 2;
  }

  // Update Ayumi
  ayumi_set_tone(ay, 0, (r[1] << 8) | r[0]);
  ayumi_set_tone(ay, 1, (r[3] << 8) | r[2]);
  ayumi_set_tone(ay, 2, (r[5] << 8) | r[4]);
  ayumi_set_noise(ay, r[6]);
  ayumi_set_mixer(ay, 0, r[7] & 1, (r[7] >> 3) & 1, r[8] >> 4);
  ayumi_set_mixer(ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, r[9] >> 4);
  ayumi_set_mixer(ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, r[10] >> 4);
  ayumi_set_volume(ay, 0, r[8] & 0xf);
  ayumi_set_volume(ay, 1, r[9] & 0xf);
  ayumi_set_volume(ay, 2, r[10] & 0xf);
  ayumi_set_envelope(ay, (r[12] << 8) | r[11]);
  if (r[13] != 255) {
    ayumi_set_envelope_shape(ay, r[13]);
  }

  // Drop r13 to avoid env retrigs
  r[13] = 255;

  if (psgOffset == sizeof(psgData)) {
    psgOffset = 16;
    frameCountdown = 0;
  }

}
