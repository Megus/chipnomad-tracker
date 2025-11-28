#include "../external/unity/unity.h"
#include "../src/project.h"

void setUp(void) {
  // Setup before each test
}

void tearDown(void) {
  // Cleanup after each test
}

void test_instrumentIsEmpty_should_return_true_for_none_type(void) {
  struct Project testProject;
  projectInit(&testProject);

  TEST_ASSERT_TRUE(instrumentIsEmpty(0));
}

void test_instrumentIsEmpty_should_return_false_for_ay_type(void) {
  struct Project testProject;
  projectInit(&testProject);
  testProject.instruments[0].type = instAY;

  TEST_ASSERT_FALSE(instrumentIsEmpty(0));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_instrumentIsEmpty_should_return_true_for_none_type);
  RUN_TEST(test_instrumentIsEmpty_should_return_false_for_ay_type);
  return UNITY_END();
}