#include "waveform_display.h"
#include "corelib_gfx.h"
#include "chipnomad_lib.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>

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

static void drawSlice(uint8_t* bitmap, int x, int height, int prevHeight, int hasTone, int hasNoise, int envHeight, int noiseShadeBase) {
  // Clamp heights to valid range
  if (height > charH - 1) height = charH - 1;
  if (prevHeight > charH - 1) prevHeight = charH - 1;
  if (envHeight > charH - 1) envHeight = charH - 1;

  int y = charH - 1 - height;
  int prevY = charH - 1 - prevHeight;

  if (hasTone) {
    // Draw pixel at height (or bottom if height is 0)
    bitmap[y * charW + x] = 255;
    // Connect to previous height
    int startY = y < prevY ? y : prevY;
    int endY = y > prevY ? y : prevY;
    for (int dy = startY; dy <= endY; dy++) {
      bitmap[dy * charW + x] = 255;
    }
    // If height is 0 but envHeight > 0, draw dim pixel at envelope height
    if (height == 0 && envHeight > 0) {
      int envY = charH - 1 - envHeight;
      bitmap[envY * charW + x] = 192;
    }
    // Fill with noise below if needed
    if (hasNoise && height > 0) {
      for (int dy = y + 1; dy < charH; dy++) {
        int noiseShade = noisePattern[noiseAnimIdx] ? noiseShadeBase : 64;
        noiseAnimIdx = (noiseAnimIdx + 1) & 511;
        bitmap[dy * charW + x] = noiseShade;
      }
    }
  } else if (hasNoise) {
    // Noise only - fill from bottom to height
    for (int dy = y; dy < charH; dy++) {
      int noiseShade = noisePattern[noiseAnimIdx] ? noiseShadeBase : 64;
      noiseAnimIdx = (noiseAnimIdx + 1) & 511;
      bitmap[dy * charW + x] = noiseShade;
    }
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
    int height = (volume * (charH - 1)) / 15;

    if (!hasTone && !hasNoise) {
      // Both disabled - horizontal line
      for (int x = 0; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, height, height, 1, 0, 0, 0);
      }
    } else if (hasTone && !hasNoise) {
      // Tone only - square wave
      for (int x = 0; x < charW / 2; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, height, height, 1, 0, 0, 0);
      }
      for (int x = charW / 2; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, 0, x == charW / 2 ? height : 0, 1, 0, 0, 0);
      }
    } else if (!hasTone && hasNoise) {
      // Noise only
      for (int x = 0; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, height, height, 0, 1, 0, noiseShadeBase);
      }
    } else {
      // Tone + noise - first half with both, second half zero
      for (int x = 0; x < charW / 2; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, height, height, 1, 1, 0, noiseShadeBase);
      }
      for (int x = charW / 2; x < charW; x++) {
        drawSlice(waveformBitmaps[trackIdx], x, 0, x == charW / 2 ? height : 0, 1, 1, 0, noiseShadeBase);
      }
    }
  } else {
    // Envelope enabled
    uint8_t envShape = chip->regs[13];
    int prevHeight = -1; // -1 means no previous height yet

    for (int x = 0; x < charW; x++) {
      int envHeight = getEnvelopeHeight(x, envShape);
      int height = envHeight;

      // Mix with tone if enabled (2 periods across width)
      if (hasTone) {
        // Calculate position within 2 full periods (0.0 to 2.0)
        float phase = (x * 2.0f) / charW;
        // Get fractional part to determine if we're in high or low part of current period
        float periodPhase = phase - (int)phase;
        int pulseHigh = periodPhase < 0.5f;
        height = pulseHigh ? envHeight : 0;
      }

      // Use current height as prevHeight for first pixel
      int usePrevHeight = (prevHeight == -1) ? height : prevHeight;

      drawSlice(waveformBitmaps[trackIdx], x, height, usePrevHeight,
                hasTone || (!hasTone && !hasNoise), hasNoise, envHeight, noiseShadeBase);
      prevHeight = height;
    }
  }

  return waveformBitmaps[trackIdx];
}
