#include "waveform_display.h"
#include "corelib_gfx.h"
#include "chipnomad_lib.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>

#define ENVELOPE_DIM_BRIGHTNESS 160

static uint8_t* emptyBitmap = NULL;
static uint8_t* waveformBitmaps[PROJECT_MAX_TRACKS];
static int bitmapSize = 0;
static int charW = 0;
static int charH = 0;
static uint8_t noisePattern[512];
static int noiseAnimIdx = 0;

void waveformDisplayInit(void) {
  charW = gfxGetCharWidth();
  charH = gfxGetCharHeight();
  bitmapSize = charW * charH;

  emptyBitmap = malloc(bitmapSize);
  if (emptyBitmap) {
    memset(emptyBitmap, 0, bitmapSize);
  }

  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    waveformBitmaps[i] = malloc(bitmapSize);
    if (waveformBitmaps[i]) {
      memset(waveformBitmaps[i], 0, bitmapSize);
    }
  }

  for (int i = 0; i < 512; i++) {
    noisePattern[i] = rand() & 1;
  }
}

static void drawVerticalLine(uint8_t* bitmap, int x, int y1, int y2, uint8_t shade) {
  int startY = y1 < y2 ? y1 : y2;
  int endY = y1 > y2 ? y1 : y2;
  for (int dy = startY; dy <= endY; dy++) {
    bitmap[dy * charW + x] = shade;
  }
}

static void drawSlice(uint8_t* bitmap, int x, int amplitude, int toneHigh, int hasEnvelope, int hasNoise, int noiseShadeBase) {
  static int prevAmplitude = -1;
  static int prevToneHigh = 1;
  static int prevEnvAmplitude = -1;

  if (x == 0) {
    prevAmplitude = -1;
    prevToneHigh = 1;
    prevEnvAmplitude = -1;
  }

  if (amplitude > charH - 1) amplitude = charH - 1;

  int y = charH - 1 - amplitude;
  int lowY = charH - 1;

  if (toneHigh) {
    bitmap[y * charW + x] = 255;

    if (prevAmplitude >= 0) {
      int prevY = charH - 1 - prevAmplitude;
      if (prevToneHigh) {
        drawVerticalLine(bitmap, x, y, prevY, 255);
      } else {
        drawVerticalLine(bitmap, x, y, prevY, 255);
      }
    }
  } else {
    bitmap[lowY * charW + x] = 255;

    if (hasEnvelope) {
      bitmap[y * charW + x] = ENVELOPE_DIM_BRIGHTNESS;
      if (prevEnvAmplitude >= 0) {
        int prevY = charH - 1 - prevEnvAmplitude;
        drawVerticalLine(bitmap, x, y, prevY, ENVELOPE_DIM_BRIGHTNESS);
      }
    }

    if (prevToneHigh && prevAmplitude >= 0) {
      int prevY = charH - 1 - prevAmplitude;
      drawVerticalLine(bitmap, x, lowY, prevY, 255);
    }
  }

  if (hasNoise && toneHigh && amplitude > 0) {
    for (int dy = y + 1; dy < charH; dy++) {
      int noiseShade = noisePattern[noiseAnimIdx] ? noiseShadeBase : 64;
      noiseAnimIdx = (noiseAnimIdx + 1) & 511;
      bitmap[dy * charW + x] = noiseShade;
    }
  }

  prevAmplitude = toneHigh ? amplitude : 0;
  prevToneHigh = toneHigh;
  if (hasEnvelope) {
    prevEnvAmplitude = amplitude;
  }
}

static int getEnvelopeHeight(int x, int envShape) {
  int shape = envShape & 0x0F;
  int halfW = charW / 2;
  int maxH = charH - 1;
  int isFirstHalf = x < halfW;
  int decay = maxH - (x * maxH) / halfW;
  int attack = (x * maxH) / halfW;
  int decay2 = maxH - ((x - halfW) * maxH) / halfW;
  int attack2 = ((x - halfW) * maxH) / halfW;

  if (shape <= 3 || shape == 9) return isFirstHalf ? decay : 0;
  if ((shape >= 4 && shape <= 7) || shape == 15) return isFirstHalf ? attack : 0;
  if (shape == 8) return isFirstHalf ? decay : decay2;
  if (shape == 10) return isFirstHalf ? decay : attack2;
  if (shape == 11) return isFirstHalf ? decay : maxH;
  if (shape == 12) return isFirstHalf ? attack : attack2;
  if (shape == 13) return isFirstHalf ? attack : maxH;
  if (shape == 14) return isFirstHalf ? attack : decay2;
  return 0;
}

uint8_t* waveformDisplayGetBitmap(int trackIdx) {
  PlaybackTrackState* track = &chipnomadState->playbackState.tracks[trackIdx];

  // Check if track is playing
  if (track->note.noteFinal == EMPTY_VALUE_8) {
    return emptyBitmap;
  }

  // Determine which chip and channel this track belongs to
  int chipIdx = trackIdx / 3;
  int ayChannel = trackIdx % 3;

  SoundChip* chip = &chipnomadState->chips[chipIdx];

  // Read mixer register (reg 7)
  uint8_t mixerReg = chip->regs[7];
  int hasTone = !((mixerReg >> ayChannel) & 1);
  int hasNoise = !((mixerReg >> (ayChannel + 3)) & 1);

  // Read volume register (reg 8/9/10)
  uint8_t volumeReg = chip->regs[8 + ayChannel];
  int envEnabled = (volumeReg & 0x10) != 0;

  // Read noise period (reg 6, lower 5 bits)
  uint8_t noisePeriod = chip->regs[6] & 0x1F;
  int noiseShadeBase = 128 + noisePeriod * 2;

  memset(waveformBitmaps[trackIdx], 0, bitmapSize);

  if (!envEnabled) {
    // Simple volume-based waveform
    int volume = volumeReg & 0x0F;
    int amplitude = (volume * (charH - 1)) / 15;

    if (!hasTone && !hasNoise) {
      // Both disabled - horizontal line (tone always HIGH)
      for (int x = 0; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, amplitude, 1, 0, 0, 0);
      }
    } else if (hasTone && !hasNoise) {
      // Tone only - square wave
      for (int x = 0; x < charW / 2; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, amplitude, 1, 0, 0, 0);
      }
      for (int x = charW / 2; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, amplitude, 0, 0, 0, 0);
      }
    } else if (!hasTone && hasNoise) {
      // Noise only (tone always HIGH)
      for (int x = 0; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, amplitude, 1, 0, 1, noiseShadeBase);
      }
    } else {
      // Tone + noise - square wave with noise
      for (int x = 0; x < charW / 2; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, amplitude, 1, 0, 1, noiseShadeBase);
      }
      for (int x = charW / 2; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, amplitude, 0, 0, 1, noiseShadeBase);
      }
    }
  } else {
    // Envelope enabled
    uint8_t envShape = chip->regs[13];

    for (int x = 0; x < charW; x++) {
      int amplitude = getEnvelopeHeight(x, envShape);

      // Determine tone state (2 periods across width)
      int toneHigh = 1; // Default HIGH when tone disabled
      if (hasTone) {
        float phase = (x * 2.0f) / charW;
        float periodPhase = phase - (int)phase;
        toneHigh = periodPhase < 0.5f;
      }

      drawSlice(waveformBitmaps[trackIdx], x, amplitude, toneHigh, 1, hasNoise, noiseShadeBase);
    }
  }

  return waveformBitmaps[trackIdx];
}
