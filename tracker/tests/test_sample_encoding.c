#include "unity.h"
#include "../chipnomad_lib/project.h"
#include "../chipnomad_lib/project_instruments.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void setUp(void) {
  // Initialize FX names before each test
  fillFXNames();
}

void tearDown(void) {}

// Test empty sample (edge case)
void test_sample_empty_save_load(void) {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with no sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  strcpy(inst->name, "Empty Sample");
  inst->chip.aySample.sampleRate = 8000;
  inst->chip.aySample.fileLength = 0;
  inst->chip.aySample.sampleData = NULL;

  // Save to file
  const char* testFile = "test_empty_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  if (result != 0) {
    printf("Save error: %s\n", projectFileError);
  }
  TEST_ASSERT_EQUAL(0, result);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  TEST_ASSERT_EQUAL(0, result);

  // Verify empty sample loaded correctly
  TEST_ASSERT_EQUAL(instAYSample, p2.instruments[0].type);
  TEST_ASSERT_EQUAL_STRING("Empty Sample", p2.instruments[0].name);
  TEST_ASSERT_EQUAL(8000, p2.instruments[0].chip.aySample.sampleRate);
  TEST_ASSERT_EQUAL(0, p2.instruments[0].chip.aySample.fileLength);
  TEST_ASSERT_NULL(p2.instruments[0].chip.aySample.sampleData);

  // Cleanup
  remove(testFile);
}

// Test small sample (less than one line)
void test_sample_small_save_load(void) {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with small sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  strcpy(inst->name, "Small Sample");

  // Create test data: 10 bytes
  uint8_t testData[10] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
  inst->chip.aySample.fileLength = 10;
  inst->chip.aySample.sampleData = (uint8_t*)malloc(10);
  memcpy(inst->chip.aySample.sampleData, testData, 10);
  inst->chip.aySample.sampleRate = 8000;
  inst->chip.aySample.sampleStart = 0;
  inst->chip.aySample.sampleLength = 10;

  // Save to file
  const char* testFile = "test_small_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  TEST_ASSERT_EQUAL(0, result);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  TEST_ASSERT_EQUAL(0, result);

  // Verify sample loaded correctly
  TEST_ASSERT_EQUAL(instAYSample, p2.instruments[0].type);
  TEST_ASSERT_EQUAL_STRING("Small Sample", p2.instruments[0].name);
  TEST_ASSERT_EQUAL(10, p2.instruments[0].chip.aySample.fileLength);
  TEST_ASSERT_NOT_NULL(p2.instruments[0].chip.aySample.sampleData);

  // Verify data matches
  for (int i = 0; i < 10; i++) {
    TEST_ASSERT_EQUAL_HEX8(testData[i], p2.instruments[0].chip.aySample.sampleData[i]);
  }

  // Cleanup
  remove(testFile);
}

// Test larger sample (multiple lines)
void test_sample_large_save_load(void) {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with larger sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  strcpy(inst->name, "Large Sample");

  // Create test data: 200 bytes (more than one 80-char line)
  uint8_t* testData = (uint8_t*)malloc(200);
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
  TEST_ASSERT_EQUAL(0, result);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  TEST_ASSERT_EQUAL(0, result);

  // Verify sample loaded correctly
  TEST_ASSERT_EQUAL(instAYSample, p2.instruments[0].type);
  TEST_ASSERT_EQUAL_STRING("Large Sample", p2.instruments[0].name);
  TEST_ASSERT_EQUAL(200, p2.instruments[0].chip.aySample.fileLength);
  TEST_ASSERT_EQUAL(16000, p2.instruments[0].chip.aySample.sampleRate);
  TEST_ASSERT_EQUAL(10, p2.instruments[0].chip.aySample.sampleStart);
  TEST_ASSERT_EQUAL(180, p2.instruments[0].chip.aySample.sampleLength);
  TEST_ASSERT_EQUAL(50, p2.instruments[0].chip.aySample.sampleLoopStart);
  TEST_ASSERT_NOT_NULL(p2.instruments[0].chip.aySample.sampleData);

  // Verify data matches
  for (int i = 0; i < 200; i++) {
    TEST_ASSERT_EQUAL_HEX8(testData[i], p2.instruments[0].chip.aySample.sampleData[i]);
  }

  // Cleanup
  remove(testFile);
}

// Test maximum size sample
void test_sample_max_size_save_load(void) {
  Project p;
  projectInit(&p);

  // Create an AYSample instrument with maximum sample data
  Instrument* inst = &p.instruments[0];
  getInstrumentFunctions(instAYSample).init(inst);
  strcpy(inst->name, "Max Sample");

  // Create test data: 16384 bytes (maximum)
  uint8_t* testData = (uint8_t*)malloc(PROJECT_MAX_SAMPLE_SIZE);
  for (int i = 0; i < PROJECT_MAX_SAMPLE_SIZE; i++) {
    testData[i] = (uint8_t)((i * 7) & 0xFF); // Some pattern
  }

  inst->chip.aySample.fileLength = PROJECT_MAX_SAMPLE_SIZE;
  inst->chip.aySample.sampleData = testData;
  inst->chip.aySample.sampleRate = 22050;

  // Save to file
  const char* testFile = "test_max_sample.cnm";
  int result = instrumentSave(&p, testFile, 0);
  TEST_ASSERT_EQUAL(0, result);

  // Load from file
  Project p2;
  projectInit(&p2);
  result = instrumentLoad(&p2, testFile, 0);
  TEST_ASSERT_EQUAL(0, result);

  // Verify sample loaded correctly
  TEST_ASSERT_EQUAL(instAYSample, p2.instruments[0].type);
  TEST_ASSERT_EQUAL(PROJECT_MAX_SAMPLE_SIZE, p2.instruments[0].chip.aySample.fileLength);
  TEST_ASSERT_NOT_NULL(p2.instruments[0].chip.aySample.sampleData);

  // Verify data matches (spot check to avoid slow test)
  for (int i = 0; i < PROJECT_MAX_SAMPLE_SIZE; i += 100) {
    TEST_ASSERT_EQUAL_HEX8(testData[i], p2.instruments[0].chip.aySample.sampleData[i]);
  }

  // Cleanup
  remove(testFile);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_sample_empty_save_load);
  RUN_TEST(test_sample_small_save_load);
  RUN_TEST(test_sample_large_save_load);
  RUN_TEST(test_sample_max_size_save_load);

  return UNITY_END();
}
