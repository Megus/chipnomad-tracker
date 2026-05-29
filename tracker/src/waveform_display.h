#ifndef __WAVEFORM_DISPLAY_H__
#define __WAVEFORM_DISPLAY_H__

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

#endif
