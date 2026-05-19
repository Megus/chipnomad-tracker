#include "playback_modulation.h"
#include <math.h>

static void handleADSR(PlaybackModState* state) {
  // step: 0 - Attack, 1 - Decay, 2 - Sustain, 3 - Release

  int16_t envelopeValue = 0;

  while (1) {
    if (state->step > 3) break; // Just in case...

    // Sustain phase
    if (state->step == 2) {
      // Scale sustain (0-255) to 0-32767 range
      envelopeValue = (state->p3 * 32767) / 255;
      break;
    }

    int duration = 0;
    if (state->step == 0) duration = state->p1;  // Attack
    else if (state->step == 1) duration = state->p2;  // Decay
    else if (state->step == 3) duration = state->p4;  // Release

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
        // Decay to sustain
        if (state->step == 1) {
          state->data1 = 32767; // From (full amplitude)
          state->data2 = (state->p3 * 32767) / 255;  // To (Sustain level scaled to 16-bit)
        }
      }
    } else {
      // LERP between data1 and data2
      state->counter++;
      int32_t from = state->data1;
      int32_t to = state->data2;
      int32_t delta = to - from;
      envelopeValue = from + ((delta * ((state->counter << 8) / (duration + 1))) >> 8);
      break;
    }
  }

  // Apply amount scaling: amount is -128 to 127
  // Scale envelope (0-32767) by amount to get final signed 16-bit value
  // outValue range: approximately -32768 to 32767
  int32_t scaled = (envelopeValue * state->amount) / 127;
  state->outValue = (int16_t)scaled;
}

static void handleADSRnoteOff(PlaybackModState* state) {
  // If the note is turned off, jump to release phase
  if (state->step < 3) {
    state->step = 3;
    state->counter = 0;
    // Set release from the current envelope value (before amount scaling)
    // Reverse the amount scaling to get the envelope value
    int32_t currentEnvelope = (state->outValue * 127) / (state->amount != 0 ? state->amount : 1);
    state->data1 = (int16_t)currentEnvelope;
    state->data2 = 0; // Release to 0
  }
}

static void handleAHD(PlaybackModState* state) {
  // step: 0 - Attack, 1 - Hold, 2 - Decay
  // Output range: 0 to 32767 (16-bit signed positive range)
  // All three phases are treated uniformly as ramps (Hold is a ramp from 32767 to 32767)

  int16_t envelopeValue = 0;

  while (1) {
    if (state->step > 2) {
      // Envelope stopped
      envelopeValue = 0;
      break;
    }

    // Get duration for current phase
    int duration = 0;
    if (state->step == 0) duration = state->p1;      // Attack
    else if (state->step == 1) duration = state->p2; // Hold
    else if (state->step == 2) duration = state->p3; // Decay

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
          // Hold: 32767 to 32767
          state->data1 = 32767;
          state->data2 = 32767;
        } else if (state->step == 2) {
          // Decay: 32767 to 0
          state->data1 = 32767;
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
      envelopeValue = from + ((delta * ((state->counter << 8) / (duration + 1))) >> 8);
      break;
    }
  }

  // Apply amount scaling: amount is -128 to 127
  // Scale envelope (0-32767) by amount to get final signed 16-bit value
  int32_t scaled = (envelopeValue * state->amount) / 127;
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

  uint8_t shape = state->p1;
  uint8_t trigger = state->p2;
  uint8_t period = state->p3;

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
    case lfoTri: { // Triangle: bipolar -32767 to +32767
      // Triangle wave centered at 0
      float x = phase * 4.0f; // Scale to 0-4
      if (x < 1.0f) {
        envelopeValue = (int16_t)(x * 32767.0f); // 0 to 32767
      } else if (x < 3.0f) {
        envelopeValue = (int16_t)(32767.0f - (x - 1.0f) * 32767.0f); // 32767 to -32767
      } else {
        envelopeValue = (int16_t)(-32767.0f + (x - 3.0f) * 32767.0f); // -32767 to 0
      }
      break;
    }

    case lfoSin: { // Sine: bipolar -32767 to +32767
      envelopeValue = (int16_t)(sinf(phase * 2.0f * 3.14159265f) * 32767.0f);
      break;
    }

    case lfoRampDown: { // Ramp down: unipolar 32767 to 0
      envelopeValue = (int16_t)(32767.0f * (1.0f - phase));
      break;
    }

    case lfoRampUp: { // Ramp up: unipolar 0 to 32767
      envelopeValue = (int16_t)(32767.0f * phase);
      break;
    }

    case lfoExpDown: { // Exponential decay: unipolar 32767 to 0
      envelopeValue = (int16_t)(32767.0f * expf(-5.0f * phase));
      break;
    }

    case lfoExpUp: { // Exponential rise: unipolar 0 to 32767
      envelopeValue = (int16_t)(32767.0f * (1.0f - expf(-5.0f * phase)));
      break;
    }

    case lfoSquare: { // Square: bipolar -32767 to +32767
      envelopeValue = (phase < 0.5f) ? 32767 : -32767;
      break;
    }

    case lfoRandom: { // Random: bipolar sample-and-hold
      // Use counter as seed for pseudo-random
      // Simple linear congruential generator
      uint32_t seed = (uint32_t)state->counter + (uint32_t)state->p3 * 1103515245u + 12345u;
      int16_t randomValue = (int16_t)((seed >> 16) & 0xFFFF);
      // Scale to -32767 to +32767
      envelopeValue = (int16_t)(((int32_t)randomValue - 32768) * 32767 / 32768);
      break;
    }

    default:
      envelopeValue = 0;
      break;
  }

  // Apply amount scaling: amount is -128 to 127
  int32_t scaled = ((int32_t)envelopeValue * state->amount) / 127;
  state->outValue = (int16_t)scaled;

  // Increment counter AFTER calculating output
  state->counter++;

  // Check if we completed a cycle
  if (state->counter >= period) {
    if (trigger == lfoOnce) {
      // Stop and hold at zero
      state->step = 0xff;
      state->outValue = 0;
      return;
    } else if (trigger == lfoHold) {
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
  state->type = mod->type;
  state->destination = mod->destination;
  state->amount = mod->amount;
  state->p1 = mod->p1;
  state->p2 = mod->p2;
  state->p3 = mod->p3;
  state->p4 = mod->p4;
  state->step = 0;
  state->counter = 0;
  state->data1 = 0;
  state->data2 = 0;
  state->outValue = 0;

  // Additional initialization based on modulation type
  if (state->type == modADSR) {
    // For ADSR, set initial values for Attack phase
    // Start from 0, target is full 16-bit range (32767)
    state->data1 = 0;
    state->data2 = 32767;
  } else if (state->type == modAHD) {
    // For AHD, set initial values for Attack phase
    // Start from 0, target is full 16-bit range (32767)
    state->data1 = 0;
    state->data2 = 32767;
  }
}

void playbackModNext(PlaybackModState* state) {
  switch (state->type) {
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
  switch (state->type) {
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
  // modValue is in range [-32768, 32767]
  // Scale to [0, maxAmplitude]

  // First, shift to unsigned range [0, 65535]
  int32_t shifted = (int32_t)modValue + 32768;

  // Scale to [0, maxAmplitude]
  int32_t scaled = (shifted * maxAmplitude) / 65535;

  return (int16_t)scaled;
}
