#include "doctest.h"
extern "C" {
#include "../../chipnomad_lib/playback_modulation.h"
}
#include <string.h>
#include <stdio.h>

TEST_SUITE("modulation") {


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
TEST_CASE("test_modInit_ADSR_initializes_correctly") {
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

  CHECK(&mod == state.modulation);
  CHECK(state.p1Offset == 0);
  CHECK(state.p2Offset == 0);
  CHECK(state.p3Offset == 0);
  CHECK(state.p4Offset == 0);
  CHECK(state.step == 0);
  CHECK(state.counter == 0);
  CHECK(state.data1 == 0);
  CHECK(state.data2 == 32385);
  CHECK(state.outValue == 0);
  CHECK(state.cachedType == modADSR);
  CHECK(state.cachedP2 == 20);
}

// Test ADSR attack phase
TEST_CASE("test_ADSR_attack_phase_ramps_up") {
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
  CHECK(state.step == 0); // Still in attack
  CHECK(state.outValue > 0);

  int16_t prevValue = state.outValue;

  // Continue through attack phase
  for (int i = 1; i < 10; i++) {
    playbackModNext(&state);
    CHECK(state.outValue > prevValue);
    prevValue = state.outValue;
  }

  // After attack duration, should move to decay/sustain
  playbackModNext(&state);
  CHECK(state.step != 0);
}

// Test ADSR with zero attack goes straight to sustain
TEST_CASE("test_ADSR_zero_attack_goes_to_sustain") {
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
  CHECK(state.step == 2);

  // Sustain value should be scaled: 128 * 128 = 16384
  // Then scaled by amount: 16384 * 127 / 128 = 16256
  CHECK(state.outValue >= 16256 - 100); CHECK(state.outValue <= 16256 + 100);
}

// Test ADSR sustain phase holds value
TEST_CASE("test_ADSR_sustain_phase_holds_value") {
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
    CHECK(state.outValue == sustainValue);
    CHECK(state.step == 2); // Still in sustain
  }
}

// Test ADSR note off triggers release
TEST_CASE("test_ADSR_noteOff_triggers_release") {
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
  CHECK(state.step == 2);
  int16_t sustainValue = state.outValue;

  // Trigger note off
  playbackModNoteOff(&state);

  // Should be in release phase (step 3)
  CHECK(state.step == 3);
  CHECK(state.counter == 0);

  // Release should ramp down from sustain value
  playbackModNext(&state);
  CHECK(state.outValue < sustainValue);
}

// Test ADSR release phase ramps to zero
TEST_CASE("test_ADSR_release_ramps_to_zero") {
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
      CHECK(state.outValue < prevValue);
      prevValue = state.outValue;
    }
  }

  // After release duration, should be stopped
  playbackModNext(&state);
  CHECK(state.step == 0xff);
  CHECK(state.outValue == 0);
}

// Test ADSR decay phase
TEST_CASE("test_ADSR_decay_phase_ramps_to_sustain") {
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
  CHECK(state.step == 1);

  int16_t prevValue = state.outValue;

  // Decay should ramp down to sustain
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    if (state.step == 1) { // Still in decay
      CHECK(state.outValue < prevValue);
      prevValue = state.outValue;
    }
  }

  // Should reach sustain phase
  CHECK(state.step == 2);
}

// Test amount scaling - positive amount
TEST_CASE("test_ADSR_amount_positive_scales_correctly") {
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
  CHECK(state.outValue >= 16320 - 100); CHECK(state.outValue <= 16320 + 100);
}

// Test amount scaling - negative amount
TEST_CASE("test_ADSR_amount_negative_inverts_output") {
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
  CHECK(state.outValue < 0);
  // Approximately -32640 * 127 / 128 = -32385
  CHECK(state.outValue >= -32385 - 100); CHECK(state.outValue <= -32385 + 100);
}

// Test amount scaling - zero amount
TEST_CASE("test_ADSR_amount_zero_outputs_zero") {
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

  CHECK(state.outValue == 0);
}

// Test playbackModScaleToRange - positive value
TEST_CASE("test_scaleToRange_positive_value") {
  // Max positive value (32640) should scale to maxAmplitude
  int16_t result = playbackModScaleToRange(32640, 15);
  CHECK(result == 15);

  // Zero should stay zero
  result = playbackModScaleToRange(0, 15);
  CHECK(result == 0);

  // Mid-positive value (half of 32640 should give half of 15)
  result = playbackModScaleToRange(16384, 15);
  CHECK(result >= 7 - 1); CHECK(result <= 7 + 1);
}

// Test playbackModScaleToRange - negative value
TEST_CASE("test_scaleToRange_negative_value") {
  // Max negative value (-32768) should scale to -maxAmplitude
  int16_t result = playbackModScaleToRange(-32768, 15);
  CHECK(result == -15);

  // Mid-negative value (half of -32768 should give half of -15)
  result = playbackModScaleToRange(-16384, 15);
  CHECK(result >= -7 - 1); CHECK(result <= -7 + 1);
}

// Test playbackModScaleToRange - different ranges
TEST_CASE("test_scaleToRange_different_maxAmplitudes") {
  // Scale to 255 (8-bit)
  int16_t result = playbackModScaleToRange(32385, 255);
  CHECK(result == 255);

  result = playbackModScaleToRange(-32385, 255);
  CHECK(result >= -255 - 1); CHECK(result <= -255 + 1);

  result = playbackModScaleToRange(0, 255);
  CHECK(result == 0);

  // Scale to 1 (binary)
  result = playbackModScaleToRange(32385, 1);
  CHECK(result == 1);

  result = playbackModScaleToRange(-32385, 1);
  CHECK(result >= -1 - 1); CHECK(result <= -1 + 1);
}

// Test ADSR full cycle
TEST_CASE("test_ADSR_full_cycle") {
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
  CHECK(state.step == 0);
  for (int i = 0; i < 3; i++) {
    playbackModNext(&state);
  }

  // Should move to decay
  playbackModNext(&state);
  CHECK(state.step == 1);

  // Decay phase
  for (int i = 0; i < 2; i++) {
    playbackModNext(&state);
  }

  // Should move to sustain
  playbackModNext(&state);
  CHECK(state.step == 2);

  // Sustain holds
  int16_t sustainValue = state.outValue;
  playbackModNext(&state);
  CHECK(state.outValue == sustainValue);

  // Note off -> release
  playbackModNoteOff(&state);
  CHECK(state.step == 3);

  // Release phase
  for (int i = 0; i < 2; i++) {
    playbackModNext(&state);
  }

  // Should be stopped
  playbackModNext(&state);
  CHECK(state.step == 0xff);
  CHECK(state.outValue == 0);
}

// Test AHD initialization
TEST_CASE("test_modInit_AHD_initializes_correctly") {
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

  CHECK(&mod == state.modulation);
  CHECK(state.p1Offset == 0);
  CHECK(state.p2Offset == 0);
  CHECK(state.p3Offset == 0);
  CHECK(state.p4Offset == 0);
  CHECK(state.step == 0);
  CHECK(state.counter == 0);
  CHECK(state.data1 == 0);
  CHECK(state.data2 == 32385);
  CHECK(state.outValue == 0);
  CHECK(state.cachedType == modAHD);
  CHECK(state.cachedP2 == 20);
}

// Test AHD attack phase
TEST_CASE("test_AHD_attack_phase_ramps_up") {
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
  CHECK(state.step == 0); // Still in attack
  CHECK(state.outValue > 0);

  int16_t prevValue = state.outValue;

  // Continue through attack phase
  for (int i = 1; i < 10; i++) {
    playbackModNext(&state);
    CHECK(state.outValue > prevValue);
    prevValue = state.outValue;
  }

  // After attack duration, should move to hold
  playbackModNext(&state);
  CHECK(state.step == 1);
}

// Test AHD hold phase
TEST_CASE("test_AHD_hold_phase_stays_at_peak") {
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
  CHECK(state.step == 1);

  // Value should be at peak (32640 * 127 / 128 = 32385)
  CHECK(state.outValue >= 32385 - 100); CHECK(state.outValue <= 32385 + 100);

  int16_t holdValue = state.outValue;

  // Hold should maintain the same value
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    if (state.step == 1) { // Still in hold
      CHECK(state.outValue == holdValue);
    }
  }

  // Should eventually move to decay
  CHECK(state.step == 2);
}

// Test AHD decay phase
TEST_CASE("test_AHD_decay_phase_ramps_to_zero") {
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
  CHECK(state.step == 2);

  int16_t prevValue = state.outValue;

  // Decay should ramp down
  for (int i = 0; i < 10; i++) {
    playbackModNext(&state);
    if (state.step == 2) { // Still in decay
      CHECK(state.outValue < prevValue);
      prevValue = state.outValue;
    }
  }

  // After decay, should be stopped
  playbackModNext(&state);
  CHECK(state.step == 0xff);
  CHECK(state.outValue == 0);
}

// Test AHD with zero attack
TEST_CASE("test_AHD_zero_attack_goes_to_hold") {
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
  CHECK(state.step == 1);

  // Should be at peak (32640 * 127 / 128 = 32385)
  CHECK(state.outValue >= 32385 - 100); CHECK(state.outValue <= 32385 + 100);
}

// Test AHD with zero hold
TEST_CASE("test_AHD_zero_hold_goes_to_decay") {
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
  CHECK(state.step == 2);
}

// Test AHD note off does nothing
TEST_CASE("test_AHD_noteOff_does_nothing") {
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
  CHECK(state.step == 1);

  int16_t valueBeforeNoteOff = state.outValue;
  uint8_t stepBeforeNoteOff = state.step;

  // Trigger note off
  playbackModNoteOff(&state);

  // Should not change anything
  CHECK(state.step == stepBeforeNoteOff);

  // Continue playing
  playbackModNext(&state);
  CHECK(state.outValue == valueBeforeNoteOff);
}

// Test AHD full cycle
TEST_CASE("test_AHD_full_cycle") {
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
  CHECK(state.step == 0);
  for (int i = 0; i < 3; i++) {
    playbackModNext(&state);
  }

  // Should move to hold
  playbackModNext(&state);
  CHECK(state.step == 1);

  // Hold phase
  int16_t holdValue = state.outValue;
  for (int i = 0; i < 2; i++) {
    playbackModNext(&state);
    if (state.step == 1) {
      CHECK(state.outValue == holdValue);
    }
  }

  // Should move to decay
  playbackModNext(&state);
  CHECK(state.step == 2);

  // Decay phase
  for (int i = 0; i < 3; i++) {
    playbackModNext(&state);
  }

  // Should be stopped
  playbackModNext(&state);
  CHECK(state.step == 0xff);
  CHECK(state.outValue == 0);
}

// Test AHD with negative amount
TEST_CASE("test_AHD_negative_amount_inverts_output") {
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
  CHECK(state.outValue < 0);
  CHECK(state.outValue >= -32385 - 100); CHECK(state.outValue <= -32385 + 100);
}

// Test LFO initialization
TEST_CASE("test_modInit_LFO_initializes_correctly") {
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

  CHECK(&mod == state.modulation);
  CHECK(state.p1Offset == 0);
  CHECK(state.p2Offset == 0);
  CHECK(state.p3Offset == 0);
  CHECK(state.p4Offset == 0);
  CHECK(state.step == 0);
  CHECK(state.counter == 0);
  CHECK(state.cachedType == modLFO);
  CHECK(state.cachedP2 == lfoTrigFree);
}

// Test LFO triangle shape oscillates
TEST_CASE("test_LFO_triangle_oscillates") {
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
  CHECK(firstValue >= 0 - 2000); CHECK(firstValue <= 0 + 2000); // Should start near zero

  // Continue - should increase
  playbackModNext(&state);
  CHECK(state.outValue > firstValue);

  // Continue to peak
  for (int i = 0; i < 3; i++) {
    int16_t prev = state.outValue;
    playbackModNext(&state);
    CHECK(state.outValue > prev);
  }

  // Should reach peak around quarter period (counter=4, phase=0.25)
  int16_t peakValue = state.outValue;
  CHECK(peakValue >= 32640 - 2000); CHECK(peakValue <= 32640 + 2000);

  // Continue past peak, should decrease
  playbackModNext(&state);
  CHECK(state.outValue < peakValue);
}

// Test LFO square wave
TEST_CASE("test_LFO_square_toggles") {
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
  CHECK(firstHalf >= 32385 - 100); CHECK(firstHalf <= 32385 + 100);

  // Should stay at same value for first half
  for (int i = 0; i < 4; i++) {
    playbackModNext(&state);
    CHECK(state.outValue == firstHalf);
  }

  // Second half should be negative (-32640 * 127 / 128 = -32385)
  playbackModNext(&state);
  int16_t secondHalf = state.outValue;
  CHECK(secondHalf >= -32385 - 100); CHECK(secondHalf <= -32385 + 100);
}

// Test LFO ramp up
TEST_CASE("test_LFO_rampUp_increases") {
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
  CHECK(state.outValue >= 0 - 5000); CHECK(state.outValue <= 0 + 5000);

  int16_t prevValue = state.outValue;
  for (int i = 0; i < 9; i++) {
    playbackModNext(&state);
    CHECK(state.outValue > prevValue);
    prevValue = state.outValue;
  }

  // Should end near peak
  CHECK(state.outValue >= 32640 - 5000); CHECK(state.outValue <= 32640 + 5000);
}

// Test LFO ramp down
TEST_CASE("test_LFO_rampDown_decreases") {
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
  CHECK(state.outValue >= 32640 - 5000); CHECK(state.outValue <= 32640 + 5000);

  int16_t prevValue = state.outValue;
  for (int i = 0; i < 9; i++) {
    playbackModNext(&state);
    CHECK(state.outValue < prevValue);
    prevValue = state.outValue;
  }

  // Should end near zero
  CHECK(state.outValue >= 0 - 5000); CHECK(state.outValue <= 0 + 5000);
}

// Test LFO wraps around with lfoFree
TEST_CASE("test_LFO_free_wraps_around") {
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
  CHECK(state.step != 0xff); // Not stopped
  CHECK(state.counter == 1); // Wrapped to 1
}

// Test LFO stops with lfoOnce
TEST_CASE("test_LFO_once_stops_after_cycle") {
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
  CHECK(state.step == 0xff); // Stopped
  CHECK(state.outValue == 0); // Held at zero

  // Should stay stopped
  playbackModNext(&state);
  CHECK(state.step == 0xff);
  CHECK(state.outValue == 0);
}

// Test LFO holds last value with lfoHold
TEST_CASE("test_LFO_hold_stops_at_last_value") {
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
  CHECK(state.step == 0xff); // Stopped
  CHECK(state.outValue == lastValue); // Held at last value

  // Should stay at that value
  playbackModNext(&state);
  CHECK(state.outValue == lastValue);
}

// Test LFO with negative amount inverts
TEST_CASE("test_LFO_negative_amount_inverts") {
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
  CHECK(state.outValue >= -32385 - 100); CHECK(state.outValue <= -32385 + 100);

  // Second half should be positive (inverted) (-32640 * -127 / 128 = 32385)
  for (int i = 0; i < 5; i++) {
    playbackModNext(&state);
  }
  CHECK(state.outValue >= 32385 - 100); CHECK(state.outValue <= 32385 + 100);
}

// Test LFO with zero period outputs zero
TEST_CASE("test_LFO_zero_period_outputs_zero") {
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
  CHECK(state.outValue == 0);
}

// Test type change detection and reinitialization
TEST_CASE("test_type_change_reinitializes") {
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
  CHECK(state.step == 0);
  CHECK(state.counter == 5);
  CHECK(state.outValue > 0);
  int16_t adsrValue = state.outValue;

  // Change type to AHD
  mod.type = modAHD;

  // Next call should detect change and reinitialize
  playbackModNext(&state);

  // Should be reinitialized: step 0, counter 0 (or 1 after first next)
  CHECK(state.cachedType == modAHD);
  CHECK(state.step == 0);
  CHECK(state.counter == 1); // Counter increments in first call
  // Value should be different (restarted from beginning)
  CHECK(state.outValue != adsrValue);
}

TEST_CASE("test_LFO_trigger_mode_change_reinitializes") {
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

  CHECK(state.counter == 8);
  CHECK(state.cachedP2 == lfoTrigFree);

  // Change trigger mode to lfoOnce
  mod.p2 = lfoTrigOnce;

  // Next call should detect change and reinitialize
  playbackModNext(&state);

  // Should be reinitialized
  CHECK(state.cachedP2 == lfoTrigOnce);
  CHECK(state.counter == 1); // Restarted and advanced one step
}

TEST_CASE("test_parameter_change_without_type_change_continues") {
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

  CHECK(state.step == 0);
  CHECK(state.counter == 5);

  // Change attack duration (p1) - should NOT reinitialize
  mod.p1 = 20;

  playbackModNext(&state);

  // Should continue from where it was (counter should increment)
  CHECK(state.step == 0);
  CHECK(state.counter == 6);
  CHECK(state.cachedType == modADSR);
}

TEST_CASE("test_LFO_shape_change_without_trigger_change_continues") {
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

  CHECK(state.counter == 4);
  int16_t triValue = state.outValue;

  // Change shape to square (p1) - should NOT reinitialize
  mod.p1 = lfoShapeSquare;

  playbackModNext(&state);

  // Should continue from same position (counter increments)
  CHECK(state.counter == 5);
  CHECK(state.cachedType == modLFO);
  CHECK(state.cachedP2 == lfoTrigFree);
  // Value will be different due to shape change, but counter continues
  CHECK(state.outValue != triValue);
}

TEST_CASE("test_LFO_random_changes_each_period") {
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
    CHECK(state.outValue == firstValue);
  }

  // After period wraps, should get a new random value
  playbackModNext(&state);
  int16_t secondValue = state.outValue;

  // New value should be different (extremely unlikely to be the same)
  CHECK(secondValue != firstValue);

  // Second value should also hold for the entire period
  for (int i = 1; i < 8; i++) {
    playbackModNext(&state);
    CHECK(state.outValue == secondValue);
  }

  // Third period should get yet another value
  playbackModNext(&state);
  int16_t thirdValue = state.outValue;
  CHECK(thirdValue != secondValue);
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

TEST_CASE("test_legacy_AY1_volume_sustain_scaling") {
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
    CHECK_MESSAGE(volume == sustainValue,
      "Legacy AY1 sustain scaling failed for value");
  }
}

// Tests are run automatically by doctest

} // TEST_SUITE("modulation")
