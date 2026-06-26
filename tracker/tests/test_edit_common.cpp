#include "doctest.h"

extern "C" {
#include "chipnomad_lib.h"
#include "screens.h"
}

#include <cstring>

TEST_SUITE("edit_common") {

// Reference the global chipnomadState from app.c
extern "C" ChipNomadState* chipnomadState;
static ChipNomadState testState;

#define S(r, c) chipnomadState->project.song[r][c]
#define H(r, c) chipnomadState->project.songHighlight[r][c]

// Test fixture for common setup/teardown
struct EditCommonFixture {
  EditCommonFixture() {
    std::memset(&testState, 0, sizeof(testState));
    chipnomadState = &testState;
    projectInit(&chipnomadState->project);
    chipnomadState->project.tracksCount = 3;
  }
  ~EditCommonFixture() = default;
};

// applySongMoveDown tests

TEST_CASE_FIXTURE(EditCommonFixture, "moveDown: single cell") {
  S(2, 0) = 10;
  H(2, 0) = 1;

  int result = applySongMoveDown(0, 2, 0, 2);

  CHECK(result == 1);
  CHECK(S(2, 0) == EMPTY_VALUE_16);
  CHECK(H(2, 0) == 0);
  CHECK(S(3, 0) == 10);
  CHECK(H(3, 0) == 1);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveDown: pushes below") {
  S(2, 0) = 10;
  S(5, 0) = 20;

  int result = applySongMoveDown(0, 2, 0, 2);

  CHECK(result == 1);
  CHECK(S(3, 0) == 10);
  CHECK(S(6, 0) == 20);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveDown: fails when bottom full") {
  S(2, 0) = 10;
  S(PROJECT_MAX_LENGTH - 1, 0) = 99;

  int result = applySongMoveDown(0, 2, 0, 2);

  CHECK(result == 0);
  CHECK(S(2, 0) == 10);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveDown: multiple columns") {
  S(1, 0) = 10; H(1, 0) = 1;
  S(1, 1) = 20; H(1, 1) = 1;

  int result = applySongMoveDown(0, 1, 1, 1);

  CHECK(result == 1);
  CHECK(S(1, 0) == EMPTY_VALUE_16);
  CHECK(H(1, 0) == 0);
  CHECK(S(1, 1) == EMPTY_VALUE_16);
  CHECK(H(1, 1) == 0);
  CHECK(S(2, 0) == 10);
  CHECK(H(2, 0) == 1);
  CHECK(S(2, 1) == 20);
  CHECK(H(2, 1) == 1);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveDown: selection range") {
  S(3, 0) = 10;
  S(4, 0) = 11;
  S(5, 0) = 12;

  int result = applySongMoveDown(0, 3, 0, 5);

  CHECK(result == 1);
  CHECK(S(3, 0) == EMPTY_VALUE_16);
  CHECK(S(4, 0) == 10);
  CHECK(S(5, 0) == 11);
  CHECK(S(6, 0) == 12);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveDown: does not affect other columns") {
  S(2, 0) = 10;
  S(2, 2) = 99;

  applySongMoveDown(0, 2, 0, 2);

  CHECK(S(2, 2) == 99);
}

// applySongMoveUp tests

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: single cell") {
  S(3, 0) = 10;
  H(3, 0) = 1;

  int result = applySongMoveUp(0, 3, 0, 3);

  CHECK(result == 1);
  CHECK(S(2, 0) == 10);
  CHECK(H(2, 0) == 1);
  CHECK(S(3, 0) == EMPTY_VALUE_16);
  CHECK(H(3, 0) == 0);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: fails at row zero") {
  S(0, 0) = 10;

  int result = applySongMoveUp(0, 0, 0, 0);

  CHECK(result == 0);
  CHECK(S(0, 0) == 10);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: fails when above occupied") {
  S(2, 0) = 99;
  S(3, 0) = 10;

  int result = applySongMoveUp(0, 3, 0, 3);

  CHECK(result == 0);
  CHECK(S(3, 0) == 10);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: multiple columns") {
  S(3, 0) = 10; H(3, 0) = 1;
  S(3, 1) = 20;

  int result = applySongMoveUp(0, 3, 1, 3);

  CHECK(result == 1);
  CHECK(S(2, 0) == 10);
  CHECK(H(2, 0) == 1);
  CHECK(S(2, 1) == 20);
  CHECK(S(3, 0) == EMPTY_VALUE_16);
  CHECK(S(3, 1) == EMPTY_VALUE_16);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: selection range") {
  S(3, 0) = 10;
  S(4, 0) = 11;
  S(5, 0) = 12;

  int result = applySongMoveUp(0, 3, 0, 5);

  CHECK(result == 1);
  CHECK(S(2, 0) == 10);
  CHECK(S(3, 0) == 11);
  CHECK(S(4, 0) == 12);
  CHECK(S(5, 0) == EMPTY_VALUE_16);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: does not affect other columns") {
  S(3, 0) = 10;
  S(3, 2) = 99;

  applySongMoveUp(0, 3, 0, 3);

  CHECK(S(3, 2) == 99);
}

TEST_CASE_FIXTURE(EditCommonFixture, "moveUp: fails if any column blocked") {
  S(2, 1) = 99;
  S(3, 0) = 10;
  S(3, 1) = 20;

  int result = applySongMoveUp(0, 3, 1, 3);

  CHECK(result == 0);
  CHECK(S(3, 0) == 10);
  CHECK(S(3, 1) == 20);
}

} // TEST_SUITE("edit_common")
