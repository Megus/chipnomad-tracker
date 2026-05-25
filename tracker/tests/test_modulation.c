#include "../external/unity/unity.h"
#include "../../chipnomad_lib/playback_modulation.h"
#include <string.h>
#include <stdio.h>

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// TEST STAND: Helper function for visualizing modulation output
// ============================================================================

/**
 * Print modulation output over time for manual inspection.
 *
 * @param mod Pointer to Modulation configuration
 * @param frames Number of frames to simulate
 * @param noteOffFrame Frame number to trigger note off (0 = no note off)
 * @param maxAmplitude Maximum amplitude for scaling (e.g., 15 for volume, 127 for pitch)
 * @param label Description label for the output
 */
void printModulationOutput(Modulation *mod, int frames, int noteOffFrame, int16_t maxAmplitude, const char *label) {
  PlaybackModState state;
  playbackModInit(&state, mod);

  printf("\n========================================\n");
  printf("Modulation Test: %s\n", label);
  printf("========================================\n");
  printf("Type: %d, Dest: %d, Amount: %d\n", mod->type, mod->destination, mod->amount);
  printf("p1: %d, p2: %d, p3: %d, p4: %d\n", mod->p1, mod->p2, mod->p3, mod->p4);
  printf("Max Amplitude: %d\n", maxAmplitude);
  printf("----------------------------------------\n");
  printf("Frame | Step | Counter | OutValue | Scaled\n");
  printf("----------------------------------------\n");

  for (int i = 0; i < frames; i++) {
    if (noteOffFrame > 0 && i == noteOffFrame) {
      playbackModNoteOff(&state);
      printf("  >>> NOTE OFF <<<\n");
    }

    playbackModNext(&state);
    int16_t scaled = playbackModScaleToRange(state.outValue, maxAmplitude);
    printf("%5d | %4d | %7d | %8d | %6d\n", i, state.step, state.counter, state.outValue, scaled);
  }

  printf("========================================\n\n");
}

// ============================================================================
// UNIT TESTS
// ============================================================================

// Test playbackModInit
void test_modInit_ADSR_initializes_correctly(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 1,
    .amount = 100,
    .p1 = 10,  // Attack
    .p2 = 20,  // Decay
    .p3 = 128, // Sustain
    .p4 = 30   // Release
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  TEST_ASSERT_EQUAL_PTR(&mod, state.modulation);
  TEST_ASSERT_EQUAL(0, state.p1Offset);
  TEST_ASSERT_EQUAL(0, state.p2Offset);
  TEST_ASSERT_EQUAL(0, state.p3Offset);
  TEST_ASSERT_EQUAL(0, state.p4Offset);
  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(0, state.counter);
  TEST_ASSERT_EQUAL(0, state.data1);
  TEST_ASSERT_EQUAL(32385, state.data2);
  TEST_ASSERT_EQUAL(0, state.outValue);
  TEST_ASSERT_EQUAL(modADSR, state.cachedType);
  TEST_ASSERT_EQUAL(20, state.cachedP2);
}

// Test ADSR attack phase
void test_ADSR_attack_phase_ramps_up(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,  // Full positive amount
    .p1 = 10,  // Attack duration
    .p2 = 0,   // Decay
    .p3 = 255, // Sustain
    .p4 = 0    // Release
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // First tick should be in attack phase
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0, state.step); // Still in attack
  TEST_ASSERT_GREATER_THAN(0, state.outValue);

  int16_t prevValue = state.outValue;

  // Continue through attack phase
  for (int i = 1; i < 10; i++) {
    playbackModNext(&state);
    TEST_ASSERT_GREATER_THAN(prevValue, state.outValue);
    prevValue = state.outValue;
  }

  // After attack duration, should move to decay/sustain
  playbackModNext(&state);
  TEST_ASSERT_NOT_EQUAL(0, state.step);
}

// Test ADSR with zero attack goes straight to sustain
void test_ADSR_zero_attack_goes_to_sustain(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,
    .p1 = 0,   // Zero attack
    .p2 = 0,   // Zero decay
    .p3 = 128, // Sustain at half
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);

  // Should be in sustain phase (step 2)
  TEST_ASSERT_EQUAL(2, state.step);

  // Sustain value should be scaled: 128 * 128 = 16384
  // Then scaled by amount: 16384 * 127 / 128 = 16256
  TEST_ASSERT_INT16_WITHIN(100, 16256, state.outValue);
}

// Test ADSR sustain phase holds value
void test_ADSR_sustain_phase_holds_value(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,
    .p1 = 0,   // Zero attack
    .p2 = 0,   // Zero decay
    .p3 = 200, // Sustain
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);
  int16_t sustainValue = state.outValue;

  // Call multiple times, value should remain constant
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    TEST_ASSERT_EQUAL(sustainValue, state.outValue);
    TEST_ASSERT_EQUAL(2, state.step); // Still in sustain
  }
}

// Test ADSR note off triggers release
void test_ADSR_noteOff_triggers_release(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,
    .p1 = 0,
    .p2 = 0,
    .p3 = 200,
    .p4 = 10  // Release duration
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get to sustain
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(2, state.step);
  int16_t sustainValue = state.outValue;

  // Trigger note off
  playbackModNoteOff(&state);

  // Should be in release phase (step 3)
  TEST_ASSERT_EQUAL(3, state.step);
  TEST_ASSERT_EQUAL(0, state.counter);

  // Release should ramp down from sustain value
  playbackModNext(&state);
  TEST_ASSERT_LESS_THAN(sustainValue, state.outValue);
}

// Test ADSR release phase ramps to zero
void test_ADSR_release_ramps_to_zero(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,
    .p1 = 0,
    .p2 = 0,
    .p3 = 255,
    .p4 = 5  // Short release
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state); // Get to sustain
  playbackModNoteOff(&state); // Trigger release

  int16_t prevValue = state.outValue;

  // Release should ramp down
  for (int i = 0; i < 5; i++) {
    playbackModNext(&state);
    if (state.step != 0xff) { // Not stopped yet
      TEST_ASSERT_LESS_THAN(prevValue, state.outValue);
      prevValue = state.outValue;
    }
  }

  // After release duration, should be stopped
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step);
  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test ADSR decay phase
void test_ADSR_decay_phase_ramps_to_sustain(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,
    .p1 = 1,   // Short attack
    .p2 = 10,  // Decay duration
    .p3 = 100, // Sustain level
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get through attack
  playbackModNext(&state);
  playbackModNext(&state);

  // Should be in decay phase (step 1)
  TEST_ASSERT_EQUAL(1, state.step);

  int16_t prevValue = state.outValue;

  // Decay should ramp down to sustain
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    if (state.step == 1) { // Still in decay
      TEST_ASSERT_LESS_THAN(prevValue, state.outValue);
      prevValue = state.outValue;
    }
  }

  // Should reach sustain phase
  TEST_ASSERT_EQUAL(2, state.step);
}

// Test amount scaling - positive amount
void test_ADSR_amount_positive_scales_correctly(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 64,  // Half of max (127)
    .p1 = 0,
    .p2 = 0,
    .p3 = 255,  // Full sustain
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);

  // Sustain at 255 -> 32640 envelope (255 * 128)
  // Scaled by amount 64: 32640 * 64 / 128 = 16320
  TEST_ASSERT_INT16_WITHIN(100, 16320, state.outValue);
}

// Test amount scaling - negative amount
void test_ADSR_amount_negative_inverts_output(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = -127,  // Full negative
    .p1 = 0,
    .p2 = 0,
    .p3 = 255,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);

  // Should be negative
  TEST_ASSERT_LESS_THAN(0, state.outValue);
  // Approximately -32640 * 127 / 128 = -32385
  TEST_ASSERT_INT16_WITHIN(100, -32385, state.outValue);
}

// Test amount scaling - zero amount
void test_ADSR_amount_zero_outputs_zero(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 0,  // Zero amount
    .p1 = 0,
    .p2 = 0,
    .p3 = 255,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);

  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test playbackModScaleToRange - positive value
void test_scaleToRange_positive_value(void) {
  // Max positive value (32640) should scale to maxAmplitude
  int16_t result = playbackModScaleToRange(32640, 15);
  TEST_ASSERT_EQUAL(15, result);

  // Zero should stay zero
  result = playbackModScaleToRange(0, 15);
  TEST_ASSERT_EQUAL(0, result);

  // Mid-positive value (half of 32640 should give half of 15)
  result = playbackModScaleToRange(16384, 15);
  TEST_ASSERT_INT16_WITHIN(1, 7, result);
}

// Test playbackModScaleToRange - negative value
void test_scaleToRange_negative_value(void) {
  // Max negative value (-32768) should scale to -maxAmplitude
  int16_t result = playbackModScaleToRange(-32768, 15);
  TEST_ASSERT_EQUAL(-15, result);

  // Mid-negative value (half of -32768 should give half of -15)
  result = playbackModScaleToRange(-16384, 15);
  TEST_ASSERT_INT16_WITHIN(1, -7, result);
}

// Test playbackModScaleToRange - different ranges
void test_scaleToRange_different_maxAmplitudes(void) {
  // Scale to 255 (8-bit)
  int16_t result = playbackModScaleToRange(32385, 255);
  TEST_ASSERT_EQUAL(255, result);

  result = playbackModScaleToRange(-32385, 255);
  TEST_ASSERT_INT16_WITHIN(1, -255, result);

  result = playbackModScaleToRange(0, 255);
  TEST_ASSERT_EQUAL(0, result);

  // Scale to 1 (binary)
  result = playbackModScaleToRange(32385, 1);
  TEST_ASSERT_EQUAL(1, result);

  result = playbackModScaleToRange(-32385, 1);
  TEST_ASSERT_INT16_WITHIN(1, -1, result);
}

// Test ADSR full cycle
void test_ADSR_full_cycle(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 0,
    .amount = 127,
    .p1 = 3,   // Attack
    .p2 = 2,   // Decay
    .p3 = 128, // Sustain
    .p4 = 2    // Release
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Attack phase
  TEST_ASSERT_EQUAL(0, state.step);
  for (int i = 0; i < 3; i++) {
    playbackModNext(&state);
  }

  // Should move to decay
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(1, state.step);

  // Decay phase
  for (int i = 0; i < 2; i++) {
    playbackModNext(&state);
  }

  // Should move to sustain
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(2, state.step);

  // Sustain holds
  int16_t sustainValue = state.outValue;
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(sustainValue, state.outValue);

  // Note off -> release
  playbackModNoteOff(&state);
  TEST_ASSERT_EQUAL(3, state.step);

  // Release phase
  for (int i = 0; i < 2; i++) {
    playbackModNext(&state);
  }

  // Should be stopped
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step);
  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test AHD initialization
void test_modInit_AHD_initializes_correctly(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 1,
    .amount = 100,
    .p1 = 10,  // Attack
    .p2 = 20,  // Hold
    .p3 = 30,  // Decay
    .p4 = 0    // Unused
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  TEST_ASSERT_EQUAL_PTR(&mod, state.modulation);
  TEST_ASSERT_EQUAL(0, state.p1Offset);
  TEST_ASSERT_EQUAL(0, state.p2Offset);
  TEST_ASSERT_EQUAL(0, state.p3Offset);
  TEST_ASSERT_EQUAL(0, state.p4Offset);
  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(0, state.counter);
  TEST_ASSERT_EQUAL(0, state.data1);
  TEST_ASSERT_EQUAL(32385, state.data2);
  TEST_ASSERT_EQUAL(0, state.outValue);
  TEST_ASSERT_EQUAL(modAHD, state.cachedType);
  TEST_ASSERT_EQUAL(20, state.cachedP2);
}

// Test AHD attack phase
void test_AHD_attack_phase_ramps_up(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 10,  // Attack duration
    .p2 = 5,   // Hold
    .p3 = 5,   // Decay
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // First tick should be in attack phase
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0, state.step); // Still in attack
  TEST_ASSERT_GREATER_THAN(0, state.outValue);

  int16_t prevValue = state.outValue;

  // Continue through attack phase
  for (int i = 1; i < 10; i++) {
    playbackModNext(&state);
    TEST_ASSERT_GREATER_THAN(prevValue, state.outValue);
    prevValue = state.outValue;
  }

  // After attack duration, should move to hold
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(1, state.step);
}

// Test AHD hold phase
void test_AHD_hold_phase_stays_at_peak(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 1,   // Short attack
    .p2 = 10,  // Hold duration
    .p3 = 5,   // Decay
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get through attack
  playbackModNext(&state);
  playbackModNext(&state);

  // Should be in hold phase
  TEST_ASSERT_EQUAL(1, state.step);

  // Value should be at peak (32640 * 127 / 128 = 32385)
  TEST_ASSERT_INT16_WITHIN(100, 32385, state.outValue);

  int16_t holdValue = state.outValue;

  // Hold should maintain the same value
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    if (state.step == 1) { // Still in hold
      TEST_ASSERT_EQUAL(holdValue, state.outValue);
    }
  }

  // Should eventually move to decay
  TEST_ASSERT_EQUAL(2, state.step);
}

// Test AHD decay phase
void test_AHD_decay_phase_ramps_to_zero(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 1,   // Short attack
    .p2 = 1,   // Short hold
    .p3 = 10,  // Decay duration
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get through attack and hold
  playbackModNext(&state);
  playbackModNext(&state);
  playbackModNext(&state);

  // Should be in decay phase
  TEST_ASSERT_EQUAL(2, state.step);

  int16_t prevValue = state.outValue;

  // Decay should ramp down
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    if (state.step == 2) { // Still in decay
      TEST_ASSERT_LESS_THAN(prevValue, state.outValue);
      prevValue = state.outValue;
    }
  }

  // After decay, should be stopped
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step);
  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test AHD with zero attack
void test_AHD_zero_attack_goes_to_hold(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 0,   // Zero attack
    .p2 = 5,   // Hold
    .p3 = 5,   // Decay
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);

  // Should be in hold phase (step 1)
  TEST_ASSERT_EQUAL(1, state.step);

  // Should be at peak (32640 * 127 / 128 = 32385)
  TEST_ASSERT_INT16_WITHIN(100, 32385, state.outValue);
}

// Test AHD with zero hold
void test_AHD_zero_hold_goes_to_decay(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 1,   // Short attack
    .p2 = 0,   // Zero hold
    .p3 = 5,   // Decay
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get through attack
  playbackModNext(&state);
  playbackModNext(&state);

  // Should skip hold and go to decay (step 2)
  TEST_ASSERT_EQUAL(2, state.step);
}

// Test AHD note off does nothing
void test_AHD_noteOff_does_nothing(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 0,
    .p2 = 10,  // Long hold
    .p3 = 5,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get to hold phase
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(1, state.step);

  int16_t valueBeforeNoteOff = state.outValue;
  uint8_t stepBeforeNoteOff = state.step;

  // Trigger note off
  playbackModNoteOff(&state);

  // Should not change anything
  TEST_ASSERT_EQUAL(stepBeforeNoteOff, state.step);

  // Continue playing
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(valueBeforeNoteOff, state.outValue);
}

// Test AHD full cycle
void test_AHD_full_cycle(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = 127,
    .p1 = 3,   // Attack
    .p2 = 2,   // Hold
    .p3 = 3,   // Decay
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Attack phase
  TEST_ASSERT_EQUAL(0, state.step);
  for (int i = 0; i < 3; i++) {
    playbackModNext(&state);
  }

  // Should move to hold
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(1, state.step);

  // Hold phase
  int16_t holdValue = state.outValue;
  for (int i = 0; i < 2; i++) {
    playbackModNext(&state);
    if (state.step == 1) {
      TEST_ASSERT_EQUAL(holdValue, state.outValue);
    }
  }

  // Should move to decay
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(2, state.step);

  // Decay phase
  for (int i = 0; i < 3; i++) {
    playbackModNext(&state);
  }

  // Should be stopped
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step);
  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test AHD with negative amount
void test_AHD_negative_amount_inverts_output(void) {
  Modulation mod = {
    .type = modAHD,
    .destination = 0,
    .amount = -127,  // Full negative
    .p1 = 0,
    .p2 = 5,
    .p3 = 0,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);

  // Should be negative (inverted peak)
  TEST_ASSERT_LESS_THAN(0, state.outValue);
  TEST_ASSERT_INT16_WITHIN(100, -32385, state.outValue);
}

// Test LFO initialization
void test_modInit_LFO_initializes_correctly(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 1,
    .amount = 100,
    .p1 = lfoShapeTri,    // Shape
    .p2 = lfoTrigFree,   // Trigger
    .p3 = 20,        // Period
    .p4 = 0          // Unused
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  TEST_ASSERT_EQUAL_PTR(&mod, state.modulation);
  TEST_ASSERT_EQUAL(0, state.p1Offset);
  TEST_ASSERT_EQUAL(0, state.p2Offset);
  TEST_ASSERT_EQUAL(0, state.p3Offset);
  TEST_ASSERT_EQUAL(0, state.p4Offset);
  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(0, state.counter);
  TEST_ASSERT_EQUAL(modLFO, state.cachedType);
  TEST_ASSERT_EQUAL(lfoTrigFree, state.cachedP2);
}

// Test LFO triangle shape oscillates
void test_LFO_triangle_oscillates(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeTri,
    .p2 = lfoTrigFree,
    .p3 = 16,  // 16 tick period
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Triangle should start at 0, go positive, back to 0, negative, back to 0
  playbackModNext(&state);
  int16_t firstValue = state.outValue;
  TEST_ASSERT_INT16_WITHIN(2000, 0, firstValue); // Should start near zero

  // Continue - should increase
  playbackModNext(&state);
  TEST_ASSERT_GREATER_THAN(firstValue, state.outValue);

  // Continue to peak
  for (int i = 0; i < 3; i++) {
    int16_t prev = state.outValue;
    playbackModNext(&state);
    TEST_ASSERT_GREATER_THAN(prev, state.outValue);
  }

  // Should reach peak around quarter period (counter=4, phase=0.25)
  int16_t peakValue = state.outValue;
  TEST_ASSERT_INT16_WITHIN(2000, 32640, peakValue);

  // Continue past peak, should decrease
  playbackModNext(&state);
  TEST_ASSERT_LESS_THAN(peakValue, state.outValue);
}

// Test LFO square wave
void test_LFO_square_toggles(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeSquare,
    .p2 = lfoTrigFree,
    .p3 = 10,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // First half should be positive (32640 * 127 / 128 = 32385)
  playbackModNext(&state);
  int16_t firstHalf = state.outValue;
  TEST_ASSERT_INT16_WITHIN(100, 32385, firstHalf);

  // Should stay at same value for first half
  for (int i = 0; i < 4; i++) {
    playbackModNext(&state);
    TEST_ASSERT_EQUAL(firstHalf, state.outValue);
  }

  // Second half should be negative (-32640 * 127 / 128 = -32385)
  playbackModNext(&state);
  int16_t secondHalf = state.outValue;
  TEST_ASSERT_INT16_WITHIN(100, -32385, secondHalf);
}

// Test LFO ramp up
void test_LFO_rampUp_increases(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeRampUp,
    .p2 = lfoTrigFree,
    .p3 = 10,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Should start near zero and increase
  playbackModNext(&state);
  TEST_ASSERT_INT16_WITHIN(5000, 0, state.outValue);

  int16_t prevValue = state.outValue;
  for (int i = 0; i < 9; i++) {
    playbackModNext(&state);
    TEST_ASSERT_GREATER_THAN(prevValue, state.outValue);
    prevValue = state.outValue;
  }

  // Should end near peak
  TEST_ASSERT_INT16_WITHIN(5000, 32640, state.outValue);
}

// Test LFO ramp down
void test_LFO_rampDown_decreases(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeRampDown,
    .p2 = lfoTrigFree,
    .p3 = 10,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Should start near peak and decrease
  playbackModNext(&state);
  TEST_ASSERT_INT16_WITHIN(5000, 32640, state.outValue);

  int16_t prevValue = state.outValue;
  for (int i = 0; i < 9; i++) {
    playbackModNext(&state);
    TEST_ASSERT_LESS_THAN(prevValue, state.outValue);
    prevValue = state.outValue;
  }

  // Should end near zero
  TEST_ASSERT_INT16_WITHIN(5000, 0, state.outValue);
}

// Test LFO wraps around with lfoFree
void test_LFO_free_wraps_around(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeSquare,
    .p2 = lfoTrigFree,
    .p3 = 4,  // Short period
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Run through one cycle
  for (int i = 0; i < 4; i++) {
    playbackModNext(&state);
  }

  // Should wrap around and continue
  playbackModNext(&state);
  TEST_ASSERT_NOT_EQUAL(0xff, state.step); // Not stopped
  TEST_ASSERT_EQUAL(1, state.counter); // Wrapped to 1
}

// Test LFO stops with lfoOnce
void test_LFO_once_stops_after_cycle(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeSquare,
    .p2 = lfoTrigOnce,
    .p3 = 4,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Run through one cycle
  for (int i = 0; i < 4; i++) {
    playbackModNext(&state);
  }

  // Next tick should stop and hold at zero
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step); // Stopped
  TEST_ASSERT_EQUAL(0, state.outValue); // Held at zero

  // Should stay stopped
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step);
  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test LFO holds last value with lfoHold
void test_LFO_hold_stops_at_last_value(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeRampUp,
    .p2 = lfoTrigHold,
    .p3 = 5,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Run through one cycle
  for (int i = 0; i < 5; i++) {
    playbackModNext(&state);
  }

  int16_t lastValue = state.outValue;

  // Next tick should stop and hold last value
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0xff, state.step); // Stopped
  TEST_ASSERT_EQUAL(lastValue, state.outValue); // Held at last value

  // Should stay at that value
  playbackModNext(&state);
  TEST_ASSERT_EQUAL(lastValue, state.outValue);
}

// Test LFO with negative amount inverts
void test_LFO_negative_amount_inverts(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = -127,
    .p1 = lfoShapeSquare,
    .p2 = lfoTrigFree,
    .p3 = 10,
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // First half should be negative (inverted) (32640 * -127 / 128 = -32385)
  playbackModNext(&state);
  TEST_ASSERT_INT16_WITHIN(100, -32385, state.outValue);

  // Second half should be positive (inverted) (-32640 * -127 / 128 = 32385)
  for (int i = 0; i < 5; i++) {
    playbackModNext(&state);
  }
  TEST_ASSERT_INT16_WITHIN(100, 32385, state.outValue);
}

// Test LFO with zero period outputs zero
void test_LFO_zero_period_outputs_zero(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 0,
    .amount = 127,
    .p1 = lfoShapeTri,
    .p2 = lfoTrigFree,
    .p3 = 0,  // Zero period
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  playbackModNext(&state);
  TEST_ASSERT_EQUAL(0, state.outValue);
}

// Test type change detection and reinitialization
void test_type_change_reinitializes(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 1,
    .amount = 127,
    .p1 = 10,  // Attack
    .p2 = 5,   // Decay
    .p3 = 200, // Sustain
    .p4 = 10   // Release
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Advance through attack phase
  for (int i = 0; i < 5; i++) {
    playbackModNext(&state);
  }

  // Should be in attack phase (step 0)
  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(5, state.counter);
  TEST_ASSERT_GREATER_THAN(0, state.outValue);
  int16_t adsrValue = state.outValue;

  // Change type to AHD
  mod.type = modAHD;

  // Next call should detect change and reinitialize
  playbackModNext(&state);

  // Should be reinitialized: step 0, counter 0 (or 1 after first next)
  TEST_ASSERT_EQUAL(modAHD, state.cachedType);
  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(1, state.counter); // Counter increments in first call
  // Value should be different (restarted from beginning)
  TEST_ASSERT_NOT_EQUAL(adsrValue, state.outValue);
}

void test_LFO_trigger_mode_change_reinitializes(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 1,
    .amount = 127,
    .p1 = lfoShapeTri,    // Shape
    .p2 = lfoTrigFree,   // Trigger - free running
    .p3 = 16,        // Period
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Advance LFO partway through cycle
  for (int i = 0; i < 8; i++) {
    playbackModNext(&state);
  }

  TEST_ASSERT_EQUAL(8, state.counter);
  TEST_ASSERT_EQUAL(lfoTrigFree, state.cachedP2);

  // Change trigger mode to lfoOnce
  mod.p2 = lfoTrigOnce;

  // Next call should detect change and reinitialize
  playbackModNext(&state);

  // Should be reinitialized
  TEST_ASSERT_EQUAL(lfoTrigOnce, state.cachedP2);
  TEST_ASSERT_EQUAL(1, state.counter); // Restarted and advanced one step
}

void test_parameter_change_without_type_change_continues(void) {
  Modulation mod = {
    .type = modADSR,
    .destination = 1,
    .amount = 127,
    .p1 = 10,  // Attack
    .p2 = 5,   // Decay
    .p3 = 200, // Sustain
    .p4 = 10   // Release
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Advance through attack phase
  for (int i = 0; i < 5; i++) {
    playbackModNext(&state);
  }

  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(5, state.counter);

  // Change attack duration (p1) - should NOT reinitialize
  mod.p1 = 20;

  playbackModNext(&state);

  // Should continue from where it was (counter should increment)
  TEST_ASSERT_EQUAL(0, state.step);
  TEST_ASSERT_EQUAL(6, state.counter);
  TEST_ASSERT_EQUAL(modADSR, state.cachedType);
}

void test_LFO_shape_change_without_trigger_change_continues(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 1,
    .amount = 127,
    .p1 = lfoShapeTri,    // Shape
    .p2 = lfoTrigFree,   // Trigger
    .p3 = 16,        // Period
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Advance LFO partway through cycle
  for (int i = 0; i < 4; i++) {
    playbackModNext(&state);
  }

  TEST_ASSERT_EQUAL(4, state.counter);
  int16_t triValue = state.outValue;

  // Change shape to square (p1) - should NOT reinitialize
  mod.p1 = lfoShapeSquare;

  playbackModNext(&state);

  // Should continue from same position (counter increments)
  TEST_ASSERT_EQUAL(5, state.counter);
  TEST_ASSERT_EQUAL(modLFO, state.cachedType);
  TEST_ASSERT_EQUAL(lfoTrigFree, state.cachedP2);
  // Value will be different due to shape change, but counter continues
  TEST_ASSERT_NOT_EQUAL(triValue, state.outValue);
}

void test_LFO_random_changes_each_period(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 1,
    .amount = 127,
    .p1 = lfoShapeRandom,  // Random shape
    .p2 = lfoTrigFree,     // Free running
    .p3 = 8,               // Period: 8 frames
    .p4 = 0
  };

  PlaybackModState state;
  playbackModInit(&state, &mod);

  // Get first random value
  playbackModNext(&state);
  int16_t firstValue = state.outValue;

  // Value should stay the same for the entire period
  for (int i = 1; i < 8; i++) {
    playbackModNext(&state);
    TEST_ASSERT_EQUAL(firstValue, state.outValue);
  }

  // After period wraps, should get a new random value
  playbackModNext(&state);
  int16_t secondValue = state.outValue;

  // New value should be different (extremely unlikely to be the same)
  TEST_ASSERT_NOT_EQUAL(firstValue, secondValue);

  // Second value should also hold for the entire period
  for (int i = 1; i < 8; i++) {
    playbackModNext(&state);
    TEST_ASSERT_EQUAL(secondValue, state.outValue);
  }

  // Third period should get yet another value
  playbackModNext(&state);
  int16_t thirdValue = state.outValue;
  TEST_ASSERT_NOT_EQUAL(secondValue, thirdValue);
}

// ============================================================================
// TEST STAND
// ============================================================================

void testStand(void) {
  Modulation mod = {
    .type = modLFO,
    .destination = 1,
    .amount = 127,
    .p1 = lfoShapeRandom,
    .p2 = lfoTrigRetrig,
    .p3 = 5,
    .p4 = 0
  };

  printModulationOutput(&mod, 30, 100, 15, "LFO1");
}

void test_legacy_AY1_volume_sustain_scaling(void) {
  // Test that legacy AY1 volume envelope sustain values (0-15) are correctly
  // scaled to produce exact 0-15 volume output values in the new modulation system.
  // The clever trick: use p3Offset to scale sustain from 0-15 to 0-255 range.
  // Formula: scaledSustain = (p3 * 255 + 7) / 15
  //          p3Offset = scaledSustain - p3

  for (uint8_t sustainValue = 0; sustainValue <= 15; sustainValue++) {
    // Create a modulation with ADSR type and the sustain value
    Modulation mod = {
      .type = modADSR,
      .destination = 1,  // Volume
      .amount = 127,     // Full positive amount (legacy AY1 uses 127)
      .p1 = 0,           // Attack: 0 (instant)
      .p2 = 0,           // Decay: 0 (instant)
      .p3 = sustainValue, // Sustain: 0-15 (legacy AY1 range)
      .p4 = 10           // Release: 10 frames
    };

    PlaybackModState state;
    playbackModInit(&state, &mod);

    // Apply the scaling trick (same as setupInstrumentAY1)
    int16_t scaledSustain = (mod.p3 * 255 + 7) / 15;
    state.p3Offset = scaledSustain - mod.p3;

    // Advance to sustain phase (attack and decay are 0, so we're immediately in sustain)
    playbackModNext(&state);

    // The modulation output is in range [-32640, 32640]
    // Scale it to 0-15 range (same as handleInstrumentAY1 does for ADSR)
    // For ADSR, handleInstrumentAY1 does: volume = scaledVolume (rewrite, not add)
    int16_t scaledVolume = playbackModScaleToRange(state.outValue, 15);

    // Handle the rewrite logic from handleInstrumentAY1
    uint8_t volume;
    if (scaledVolume < 0) {
      volume = 15 - (-scaledVolume);
    } else {
      volume = scaledVolume;
    }
    if (volume > 15) volume = 15;

    // Verify that the scaled volume matches the original sustain value
    TEST_ASSERT_EQUAL_MESSAGE(sustainValue, volume,
      "Legacy AY1 sustain scaling failed for value");
  }
}

int main(void) {
  UNITY_BEGIN();

  //testStand();

  // ADSR tests
  RUN_TEST(test_modInit_ADSR_initializes_correctly);
  RUN_TEST(test_ADSR_attack_phase_ramps_up);
  RUN_TEST(test_ADSR_zero_attack_goes_to_sustain);
  RUN_TEST(test_ADSR_sustain_phase_holds_value);
  RUN_TEST(test_ADSR_noteOff_triggers_release);
  RUN_TEST(test_ADSR_release_ramps_to_zero);
  RUN_TEST(test_ADSR_decay_phase_ramps_to_sustain);
  RUN_TEST(test_ADSR_amount_positive_scales_correctly);
  RUN_TEST(test_ADSR_amount_negative_inverts_output);
  RUN_TEST(test_ADSR_amount_zero_outputs_zero);
  RUN_TEST(test_ADSR_full_cycle);

  // AHD tests
  RUN_TEST(test_modInit_AHD_initializes_correctly);
  RUN_TEST(test_AHD_attack_phase_ramps_up);
  RUN_TEST(test_AHD_hold_phase_stays_at_peak);
  RUN_TEST(test_AHD_decay_phase_ramps_to_zero);
  RUN_TEST(test_AHD_zero_attack_goes_to_hold);
  RUN_TEST(test_AHD_zero_hold_goes_to_decay);
  RUN_TEST(test_AHD_noteOff_does_nothing);
  RUN_TEST(test_AHD_full_cycle);
  RUN_TEST(test_AHD_negative_amount_inverts_output);

  // LFO tests
  RUN_TEST(test_modInit_LFO_initializes_correctly);
  RUN_TEST(test_LFO_triangle_oscillates);
  RUN_TEST(test_LFO_square_toggles);
  RUN_TEST(test_LFO_rampUp_increases);
  RUN_TEST(test_LFO_rampDown_decreases);
  RUN_TEST(test_LFO_free_wraps_around);
  RUN_TEST(test_LFO_once_stops_after_cycle);
  RUN_TEST(test_LFO_hold_stops_at_last_value);
  RUN_TEST(test_LFO_negative_amount_inverts);
  RUN_TEST(test_LFO_zero_period_outputs_zero);
  RUN_TEST(test_LFO_random_changes_each_period);

  // Range scaling tests
  RUN_TEST(test_scaleToRange_positive_value);
  RUN_TEST(test_scaleToRange_negative_value);
  RUN_TEST(test_scaleToRange_different_maxAmplitudes);

  // Live parameter change tests
  RUN_TEST(test_type_change_reinitializes);
  RUN_TEST(test_LFO_trigger_mode_change_reinitializes);
  RUN_TEST(test_parameter_change_without_type_change_continues);
  RUN_TEST(test_LFO_shape_change_without_trigger_change_continues);

  // Legacy AY1 volume scaling test
  RUN_TEST(test_legacy_AY1_volume_sustain_scaling);

  return UNITY_END();
}
