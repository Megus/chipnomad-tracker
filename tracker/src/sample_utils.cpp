#include "sample_utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Fixed parameters for sample lift algorithm
#define ATTACK_FREQ_HZ 200.0f
#define RELEASE_FREQ_HZ 50.0f
#define SMOOTH_PASSES 4
#define SMOOTH_COEFF 192  // 192/256 = 0.75

/**
 * Calculate one-pole filter coefficient from frequency and sample rate.
 * Returns a value in range 0-256 for fixed-point math (8.8 format).
 * coefficient = exp(-2π * freq / rate)
 */
static uint16_t calc_one_pole_coeff(float freqHz, float sampleRate) {
  float coeff = expf(-2.0f * 3.141592653589793f * freqHz / sampleRate);
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
 *
 * Look-ahead feature: Scans ahead to anticipate deep troughs, helping with
 * samples that start loud or have sudden transients.
 */
static void lower_envelope_signed(const int8_t* samples, uint8_t* env, int length,
                                  uint16_t attackCoeff, uint16_t releaseCoeff,
                                  int lookAheadSamples) {
  int32_t state = 0;  // 16.8 fixed point

  // Optional: Initialize state with look-ahead to handle loud starts
  if (lookAheadSamples > 0) {
    // Find the maximum negative value (deepest trough) in the initial window
    int32_t max_neg = 0;
    int scan_length = (lookAheadSamples < length) ? lookAheadSamples : length;
    for (int i = 0; i < scan_length; i++) {
      int32_t neg_sample = -samples[i];
      if (neg_sample < 0) neg_sample = 0;
      if (neg_sample > max_neg) max_neg = neg_sample;
    }
    state = max_neg << 8;  // Initialize state to the deepest trough found
  }

  for (int i = 0; i < length; i++) {
    // Negate the sample: we track positive values of -sig
    int32_t neg_sample = -samples[i];

    // Clamp to non-negative (we only care about dips below zero)
    if (neg_sample < 0) neg_sample = 0;

    // Look-ahead: check upcoming samples for deeper troughs
    if (lookAheadSamples > 0) {
      int32_t max_ahead = neg_sample;
      int ahead_limit = (i + lookAheadSamples < length) ? lookAheadSamples : (length - i - 1);
      for (int j = 1; j <= ahead_limit; j++) {
        int32_t ahead_sample = -samples[i + j];
        if (ahead_sample < 0) ahead_sample = 0;
        if (ahead_sample > max_ahead) max_ahead = ahead_sample;
      }
      neg_sample = max_ahead;
    }

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
 * Each pass applies a one-pole low-pass filter with fixed coefficient (0.75).
 */
static void smooth_envelope(uint8_t* env, int length) {
  for (int pass = 0; pass < SMOOTH_PASSES; pass++) {
    int32_t state = env[0] << 8;  // 16.8 fixed point

    for (int i = 0; i < length; i++) {
      state = ((state * SMOOTH_COEFF) >> 8) + (((env[i] << 8) * (256 - SMOOTH_COEFF)) >> 8);
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

  // Calculate filter coefficients for fixed frequencies
  uint16_t attackCoeff = calc_one_pole_coeff(ATTACK_FREQ_HZ, (float)sampleRate);
  uint16_t releaseCoeff = calc_one_pole_coeff(RELEASE_FREQ_HZ, (float)sampleRate);

  // Calculate look-ahead window: sampleRate / 40
  int lookAheadSamples = sampleRate / 40;

  // Track the lower envelope (how far below zero the signal dips)
  lower_envelope_signed(signed_samples, env, sampleLength, attackCoeff, releaseCoeff, lookAheadSamples);

  // Smooth the envelope with fixed number of passes
  smooth_envelope(env, sampleLength);

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
