#include "sample_utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Calculate one-pole filter coefficient from frequency and sample rate.
 * Returns a value in range 0-256 for fixed-point math (8.8 format).
 */
static uint16_t calc_one_pole_coeff(float freqHz, float sampleRate) {
  // coefficient = exp(-2π * freq / rate)
  float coeff = expf(-2.0f * M_PI * freqHz / sampleRate);
  // Convert to 8.8 fixed point (0-256 range)
  return (uint16_t)(coeff * 256.0f + 0.5f);
}

/**
 * Track how far the waveform dips below zero (the trough depth).
 *
 * This is the "lower envelope" from wav2ays: an asymmetric one-pole follower
 * applied to the NEGATIVE of the signal. Fast attack catches each trough,
 * slow release lets it ride a decaying tail.
 *
 * Input: signed 8-bit samples (-128 to +127, where 0 is center)
 * Output: non-negative envelope showing how far below zero the signal dips
 */
static void lower_envelope_signed(const int8_t* samples, uint8_t* env, int length,
                                  uint16_t attackCoeff, uint16_t releaseCoeff) {
  int32_t state = 0;  // 16.8 fixed point

  for (int i = 0; i < length; i++) {
    // Negate the sample: we track positive values of -sig
    int32_t neg_sample = -samples[i];

    // Clamp to non-negative (we only care about dips below zero)
    if (neg_sample < 0) neg_sample = 0;

    int32_t target = neg_sample << 8;  // Convert to 16.8 fixed point

    // Asymmetric one-pole filter
    if (target > state) {
      // Attack: fast response when signal dips lower (neg_sample increases)
      state = ((state * attackCoeff) >> 8) + ((target * (256 - attackCoeff)) >> 8);
    } else {
      // Release: slow rise when signal comes back up (neg_sample decreases)
      state = ((state * releaseCoeff) >> 8) + ((target * (256 - releaseCoeff)) >> 8);
    }

    env[i] = (uint8_t)(state >> 8);  // Convert back to 8-bit
  }
}

/**
 * Simple multi-pass exponential smoothing for envelope.
 * Each pass applies a one-pole low-pass filter.
 * More passes = smoother result.
 */
static void smooth_envelope(uint8_t* env, int length, int passes) {
  if (passes <= 0) return;

  // Smoothing coefficient (0.75 = moderate smoothing)
  const uint16_t smoothCoeff = 192;  // 192/256 = 0.75

  for (int pass = 0; pass < passes; pass++) {
    int32_t state = env[0] << 8;  // 16.8 fixed point

    for (int i = 0; i < length; i++) {
      // state = smoothCoeff * state + (256 - smoothCoeff) * env[i]
      state = ((state * smoothCoeff) >> 8) + (((env[i] << 8) * (256 - smoothCoeff)) >> 8);
      env[i] = (uint8_t)(state >> 8);
    }
  }
}

void sampleLiftToZero(uint8_t* sampleData, uint16_t sampleLength, uint16_t sampleRate) {
  if (!sampleData || sampleLength == 0 || sampleRate == 0) {
    return;
  }

  // Allocate buffers
  int8_t* signed_samples = (int8_t*)malloc(sampleLength);
  uint8_t* env = (uint8_t*)malloc(sampleLength);
  int16_t* lifted_samples = (int16_t*)malloc(sampleLength * sizeof(int16_t));
  if (!signed_samples || !env || !lifted_samples) {
    free(signed_samples);
    free(env);
    free(lifted_samples);
    return;
  }

  // Convert unsigned (0-255, center=128) to signed (-128 to +127, center=0)
  for (int i = 0; i < sampleLength; i++) {
    signed_samples[i] = (int8_t)(sampleData[i] - 128);
  }

  // Calculate filter coefficients (from wav2ays defaults)
  // Attack: 400Hz (fast response to troughs)
  // Release: 15Hz (slow rise for smooth tails)
  uint16_t attackCoeff = calc_one_pole_coeff(400.0f, (float)sampleRate);
  uint16_t releaseCoeff = calc_one_pole_coeff(15.0f, (float)sampleRate);

  // Track the lower envelope (how far below zero the signal dips)
  lower_envelope_signed(signed_samples, env, sampleLength, attackCoeff, releaseCoeff);

  // Smooth the envelope (wav2ays uses a 2nd-order Butterworth at 60Hz)
  // We'll approximate with 3 passes of our simple smoother
  smooth_envelope(env, sampleLength, 3);

  // Apply antidc_bend: ADD the envelope to lift the waveform
  // This brings the trough up to 0 (in signed space)
  // Use int16_t to avoid overflow (signed_samples can be -128, env can be up to 128)
  for (int i = 0; i < sampleLength; i++) {
    lifted_samples[i] = (int16_t)signed_samples[i] + (int16_t)env[i];
  }

  // Convert to unsigned 8-bit, clamping to [0, 255]
  for (int i = 0; i < sampleLength; i++) {
    int16_t val = lifted_samples[i];
    if (val < 0) val = 0;
    if (val > 255) val = 255;

    sampleData[i] = (uint8_t)val;
  }

  free(signed_samples);
  free(env);
  free(lifted_samples);
}
