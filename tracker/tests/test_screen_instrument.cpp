#include "doctest.h"

extern "C" {
#include "chipnomad_lib.h"
#include "screens.h"
#include "screen_instrument.h"

// External from screen_instrument.c
extern int cInstrument;
}

#include <cstring>
#include <cstdlib>

// Test fixture
struct ScreenInstrumentFixture {
  ChipNomadState* state;

  ScreenInstrumentFixture() {
    state = chipnomadCreate();
    projectInit(&state->project);
    chipnomadState = state;
    cInstrument = 0;
  }

  ~ScreenInstrumentFixture() {
    chipnomadDestroy(state);
    chipnomadState = nullptr;
  }
};

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change none to ay1") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with instNone
  CHECK(inst->type == instNone);

  // Simulate user pressing right arrow to increase type (row=0, col=0)
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  // Should have handled the edit
  CHECK(handled == 1);

  // Type should now be instAY1
  CHECK(inst->type == instAY1);

  // Verify AY1 defaults are set
  CHECK(inst->tableSpeed == 1);
  CHECK(inst->transposeEnabled == 1);
  CHECK(inst->chip.ay.defaultMixer == 0x01);
  CHECK(inst->chip.ay.volumeEnvelope.type == modADSR);
  CHECK(inst->chip.ay.volumeEnvelope.p3 == 15);  // Sustain
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change ay1 to ay2") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AY1
  getInstrumentFunctions(instAY1).init(inst);
  std::strcpy(inst->name, "TestInst");
  CHECK(inst->type == instAY1);

  // Simulate user pressing right arrow to increase type
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  CHECK(handled == 1);
  CHECK(inst->type == instAY2);

  // Verify AY2 defaults
  CHECK(inst->tableSpeed == 1);
  CHECK(inst->transposeEnabled == 1);
  CHECK(inst->chip.ay2.oscTone.isOn == 1);

  // Name should be cleared by free
  CHECK(std::strcmp(inst->name, "") == 0);
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change ay2 to sample") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AY2
  getInstrumentFunctions(instAY2).init(inst);
  CHECK(inst->type == instAY2);

  // Simulate user pressing right arrow
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  CHECK(handled == 1);
  CHECK(inst->type == instAYSample);
  CHECK(inst->tableSpeed == 1);
  CHECK(inst->chip.aySample.sampleData == nullptr);
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change sample with data") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AYSample with allocated data
  getInstrumentFunctions(instAYSample).init(inst);
  inst->chip.aySample.sampleData = (uint8_t*)std::malloc(1024);
  CHECK(inst->chip.aySample.sampleData != nullptr);
  std::memset(inst->chip.aySample.sampleData, 0xAA, 1024);

  // Simulate user pressing left arrow to decrease type (should free the sample data)
  int handled = instrumentCommonOnEdit(0, 0, editDecrease);

  CHECK(handled == 1);
  CHECK(inst->type == instAY2);
  CHECK(inst->tableSpeed == 1);

  // The old sampleData should have been freed
  // (valgrind/asan would catch if it wasn't)
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change decrease") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with AY2
  getInstrumentFunctions(instAY2).init(inst);
  CHECK(inst->type == instAY2);

  // Simulate user pressing left arrow to decrease type
  int handled = instrumentCommonOnEdit(0, 0, editDecrease);

  CHECK(handled == 1);
  CHECK(inst->type == instAY1);
  CHECK(inst->chip.ay.defaultMixer == 0x01);
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change big step") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with instNone (0)
  CHECK(inst->type == instNone);

  // Simulate user pressing Shift+Right (big step = 1 for this field)
  int handled = instrumentCommonOnEdit(0, 0, editIncreaseBig);

  CHECK(handled == 1);
  // Should increase by bigStep (which is 1 for type field)
  CHECK(inst->type == instAY1);
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change at max") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with AYSample (max type)
  getInstrumentFunctions(instAYSample).init(inst);
  CHECK(inst->type == instAYSample);

  // Try to increase beyond max
  int handled = instrumentCommonOnEdit(0, 0, editIncrease);

  CHECK(handled == 1);
  // Should stay at max
  CHECK(inst->type == instAYSample);
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument type change at min") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Start with instNone (min type)
  CHECK(inst->type == instNone);

  // Try to decrease below min
  int handled = instrumentCommonOnEdit(0, 0, editDecrease);

  CHECK(handled == 1);
  // Should stay at min
  CHECK(inst->type == instNone);
}

TEST_CASE_FIXTURE(ScreenInstrumentFixture, "instrument other fields dont affect type") {
  Instrument* inst = &state->project.instruments[cInstrument];

  // Initialize as AY1
  getInstrumentFunctions(instAY1).init(inst);
  CHECK(inst->type == instAY1);

  // Edit transpose field (row=2, col=0)
  int handled = instrumentCommonOnEdit(0, 2, editIncrease);
  CHECK(handled == 1);

  // Type should not change
  CHECK(inst->type == instAY1);

  // Edit table speed (row=2, col=1)
  handled = instrumentCommonOnEdit(1, 2, editIncrease);
  CHECK(handled == 1);

  // Type should still not change
  CHECK(inst->type == instAY1);
}
