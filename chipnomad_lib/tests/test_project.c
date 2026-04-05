#include "unity.h"
#include "project.h"

static Project testProject;

void setUp(void) {
  projectInit(&testProject);
}

void tearDown(void) {
}

void test_instrumentIsEmpty_should_return_true_for_none_type(void) {
  TEST_ASSERT_TRUE(instrumentIsEmpty(&testProject, 0));
}

void test_instrumentIsEmpty_should_return_false_for_ay_type(void) {
  testProject.instruments[0].type = instAY;
  TEST_ASSERT_FALSE(instrumentIsEmpty(&testProject, 0));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_instrumentIsEmpty_should_return_true_for_none_type);
  RUN_TEST(test_instrumentIsEmpty_should_return_false_for_ay_type);
  return UNITY_END();
}
