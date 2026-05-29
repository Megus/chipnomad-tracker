#ifndef __SAMPLE_UTILS_H__
#define __SAMPLE_UTILS_H__

#include <stdint.h>

/**
 * Lift the sample's lower envelope to zero for unipolar playback.
 *
 * The AY/YM output is unipolar (0-15), but samples are bipolar (centered at 128).
 * This function tracks the waveform's lower envelope (how far it dips below zero)
 * and lifts it so the trough rides at zero. This allows:
 * - Quiet tails to decay to true silence (code 0) instead of a loud mid-DAC pedestal
 * - Full waveform shape is preserved (no clipping or distortion)
 * - Better use of the AY's non-linear amplitude range
 *
 * Based on the "antidc_bend" algorithm from wav2ays by Ru Grantez.
 * Simplified to work entirely in 8-bit integer math for minimal quality loss.
 *
 * @param sampleData    Pointer to 8-bit unsigned sample data (128 = center)
 * @param sampleLength  Number of samples
 * @param sampleRate    Sample rate in Hz (used to calculate filter coefficients)
 *
 * Note: Modifies sampleData in-place.
 */
void sampleLiftToZero(uint8_t* sampleData, uint16_t sampleLength, uint16_t sampleRate);

#endif
