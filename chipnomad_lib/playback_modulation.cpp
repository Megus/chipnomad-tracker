#include "playback_modulation.h"
#include "utils.h"
#include <math.h>
#include <stdint.h>

// Modulation range: 255 * 127 = 32385
// This allows exact division by 255 (sustain range) and 127 (max amount)
// Using 127 instead of 128 ensures that max amount (127) reaches full range
#define MOD_MAX_RANGE 32385
#define MOD_MAX_RANGE_F 32385.0f

// Helper macros to read parameters with offsets and clamping
// Intermediate calculation happens in 16-bit, then clamped to valid range
#define GET_P1(state) clampInt16((int16_t)(state)->modulation->p1 + (state)->p1Offset, 0, 255)
#define GET_P2(state) clampInt16((int16_t)(state)->modulation->p2 + (state)->p2Offset, 0, 255)
#define GET_P3(state) clampInt16((int16_t)(state)->modulation->p3 + (state)->p3Offset, 0, 255)
#define GET_P4(state) clampInt16((int16_t)(state)->modulation->p4 + (state)->p4Offset, 0, 255)
#define GET_TYPE(state) ((state)->modulation->type)
#define GET_AMOUNT(state) clampInt16((int16_t)(state)->modulation->amount + (state)->amountOffset, -128, 127)
#define GET_DESTINATION(state) ((state)->modulation->destination)

static void handleADSR(PlaybackModState* state) {
  // step: 0 - Attack, 1 - Decay, 2 - Sustain, 3 - Release

  int16_t envelopeValue = 0;

  while (1) {
    if (state->step > 3) break; // Just in case...

    // Sustain phase
    if (state->step == 2) {
      // Scale sustain (0-255) to 0-32385 range (exact: 255 * 127 = 32385)
      envelopeValue = GET_P3(state) * 127;
      break;
    }

    int duration = 0;
    if (state->step == 0) duration = GET_P1(state);  // Attack
    else if (state->step == 1) duration = GET_P2(state);  // Decay
    else if (state->step == 3) duration = GET_P4(state);  // Release

    if (duration == 0 || state->counter >= duration) {
      // Move to the next ADSR step
      if (state->step == 3) {
        // Release phase end
        envelopeValue = 0;
        state->step = 0xff; // Mark as stopped
        break;
      } else {
        state->step++;
        state->counter = 0;
        // Set up next phase
        if (state->step == 1) {
          // Decay: from peak to sustain
          state->data1 = MOD_MAX_RANGE;
          state->data2 = GET_P3(state) * 127;
        }
        // Continue loop to handle zero-duration phases or enter sustain
      }
    } else {
      // LERP between data1 and data2
      state->counter++;
      int32_t from = state->data1;
      int32_t to = state->data2;
      int32_t delta = to - from;
      // When counter reaches duration, output the target value
      if (state->counter >= duration) {
        envelopeValue = to;
      } else {
        envelopeValue = from + ((delta * ((state->counter << 8) / (duration + 1))) >> 8);
      }
      break;
    }
  }

  // Apply amount scaling: amount is -128 to 127
  // Scale envelope (0-32385) by amount to get final signed 16-bit value
  // Using 127 as divisor for exact division (32385 / 127 = 255)
  int32_t scaled = (envelopeValue * GET_AMOUNT(state)) / 127;
  state->outValue = (int16_t)scaled;
}

static void handleADSRnoteOff(PlaybackModState* state) {
  // If the note is turned off, jump to release phase
  if (state->step < 3) {
    state->step = 3;
    state->counter = 0;
    // Set release from the current envelope value (before amount scaling)
    // Reverse the amount scaling to get the envelope value
    int8_t amount = GET_AMOUNT(state);
    int32_t currentEnvelope = (state->outValue * 127) / (amount != 0 ? amount : 1);
    state->data1 = (int16_t)currentEnvelope;
    state->data2 = 0; // Release to 0
  }
}

static void handleAHD(PlaybackModState* state) {
  // step: 0 - Attack, 1 - Hold, 2 - Decay
  // Output range: 0 to 32385 (255 * 127)
  // All three phases are treated uniformly as ramps (Hold is a ramp from 32385 to 32385)

  int16_t envelopeValue = 0;

  while (1) {
    if (state->step > 2) {
      // Envelope stopped
      envelopeValue = 0;
      break;
    }

    // Get duration for current phase
    int duration = 0;
    if (state->step == 0) duration = GET_P1(state);      // Attack
    else if (state->step == 1) duration = GET_P2(state); // Hold
    else if (state->step == 2) duration = GET_P3(state); // Decay

    if (duration == 0 || state->counter >= duration) {
      // Phase complete - move to the next phase
      if (state->step == 2) {
        // Decay phase end - stop
        envelopeValue = 0;
        state->step = 0xff; // Mark as stopped
        break;
      } else {
        // Move to next phase
        state->step++;
        state->counter = 0;

        // Set up ramp for next phase
        if (state->step == 1) {
          // Hold: 32385 to 32385
          state->data1 = MOD_MAX_RANGE;
          state->data2 = MOD_MAX_RANGE;
        } else if (state->step == 2) {
          // Decay: 32385 to 0
          state->data1 = MOD_MAX_RANGE;
          state->data2 = 0;
        }
        // Continue loop to calculate first value of new phase
      }
    } else {
      // Continue current phase - LERP between data1 and data2
      state->counter++;
      int32_t from = state->data1;
      int32_t to = state->data2;
      int32_t delta = to - from;
      // When counter reaches duration, output the target value
      if (state->counter >= duration) {
        envelopeValue = to;
      } else {
        envelopeValue = from + ((delta * ((state->counter << 8) / (duration + 1))) >> 8);
      }
      break;
    }
  }

  // Apply amount scaling: amount is -128 to 127
  // Scale envelope (0-32385) by amount to get final signed 16-bit value
  int32_t scaled = (envelopeValue * GET_AMOUNT(state)) / 127;
  state->outValue = (int16_t)scaled;
}

static void handleAHDnoteOff(PlaybackModState* state) {
  // Do nothing - AHD is not affected by note off
}

static void handleLFO(PlaybackModState* state) {
  // LFO parameters:
  // p1: shape (0-7)
  // p2: trigger mode (0-3)
  // p3: period in ticks
  // counter: current position in cycle
  // step: 0 = running, 0xff = stopped (for lfoOnce and lfoHold)

  uint8_t shape = GET_P1(state);
  uint8_t trigger = GET_P2(state);
  uint8_t period = GET_P3(state);

  if (period == 0) {
    state->outValue = 0;
    return;
  }

  // Check if LFO should be stopped (lfoOnce or lfoHold after one cycle)
  if (state->step == 0xff) {
    // outValue is already set to the hold value
    return;
  }

  // Calculate position in cycle (0.0 to 1.0) BEFORE incrementing counter
  float phase = (float)state->counter / (float)period;
  int16_t envelopeValue = 0;

  switch (shape) {
    case lfoShapeTri: { // Triangle: bipolar -32385 to +32385
      // Triangle wave centered at 0
      float x = phase * 4.0f; // Scale to 0-4
      if (x < 1.0f) {
        envelopeValue = (int16_t)(x * MOD_MAX_RANGE_F); // 0 to 32385
      } else if (x < 3.0f) {
        envelopeValue = (int16_t)(MOD_MAX_RANGE_F - (x - 1.0f) * MOD_MAX_RANGE_F); // 32385 to -32385
      } else {
        envelopeValue = (int16_t)(-MOD_MAX_RANGE_F + (x - 3.0f) * MOD_MAX_RANGE_F); // -32385 to 0
      }
      break;
    }

    case lfoShapeSin: { // Sine: bipolar -32385 to +32385
      envelopeValue = (int16_t)(sinf(phase * 2.0f * 3.14159265f) * MOD_MAX_RANGE_F);
      break;
    }

    case lfoShapeUniTri: { // Unipolar Triangle: 0 to 32385 to 0
      // Triangle wave that goes 0→max→0 over one period
      float x = phase * 2.0f; // Scale to 0-2
      if (x < 1.0f) {
        envelopeValue = (int16_t)(x * MOD_MAX_RANGE_F); // 0 to 32385
      } else {
        envelopeValue = (int16_t)(MOD_MAX_RANGE_F * (2.0f - x)); // 32385 to 0
      }
      break;
    }

    case lfoShapeUniSin: { // Unipolar Sine: 0 to 32385 to 0
      // Smooth sine wave: (1 - cos(phase * 2π)) / 2
      // Starts slow from 0, accelerates to max, decelerates back to 0
      envelopeValue = (int16_t)((1.0f - cosf(phase * 2.0f * 3.14159265f)) * 0.5f * MOD_MAX_RANGE_F);
      break;
    }

    case lfoShapeRampDown: { // Ramp down: unipolar 32385 to 0
      envelopeValue = (int16_t)(MOD_MAX_RANGE_F * (1.0f - phase));
      break;
    }

    case lfoShapeRampUp: { // Ramp up: unipolar 0 to 32385
      envelopeValue = (int16_t)(MOD_MAX_RANGE_F * phase);
      break;
    }

    case lfoShapeExpDown: { // Exponential decay: unipolar 32385 to 0
      envelopeValue = (int16_t)(MOD_MAX_RANGE_F * expf(-5.0f * phase));
      break;
    }

    case lfoShapeExpUp: { // Exponential rise: unipolar 0 to 32385
      envelopeValue = (int16_t)(MOD_MAX_RANGE_F * (1.0f - expf(-5.0f * phase)));
      break;
    }

    case lfoShapeSquare: { // Square: bipolar -32385 to +32385
      envelopeValue = (phase < 0.5f) ? MOD_MAX_RANGE : -MOD_MAX_RANGE;
      break;
    }

    case lfoShapeRandom: { // Random: bipolar sample-and-hold
      // Generate new random value only at the start of each period
      // Use data1 to store the current random value
      if (state->counter == 0) {
        // Get new random value from utils (returns 0-65535)
        uint16_t randomValue = utilsRandom();
        // Scale from [0, 65535] to [-32385, 32385]
        // 32385 = 255 * 127 (modulation range)
        state->data1 = (int16_t)(((int32_t)randomValue - 32768) * MOD_MAX_RANGE / 32768);
      }
      // Use the stored random value for the entire period
      envelopeValue = state->data1;
      break;
    }

    default:
      envelopeValue = 0;
      break;
  }

  // Apply amount scaling: amount is -128 to 127
  int32_t scaled = ((int32_t)envelopeValue * GET_AMOUNT(state)) / 127;
  state->outValue = (int16_t)scaled;

  // Increment counter AFTER calculating output
  state->counter++;

  // Check if we completed a cycle
  if (state->counter >= period) {
    if (trigger == lfoTrigOnce) {
      // Stop and hold at zero
      state->step = 0xff;
      state->outValue = 0;
      return;
    } else if (trigger == lfoTrigHold) {
      // Stop and hold at last value (already in outValue)
      state->step = 0xff;
      return;
    }
    // For lfoFree and lfoRetrig, wrap around
    state->counter = 0;
  }
}

static void handleLFOnoteOff(PlaybackModState* state) {
  // Do nothing - LFO is not affected by note off
}



void playbackModInit(PlaybackModState* state, Modulation* mod) {
  state->modulation = mod;
  state->amountOffset = 0;
  state->p1Offset = 0;
  state->p2Offset = 0;
  state->p3Offset = 0;
  state->p4Offset = 0;
  state->step = 0;
  state->counter = 0;
  state->data1 = 0;
  state->data2 = 0;
  state->outValue = 0;

  // Cache type and p2 (LFO trigger mode) for change detection
  state->cachedType = GET_TYPE(state);
  state->cachedP2 = GET_P2(state);

  // Additional initialization based on modulation type
  if (GET_TYPE(state) == modADSR) {
    // For ADSR, set initial values for Attack phase
    // Start from 0, target is full range (32385)
    state->data1 = 0;
    state->data2 = MOD_MAX_RANGE;
  } else if (GET_TYPE(state) == modAHD) {
    // For AHD, set initial values for Attack phase
    // Start from 0, target is full range (32385)
    state->data1 = 0;
    state->data2 = MOD_MAX_RANGE;
  }
}

void playbackModNext(PlaybackModState* state) {
  // Check if type or LFO trigger mode changed - reinitialize if so
  enum ModulationType currentType = GET_TYPE(state);
  uint8_t currentP2 = GET_P2(state);

  if (currentType != state->cachedType ||
      (currentType == modLFO && currentP2 != state->cachedP2)) {
    // Type or LFO trigger mode changed - reinitialize
    playbackModInit(state, (Modulation*)state->modulation);
    // Note: After reinit, we still need to process this frame, so continue below
  }

  switch (GET_TYPE(state)) {
    case modADSR:
      handleADSR(state);
      break;
    case modAHD:
      handleAHD(state);
      break;
    case modLFO:
      handleLFO(state);
      break;
    default:
      state->outValue = 0;
      break;
  }
}

void playbackModNoteOff(PlaybackModState* state) {
  switch (GET_TYPE(state)) {
    case modADSR:
      handleADSRnoteOff(state);
      break;
    case modAHD:
      handleAHDnoteOff(state);
      break;
    case modLFO:
      handleLFOnoteOff(state);
      break;
    default:
      break;
  }
}

int16_t playbackModScaleToRange(int16_t modValue, int16_t maxAmplitude) {
  // modValue is in range [-32385, 32385] (MOD_MAX_RANGE)
  // Scale proportionally to [-maxAmplitude, maxAmplitude]
  // Zero stays zero, 32385 becomes maxAmplitude, -32385 becomes -maxAmplitude
  // Add rounding: half of divisor (32385 / 2 = 16192)

  int32_t scaled;
  if (modValue >= 0) {
    scaled = ((int32_t)modValue * maxAmplitude + 16192) / MOD_MAX_RANGE;
  } else {
    scaled = ((int32_t)modValue * maxAmplitude - 16192) / MOD_MAX_RANGE;
  }

  return (int16_t)scaled;
}
