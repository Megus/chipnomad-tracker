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

void test_instrumentSwap_should_update_phrase_references(void) {
  project.instruments[1].type = instAY;
  strcpy(project.instruments[1].name, "Test1");
  project.instruments[2].type = instNone;
  strcpy(project.instruments[2].name, "Test2");

  project.phrases[0].rows[0].instrument = 1;
  project.phrases[0].rows[5].instrument = 2;
  project.phrases[1].rows[2].instrument = 1;
  project.phrases[1].rows[10].instrument = 3;

  instrumentSwap(&project, 1, 2);

  TEST_ASSERT_EQUAL(instNone, project.instruments[1].type);
  TEST_ASSERT_EQUAL_STRING("Test2", project.instruments[1].name);
  TEST_ASSERT_EQUAL(instAY, project.instruments[2].type);
  TEST_ASSERT_EQUAL_STRING("Test1", project.instruments[2].name);

  TEST_ASSERT_EQUAL(2, project.phrases[0].rows[0].instrument);
  TEST_ASSERT_EQUAL(1, project.phrases[0].rows[5].instrument);
  TEST_ASSERT_EQUAL(2, project.phrases[1].rows[2].instrument);
  TEST_ASSERT_EQUAL(3, project.phrases[1].rows[10].instrument);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_instrumentSwap_should_update_phrase_references);
  return UNITY_END();
}
