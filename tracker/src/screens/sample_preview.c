#include "sample_preview.h"
#include <string.h>

// Helper: convert sample value (0-255) to Y coordinate
static inline int sampleToY(uint8_t sample, int height) {
  return ((255 - sample) * (height - 1)) / 255;
}

void renderSamplePreview(Bitmap* bitmap, uint8_t* sampleData, uint16_t startSample, uint16_t endSample) {
  if (!bitmap || !sampleData || startSample >= endSample) {
    // Clear bitmap if invalid
    if (bitmap) {
      memset(bitmap->data, 0, bitmap->widthPixels * bitmap->heightPixels);
    }
    return;
  }

  // Clear bitmap
  memset(bitmap->data, 0, bitmap->widthPixels * bitmap->heightPixels);

  int width = bitmap->widthPixels;
  int height = bitmap->heightPixels;
  int centerY = height / 2;

  // Draw center line
  int centerLineY = centerY;
  for (int x = 0; x < width; x++) {
    bitmap->data[centerLineY * width + x] = 64;
  }

  uint16_t sampleCount = endSample - startSample;

  // Handle zero-length range: just show center line
  if (sampleCount == 0) {
    return;
  }

  // Draw waveform by connecting sample ranges
  int prevMinY = -1;
  int prevMaxY = -1;

  for (int x = 0; x < width; x++) {
    // Calculate sample range for this pixel column
    uint16_t sampleStart = startSample + (x * sampleCount) / width;
    uint16_t sampleEnd = startSample + ((x + 1) * sampleCount) / width;

    // Handle the range of samples for this pixel column
    if (sampleStart == sampleEnd) {
      // Stretched case: same sample for multiple pixels
      if (sampleStart < endSample) {
        sampleEnd = sampleStart + 1;
      }
    }

    // Find min and max in this range
    uint8_t minVal = 255;
    uint8_t maxVal = 0;

    for (uint16_t i = sampleStart; i < sampleEnd && i < endSample; i++) {
      uint8_t val = sampleData[i];
      if (val < minVal) minVal = val;
      if (val > maxVal) maxVal = val;
    }

    // Convert to Y coordinates
    int minY = sampleToY(maxVal, height);  // maxVal -> lower Y (inverted)
    int maxY = sampleToY(minVal, height);  // minVal -> higher Y

    // Draw vertical line for this column
    for (int y = minY; y <= maxY; y++) {
      if (y >= 0 && y < height) {
        bitmap->data[y * width + x] = 255;
      }
    }

    // Connect to previous column by filling the gap between ranges
    if (prevMinY >= 0 && x > 0) {
      // Find the gap between previous column's range [prevMinY, prevMaxY]
      // and current column's range [minY, maxY]
      int gapStart, gapEnd;

      if (prevMaxY < minY) {
        // Gap below: previous range is above current range
        gapStart = prevMaxY;
        gapEnd = minY;
      } else if (maxY < prevMinY) {
        // Gap above: previous range is below current range
        gapStart = maxY;
        gapEnd = prevMinY;
      } else {
        // Ranges overlap or touch - no gap to fill
        gapStart = 0;
        gapEnd = -1;
      }

      // Fill the gap in the previous column (x-1)
      for (int y = gapStart; y <= gapEnd; y++) {
        if (y >= 0 && y < height) {
          bitmap->data[y * width + (x - 1)] = 255;
        }
      }
    }

    // Remember range for next iteration
    prevMinY = minY;
    prevMaxY = maxY;
  }
}
