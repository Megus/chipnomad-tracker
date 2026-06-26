#include "doctest.h"

extern "C" {
#include "project.h"
#include "project_instruments.h"
}

#include <cstring>
#include <cstdlib>
#include <cstdio>

TEST_SUITE("sample_encoding") {

// Test fixture
struct SampleEncodingFixture {
  SampleEncodingFixture() {
    fillFXNames();
  }
  ~SampleEncodingFixture() = default;
};

TEST_CASE_FIXTURE(SampleEncodingFixture, "sample empty save load") {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with no sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  std::strcpy(inst->name, "Empty Sample");
  inst->chip.aySample.sampleRate = 8000;
  inst->chip.aySample.fileLength = 0;
  inst->chip.aySample.sampleData = nullptr;

  // Save to file
  const char* testFile = "test_empty_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  if (result != 0) {
    std::printf("Save error: %s\n", projectFileError);
  }
  CHECK(result == 0);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  CHECK(result == 0);

  // Verify empty sample loaded correctly
  CHECK(p2.instruments[0].type == instAYSample);
  CHECK(std::strcmp(p2.instruments[0].name, "Empty Sample") == 0);
  CHECK(p2.instruments[0].chip.aySample.sampleRate == 8000);
  CHECK(p2.instruments[0].chip.aySample.fileLength == 0);
  CHECK(p2.instruments[0].chip.aySample.sampleData == nullptr);

  // Cleanup
  std::remove(testFile);
}

TEST_CASE_FIXTURE(SampleEncodingFixture, "sample small save load") {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with small sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  std::strcpy(inst->name, "Small Sample");

  // Create test data: 10 bytes
  uint8_t testData[10] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
  inst->chip.aySample.fileLength = 10;
  inst->chip.aySample.sampleData = (uint8_t*)std::malloc(10);
  std::memcpy(inst->chip.aySample.sampleData, testData, 10);
  inst->chip.aySample.sampleRate = 8000;
  inst->chip.aySample.sampleStart = 0;
  inst->chip.aySample.sampleLength = 10;

  // Save to file
  const char* testFile = "test_small_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  CHECK(result == 0);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  CHECK(result == 0);

  // Verify sample loaded correctly
  CHECK(p2.instruments[0].type == instAYSample);
  CHECK(std::strcmp(p2.instruments[0].name, "Small Sample") == 0);
  CHECK(p2.instruments[0].chip.aySample.fileLength == 10);
  CHECK(p2.instruments[0].chip.aySample.sampleData != nullptr);

  // Verify data matches
  for (int i = 0; i < 10; i++) {
    CHECK(p2.instruments[0].chip.aySample.sampleData[i] == testData[i]);
  }

  // Cleanup
  std::remove(testFile);
}

TEST_CASE_FIXTURE(SampleEncodingFixture, "sample large save load") {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with larger sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  std::strcpy(inst->name, "Large Sample");

  // Create test data: 200 bytes (more than one 80-char line)
  uint8_t* testData = (uint8_t*)std::malloc(200);
  for (int i = 0; i < 200; i++) {
    testData[i] = (uint8_t)(i & 0xFF);
  }

  inst->chip.aySample.fileLength = 200;
  inst->chip.aySample.sampleData = testData;
  inst->chip.aySample.sampleRate = 16000;
  inst->chip.aySample.sampleStart = 10;
  inst->chip.aySample.sampleLength = 180;
  inst->chip.aySample.sampleLoopStart = 50;

  // Save to file
  const char* testFile = "test_large_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  CHECK(result == 0);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  CHECK(result == 0);

  // Verify sample loaded correctly
  CHECK(p2.instruments[0].type == instAYSample);
  CHECK(std::strcmp(p2.instruments[0].name, "Large Sample") == 0);
  CHECK(p2.instruments[0].chip.aySample.fileLength == 200);
  CHECK(p2.instruments[0].chip.aySample.sampleRate == 16000);
  CHECK(p2.instruments[0].chip.aySample.sampleStart == 10);
  CHECK(p2.instruments[0].chip.aySample.sampleLength == 180);
  CHECK(p2.instruments[0].chip.aySample.sampleLoopStart == 50);
  CHECK(p2.instruments[0].chip.aySample.sampleData != nullptr);

  // Verify data matches
  for (int i = 0; i < 200; i++) {
    CHECK(p2.instruments[0].chip.aySample.sampleData[i] == testData[i]);
  }

  // Cleanup
  std::remove(testFile);
}

TEST_CASE_FIXTURE(SampleEncodingFixture, "sample max size save load") {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with maximum sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  std::strcpy(inst->name, "Max Sample");

  // Create test data: 16384 bytes (maximum)
  uint8_t* testData = (uint8_t*)std::malloc(PROJECT_MAX_SAMPLE_SIZE);
  for (int i = 0; i < PROJECT_MAX_SAMPLE_SIZE; i++) {
    testData[i] = (uint8_t)((i * 7) & 0xFF); // Some pattern
  }

  inst->chip.aySample.fileLength = PROJECT_MAX_SAMPLE_SIZE;
  inst->chip.aySample.sampleData = testData;
  inst->chip.aySample.sampleRate = 22050;

  // Save to file
  const char* testFile = "test_max_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  CHECK(result == 0);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  CHECK(result == 0);

  // Verify sample loaded correctly
  CHECK(p2.instruments[0].type == instAYSample);
  CHECK(p2.instruments[0].chip.aySample.fileLength == PROJECT_MAX_SAMPLE_SIZE);
  CHECK(p2.instruments[0].chip.aySample.sampleData != nullptr);

  // Verify data matches (spot check to avoid slow test)
  for (int i = 0; i < PROJECT_MAX_SAMPLE_SIZE; i += 100) {
    CHECK(p2.instruments[0].chip.aySample.sampleData[i] == testData[i]);
  }

  // Cleanup
  std::remove(testFile);
}

} // TEST_SUITE("sample_encoding")
