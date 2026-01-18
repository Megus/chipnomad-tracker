#ifndef __WAVEFORM_DISPLAY_H__
#define __WAVEFORM_DISPLAY_H__

#include <stdint.h>

/**
 * @brief Initialize waveform display system
 */
void waveformDisplayInit(void);

/**
 * @brief Get waveform bitmap for a track
 * 
 * @param trackIdx Track index
 * @return uint8_t* Pointer to bitmap data
 */
uint8_t* waveformDisplayGetBitmap(int trackIdx);

#endif
