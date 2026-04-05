#include "unity.h"
#include "project.h"
#include "project_utils.h"
#include <string.h>

static Project project;

void setUp(void) {
  projectInit(&project);
}

void tearDown(void) {
}

void test_instrumentSwap_should_swap_instrument_data(void) {
  project.instruments[1].type = instAY;
  strcpy(project.instruments[1].name, "Test1");
  project.instruments[1].tableSpeed = 10;
  project.instruments[1].transposeEnabled = 1;

  project.instruments[2].type = instNone;
  strcpy(project.instruments[2].name, "Test2");
  project.instruments[2].tableSpeed = 20;
  project.instruments[2].transposeEnabled = 0;

  instrumentSwap(&project, 1, 2);

  TEST_ASSERT_EQUAL(instNone, project.instruments[1].type);
  TEST_ASSERT_EQUAL_STRING("Test2", project.instruments[1].name);
  TEST_ASSERT_EQUAL(20, project.instruments[1].tableSpeed);
  TEST_ASSERT_EQUAL(0, project.instruments[1].transposeEnabled);

  TEST_ASSERT_EQUAL(instAY, project.instruments[2].type);
  TEST_ASSERT_EQUAL_STRING("Test1", project.instruments[2].name);
  TEST_ASSERT_EQUAL(10, project.instruments[2].tableSpeed);
  TEST_ASSERT_EQUAL(1, project.instruments[2].transposeEnabled);
}

void test_instrumentSwap_should_swap_default_tables(void) {
  project.tables[1].rows[0].pitchFlag = 1;
  project.tables[1].rows[0].pitchOffset = 10;
  project.tables[1].rows[0].volume = 15;

  project.tables[2].rows[0].pitchFlag = 0;
  project.tables[2].rows[0].pitchOffset = 20;
  project.tables[2].rows[0].volume = 8;

  instrumentSwap(&project, 1, 2);

  TEST_ASSERT_EQUAL(0, project.tables[1].rows[0].pitchFlag);
  TEST_ASSERT_EQUAL(20, project.tables[1].rows[0].pitchOffset);
  TEST_ASSERT_EQUAL(8, project.tables[1].rows[0].volume);

  TEST_ASSERT_EQUAL(1, project.tables[2].rows[0].pitchFlag);
  TEST_ASSERT_EQUAL(10, project.tables[2].rows[0].pitchOffset);
  TEST_ASSERT_EQUAL(15, project.tables[2].rows[0].volume);
}

void test_instrumentSwap_should_handle_same_instrument(void) {
  project.instruments[1].type = instAY;
  strcpy(project.instruments[1].name, "Test");

  instrumentSwap(&project, 1, 1);

  TEST_ASSERT_EQUAL(instAY, project.instruments[1].type);
  TEST_ASSERT_EQUAL_STRING("Test", project.instruments[1].name);
}

void test_instrumentSwap_should_handle_invalid_indices(void) {
  project.instruments[1].type = instAY;
  strcpy(project.instruments[1].name, "Test");

  instrumentSwap(&project, 1, PROJECT_MAX_INSTRUMENTS);

  TEST_ASSERT_EQUAL(instAY, project.instruments[1].type);
  TEST_ASSERT_EQUAL_STRING("Test", project.instruments[1].name);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_instrumentSwap_should_swap_instrument_data);
  RUN_TEST(test_instrumentSwap_should_swap_default_tables);
  RUN_TEST(test_instrumentSwap_should_handle_same_instrument);
  RUN_TEST(test_instrumentSwap_should_handle_invalid_indices);
  return UNITY_END();
}
