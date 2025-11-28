#include "../external/unity/unity.h"
#include "../src/project.h"
#include <string.h>

void setUp(void) {
  projectInit(&project);
}

void tearDown(void) {
  // Cleanup after each test
}


void test_instrumentSwap_should_update_phrase_references(void) {
  // Setup test data
  project.instruments[1].type = instAY;
  strcpy(project.instruments[1].name, "Test1");
  project.instruments[2].type = instNone;
  strcpy(project.instruments[2].name, "Test2");

  // Add phrase references
  project.phrases[0].instruments[0] = 1;
  project.phrases[0].instruments[5] = 2;
  project.phrases[1].instruments[2] = 1;
  project.phrases[1].instruments[10] = 3; // Different instrument

  // Perform swap
  instrumentSwap(1, 2);

  // Verify instrument data was swapped
  TEST_ASSERT_EQUAL(instNone, project.instruments[1].type);
  TEST_ASSERT_EQUAL_STRING("Test2", project.instruments[1].name);
  TEST_ASSERT_EQUAL(instAY, project.instruments[2].type);
  TEST_ASSERT_EQUAL_STRING("Test1", project.instruments[2].name);

  // Verify phrase references were swapped
  TEST_ASSERT_EQUAL(2, project.phrases[0].instruments[0]); // Was 1, now 2
  TEST_ASSERT_EQUAL(1, project.phrases[0].instruments[5]); // Was 2, now 1
  TEST_ASSERT_EQUAL(2, project.phrases[1].instruments[2]); // Was 1, now 2
  TEST_ASSERT_EQUAL(3, project.phrases[1].instruments[10]); // Should remain unchanged
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_instrumentSwap_should_update_phrase_references);
  return UNITY_END();
}