#include "oscilloscope.h"
#include "corelib_gfx.h"
#include "common.h"
#include "chipnomad_lib.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

#define OSC_RING_SIZE    4096  // Samples per track (power of two)
#define OSC_PIXEL_SCALE  2     // Horizontal pixels per waveform column
#define OSC_FADE_STEP    4     // Vertical pixels per fade sub-segment (smaller = smoother)

static std::atomic<uint32_t> gWriteIndex{0};
static float gRing[PROJECT_MAX_TRACKS][OSC_RING_SIZE];
static int gW = 0;        // Strip width in pixels
static int gH = 0;        // Strip height in pixels
static int gTopY = 0;     // Strip top Y in pixels
static int gSampleRate = 44100;

void oscilloscopeInit(int sampleRate) {
  gSampleRate = sampleRate > 0 ? sampleRate : 44100;

  int charW = gfxGetCharWidth();
  int charH = gfxGetCharHeight();
  int offsetY = gfxGetOffsetY();
  if (offsetY < 0) offsetY = 0;

  gW = gfxGetScreenWidth();
  // Strip starts OSCILLOSCOPE_OVERLAP_ROWS table rows above the bottom and
  // extends down to the physical bottom of the screen.
  gTopY = (19 - OSCILLOSCOPE_OVERLAP_ROWS) * charH + offsetY;
  gH = gfxGetScreenHeight() - gTopY;
  if (gH < charH) gH = charH;

  memset(gRing, 0, sizeof(gRing));
  gWriteIndex.store(0, std::memory_order_relaxed);
}

void oscilloscopePushSamples(float** trackBuffers, int trackCount, int frames) {
  if (frames <= 0 || trackCount <= 0) return;

  const uint32_t mask = OSC_RING_SIZE - 1;
  uint32_t base = gWriteIndex.load(std::memory_order_relaxed);

  for (int t = 0; t < trackCount && t < PROJECT_MAX_TRACKS; t++) {
    if (!trackBuffers[t]) continue;
    float* ring = gRing[t];
    for (int i = 0; i < frames; i++) {
      ring[(base + i) & mask] = trackBuffers[t][i];
    }
  }

  gWriteIndex.store(base + frames, std::memory_order_release);
}

// Linear vertical fade: 0 (transparent) at the top, 255 (opaque) at the bottom.
static int fadeAlpha(int y) {
  float yNorm = (float)(y - gTopY) / (float)(gH - 1);
  if (yNorm < 0.0f) yNorm = 0.0f;
  if (yNorm > 1.0f) yNorm = 1.0f;
  int alpha = (int)(255.0f * yNorm);
  if (alpha < 0) alpha = 0;
  if (alpha > 255) alpha = 255;
  return alpha;
}

// Draw a line with a per-Y exponential fade. Steep lines are split into
// sub-segments so vertical transitions fade smoothly too (not a single color).
static void drawFadedLine(int x1, int y1, int x2, int y2) {
  int dy = y2 - y1;
  int ady = dy < 0 ? -dy : dy;
  int steps = ady / OSC_FADE_STEP + 1;

  for (int i = 0; i < steps; i++) {
    int sx1 = x1 + ((x2 - x1) * i) / steps;
    int sy1 = y1 + (dy * i) / steps;
    int sx2 = x1 + ((x2 - x1) * (i + 1)) / steps;
    int sy2 = y1 + (dy * (i + 1)) / steps;
    int midY = (sy1 + sy2) / 2;
    gfxDrawLineAlpha(sx1, sy1, sx2, sy2, fadeAlpha(midY));
  }
}

void oscilloscopeDraw(void) {
  if (gW <= 0 || gH <= 0 || !chipnomadState) return;

  int trackCount = chipnomadState->project.tracksCount;
  if (trackCount > PROJECT_MAX_TRACKS) trackCount = PROJECT_MAX_TRACKS;

  const uint32_t mask = OSC_RING_SIZE - 1;
  uint32_t end = gWriteIndex.load(std::memory_order_acquire);

  int cols = gW / OSC_PIXEL_SCALE;
  if (cols < 1) cols = 1;
  uint32_t start = end - cols;

  for (int t = 0; t < trackCount; t++) {
    gfxSetFgColor(appSettings.colorScheme.oscColors[t]);
    float* ring = gRing[t];

    int prevX = 0;
    int prevY = -1;
    for (int x = 0; x < cols; x++) {
      float s = ring[(start + x) & mask];
      // s is the AY DAC value in [0, 1]; map 0 -> bottom, 1 -> top.
      int y = gTopY + gH - 1 - (int)(s * (float)(gH - 1));
      int cx = x * OSC_PIXEL_SCALE;
      if (prevY >= 0) {
        drawFadedLine(prevX, prevY, cx, y);
      }
      prevX = cx;
      prevY = y;
    }
  }
}
