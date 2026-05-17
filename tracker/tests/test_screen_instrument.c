#include "unity.h"
#include "chipnomad_lib.h"
#include "screens.h"
#include "screen_instrument.h"
#include <string.h>

// External from screen_instrument.c
extern int cInstrument;

static ChipNomadState* state;

void setUp(void) {
  state = chipnomadCreate();
  projectInit(&state->project);
  chipnomadState = state;
  cInstrument = 0;
}

void tearDown(void) {
  chipnomadDestroy(state);
  chipnomadState = NULL;
}

// Test changing instrument type from instNone to instAY1
void test_instrument_type_change_none_to_ay1(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with instNone
  TEST_ASSERT_EQUAL(instNone, inst->type);

  // Simulate user pressing right arrow to increase type (row=0, col=0)
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  // Should have handled the edit
  TEST_ASSERT_EQUAL(1, handled);

  // Type should now be instAY1
  TEST_ASSERT_EQUAL(instAY1, inst->type);

  // Verify AY1 defaults are set
  TEST_ASSERT_EQUAL(1, inst->tableSpeed);
  TEST_ASSERT_EQUAL(1, inst->transposeEnabled);
  TEST_ASSERT_EQUAL(0x01, inst->chip.ay.defaultMixer);
  TEST_ASSERT_EQUAL(modADSR, inst->chip.ay.volumeEnvelope.type);
  TEST_ASSERT_EQUAL(15, inst->chip.ay.volumeEnvelope.p3);  // Sustain
}

// Test changing from instAY1 to instAY2
void test_instrument_type_change_ay1_to_ay2(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AY1
  getInstrumentFunctions(instAY1).init(inst);
  strcpy(inst->name, "TestInst");
  TEST_ASSERT_EQUAL(instAY1, inst->type);

  // Simulate user pressing right arrow to increase type
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  TEST_ASSERT_EQUAL(1, handled);
  TEST_ASSERT_EQUAL(instAY2, inst->type);

  // Verify AY2 defaults
  TEST_ASSERT_EQUAL(1, inst->tableSpeed);
  TEST_ASSERT_EQUAL(1, inst->transposeEnabled);
  TEST_ASSERT_EQUAL(1, inst->chip.ay2.oscTone.isOn);

  // Name should be cleared by free
  TEST_ASSERT_EQUAL_STRING("", inst->name);
}

// Test changing from instAY2 to instAYSample
void test_instrument_type_change_ay2_to_sample(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AY2
  getInstrumentFunctions(instAY2).init(inst);
  TEST_ASSERT_EQUAL(instAY2, inst->type);

  // Simulate user pressing right arrow
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  TEST_ASSERT_EQUAL(1, handled);
  TEST_ASSERT_EQUAL(instAYSample, inst->type);
  TEST_ASSERT_EQUAL(1, inst->tableSpeed);
  TEST_ASSERT_NULL(inst->chip.aySample.sampleData);
}

// Test changing from instAYSample with allocated data to instAYWavetable
// This tests the critical memory leak scenario
void test_instrument_type_change_sample_with_data(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AYSample with allocated data
  getInstrumentFunctions(instAYSample).init(inst);
  inst->chip.aySample.sampleData = (uint8_t*)malloc(1024);
  TEST_ASSERT_NOT_NULL(inst->chip.aySample.sampleData);
  memset(inst->chip.aySample.sampleData, 0xAA, 1024);

  // Simulate user pressing right arrow (should free the sample data)
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  TEST_ASSERT_EQUAL(1, handled);
  TEST_ASSERT_EQUAL(instAYWavetable, inst->type);
  TEST_ASSERT_EQUAL(1, inst->tableSpeed);

  // The old sampleData should have been freed
  // (valgrind/asan would catch if it wasn't)
}

// Test decreasing type (going backwards)
void test_instrument_type_change_decrease(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with AY2
  getInstrumentFunctions(instAY2).init(inst);
  TEST_ASSERT_EQUAL(instAY2, inst->type);

  // Simulate user pressing left arrow to decrease type
  int handled = instrumentCommonOnEdit(0, 0, editDecrease);

  TEST_ASSERT_EQUAL(1, handled);
  TEST_ASSERT_EQUAL(instAY1, inst->type);
  TEST_ASSERT_EQUAL(0x01, inst->chip.ay.defaultMixer);
}

// Test big step increase (Shift+Right)
void test_instrument_type_change_big_step(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with instNone (0)
  TEST_ASSERT_EQUAL(instNone, inst->type);

  // Simulate user pressing Shift+Right (big step = 1 for this field)
  int handled = instrumentCommonOnEdit(0, 0, editIncreaseBig);

  TEST_ASSERT_EQUAL(1, handled);
  // Should increase by bigStep (which is 1 for type field)
  TEST_ASSERT_EQUAL(instAY1, inst->type);
}

// Test that type doesn't go beyond max (instAYWavetable = 4)
void test_instrument_type_change_at_max(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with AYWavetable (max type)
  getInstrumentFunctions(instAYWavetable).init(inst);
  TEST_ASSERT_EQUAL(instAYWavetable, inst->type);

  // Try to increase beyond max
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  TEST_ASSERT_EQUAL(1, handled);
  // Should stay at max
  TEST_ASSERT_EQUAL(instAYWavetable, inst->type);
}

// Test that type doesn't go below min (instNone = 0)
void test_instrument_type_change_at_min(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with instNone (min type)
  TEST_ASSERT_EQUAL(instNone, inst->type);

  // Try to decrease below min
  int handled = instrumentCommonOnEdit(0, 0, editDecrease);

  TEST_ASSERT_EQUAL(1, handled);
  // Should stay at min
  TEST_ASSERT_EQUAL(instNone, inst->type);
}

// Test editing other fields doesn't affect type
void test_instrument_other_fields_dont_affect_type(void) {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AY1
  getInstrumentFunctions(instAY1).init(inst);
  TEST_ASSERT_EQUAL(instAY1, inst->type);

  // Edit transpose field (row=2, col=0)
  int handled = instrumentCommonOnEdit(0, 2, editIncrease);
  TEST_ASSERT_EQUAL(1, handled);

  // Type should not change
  TEST_ASSERT_EQUAL(instAY1, inst->type);

  // Edit table speed (row=2, col=1)
  handled = instrumentCommonOnEdit(1, 2, editIncrease);
  TEST_ASSERT_EQUAL(1, handled);

  // Type should still not change
  TEST_ASSERT_EQUAL(instAY1, inst->type);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_instrument_type_change_none_to_ay1);
  RUN_TEST(test_instrument_type_change_ay1_to_ay2);
  RUN_TEST(test_instrument_type_change_ay2_to_sample);
  RUN_TEST(test_instrument_type_change_sample_with_data);
  RUN_TEST(test_instrument_type_change_decrease);
  RUN_TEST(test_instrument_type_change_big_step);
  RUN_TEST(test_instrument_type_change_at_max);
  RUN_TEST(test_instrument_type_change_at_min);
  RUN_TEST(test_instrument_other_fields_dont_affect_type);
  return UNITY_END();
}
