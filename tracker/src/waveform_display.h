#ifndef __WAVEFORM_DISPLAY_H__
#define __WAVEFORM_DISPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "corelib_gfx.h"

/**
 * @brief Initialize waveform display system
 */
void waveformDisplayInit(void);

/**
 * @brief Get waveform bitmap for a track
 *
 * @param trackIdx Track index
 * @return Bitmap* Pointer to bitmap
 */
Bitmap* waveformDisplayGetBitmap(int trackIdx);

/**
 * @brief Render a sample waveform preview into a bitmap
 *
 * @param bitmap Target bitmap to render into (must be pre-allocated)
 * @param sampleData 8-bit unsigned sample data (128 = center)
 * @param startSample First sample index to render
 * @param endSample Last sample index to render (exclusive)
 */
void renderSamplePreview(Bitmap* bitmap, uint8_t* sampleData, uint16_t startSample, uint16_t endSample);

/**
 * @brief Render a wavetable preview into a bitmap
 *
 * @param bitmap Target bitmap to render into (must be pre-allocated)
 * @param wavetable 32 4-bit wavetable values (0-15)
 * @param isYM 1 for YM chip, 0 for AY chip (affects amplitude scaling)
 */
void renderAYWavetablePreview(Bitmap* bitmap, uint8_t* wavetable, int isYM);


#ifdef __cplusplus
}
#endif

#endif
