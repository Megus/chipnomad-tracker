#ifndef __SAMPLE_PREVIEW_H__
#define __SAMPLE_PREVIEW_H__

#include <stdint.h>
#include "corelib_gfx.h"

// Render a sample waveform preview into a bitmap
// Parameters:
//   bitmap: Target bitmap to render into (must be pre-allocated)
//   sampleData: 8-bit unsigned sample data (128 = center)
//   startSample: First sample index to render
//   endSample: Last sample index to render (exclusive)
void renderSamplePreview(Bitmap* bitmap, uint8_t* sampleData, uint16_t startSample, uint16_t endSample);

#endif
