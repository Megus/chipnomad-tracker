#include "unity.h"
#include "chipnomad_lib.h"
#include "screens.h"
#include <string.h>

// Provide the global chipnomadState
ChipNomadState* chipnomadState;
static ChipNomadState testState;

#define S(r, c) chipnomadState->project.song[r][c]
#define H(r, c) chipnomadState->project.songHighlight[r][c]

void setUp(void) {
  memset(&testState, 0, sizeof(testState));
  chipnomadState = &testState;
  projectInit(&chipnomadState->project);
  chipnomadState->project.tracksCount = 3;
}

void tearDown(void) {
}

// applySongMoveDown

void test_moveDown_single_cell(void) {
  S(2, 0) = 10;
  H(2, 0) = 1;

  int result = applySongMoveDown(0, 2, 0, 2);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(2, 0));
  TEST_ASSERT_EQUAL(0, H(2, 0));
  TEST_ASSERT_EQUAL(10, S(3, 0));
  TEST_ASSERT_EQUAL(1, H(3, 0));
}

void test_moveDown_pushes_below(void) {
  S(2, 0) = 10;
  S(5, 0) = 20;

  int result = applySongMoveDown(0, 2, 0, 2);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(10, S(3, 0));
  TEST_ASSERT_EQUAL(20, S(6, 0));
}

void test_moveDown_fails_when_bottom_full(void) {
  S(2, 0) = 10;
  S(PROJECT_MAX_LENGTH - 1, 0) = 99;

  int result = applySongMoveDown(0, 2, 0, 2);

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(10, S(2, 0));
}

void test_moveDown_multiple_columns(void) {
  S(1, 0) = 10; H(1, 0) = 1;
  S(1, 1) = 20; H(1, 1) = 1;

  int result = applySongMoveDown(0, 1, 1, 1);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(1, 0));
  TEST_ASSERT_EQUAL(0, H(1, 0));
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(1, 1));
  TEST_ASSERT_EQUAL(0, H(1, 1));
  TEST_ASSERT_EQUAL(10, S(2, 0));
  TEST_ASSERT_EQUAL(1, H(2, 0));
  TEST_ASSERT_EQUAL(20, S(2, 1));
  TEST_ASSERT_EQUAL(1, H(2, 1));
}

void test_moveDown_selection_range(void) {
  S(3, 0) = 10;
  S(4, 0) = 11;
  S(5, 0) = 12;

  int result = applySongMoveDown(0, 3, 0, 5);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(3, 0));
  TEST_ASSERT_EQUAL(10, S(4, 0));
  TEST_ASSERT_EQUAL(11, S(5, 0));
  TEST_ASSERT_EQUAL(12, S(6, 0));
}

void test_moveDown_does_not_affect_other_columns(void) {
  S(2, 0) = 10;
  S(2, 2) = 99;

  applySongMoveDown(0, 2, 0, 2);

  TEST_ASSERT_EQUAL(99, S(2, 2));
}

// applySongMoveUp

void test_moveUp_single_cell(void) {
  S(3, 0) = 10;
  H(3, 0) = 1;

  int result = applySongMoveUp(0, 3, 0, 3);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(10, S(2, 0));
  TEST_ASSERT_EQUAL(1, H(2, 0));
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(3, 0));
  TEST_ASSERT_EQUAL(0, H(3, 0));
}

void test_moveUp_fails_at_row_zero(void) {
  S(0, 0) = 10;

  int result = applySongMoveUp(0, 0, 0, 0);

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(10, S(0, 0));
}

void test_moveUp_fails_when_above_occupied(void) {
  S(2, 0) = 99;
  S(3, 0) = 10;

  int result = applySongMoveUp(0, 3, 0, 3);

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(10, S(3, 0));
}

void test_moveUp_multiple_columns(void) {
  S(3, 0) = 10; H(3, 0) = 1;
  S(3, 1) = 20;

  int result = applySongMoveUp(0, 3, 1, 3);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(10, S(2, 0));
  TEST_ASSERT_EQUAL(1, H(2, 0));
  TEST_ASSERT_EQUAL(20, S(2, 1));
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(3, 0));
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(3, 1));
}

void test_moveUp_selection_range(void) {
  S(3, 0) = 10;
  S(4, 0) = 11;
  S(5, 0) = 12;

  int result = applySongMoveUp(0, 3, 0, 5);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_EQUAL(10, S(2, 0));
  TEST_ASSERT_EQUAL(11, S(3, 0));
  TEST_ASSERT_EQUAL(12, S(4, 0));
  TEST_ASSERT_EQUAL(EMPTY_VALUE_16, S(5, 0));
}

void test_moveUp_does_not_affect_other_columns(void) {
  S(3, 0) = 10;
  S(3, 2) = 99;

  applySongMoveUp(0, 3, 0, 3);

  TEST_ASSERT_EQUAL(99, S(3, 2));
}

void test_moveUp_fails_if_any_column_blocked(void) {
  S(2, 1) = 99;
  S(3, 0) = 10;
  S(3, 1) = 20;

  int result = applySongMoveUp(0, 3, 1, 3);

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(10, S(3, 0));
  TEST_ASSERT_EQUAL(20, S(3, 1));
}

int main(void) {
  UNITY_BEGIN();
  // applySongMoveDown
  RUN_TEST(test_moveDown_single_cell);
  RUN_TEST(test_moveDown_pushes_below);
  RUN_TEST(test_moveDown_fails_when_bottom_full);
  RUN_TEST(test_moveDown_multiple_columns);
  RUN_TEST(test_moveDown_selection_range);
  RUN_TEST(test_moveDown_does_not_affect_other_columns);
  // applySongMoveUp
  RUN_TEST(test_moveUp_single_cell);
  RUN_TEST(test_moveUp_fails_at_row_zero);
  RUN_TEST(test_moveUp_fails_when_above_occupied);
  RUN_TEST(test_moveUp_multiple_columns);
  RUN_TEST(test_moveUp_selection_range);
  RUN_TEST(test_moveUp_does_not_affect_other_columns);
  RUN_TEST(test_moveUp_fails_if_any_column_blocked);
  return UNITY_END();
}
