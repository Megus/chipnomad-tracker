#include "doctest.h"

extern "C" {
#include "project.h"
#include "project_utils.h"
}

#include <cstring>

TEST_SUITE("instrument_swap") {

// Test fixture
struct InstrumentSwapFixture {
  Project project;

  InstrumentSwapFixture() {
    projectInit(&project);
  }
  ~InstrumentSwapFixture() = default;
};

TEST_CASE_FIXTURE(InstrumentSwapFixture, "instrumentSwap should swap instrument data") {
  project.instruments[1].type = instAY1;
  std::strcpy(project.instruments[1].name, "Test1");
  project.instruments[1].tableSpeed = 10;
  project.instruments[1].transposeEnabled = 1;

  project.instruments[2].type = instNone;
  std::strcpy(project.instruments[2].name, "Test2");
  project.instruments[2].tableSpeed = 20;
  project.instruments[2].transposeEnabled = 0;

  instrumentSwap(&project, 1, 2);

  CHECK(project.instruments[1].type == instNone);
  CHECK(std::strcmp(project.instruments[1].name, "Test2") == 0);
  CHECK(project.instruments[1].tableSpeed == 20);
  CHECK(project.instruments[1].transposeEnabled == 0);

  CHECK(project.instruments[2].type == instAY1);
  CHECK(std::strcmp(project.instruments[2].name, "Test1") == 0);
  CHECK(project.instruments[2].tableSpeed == 10);
  CHECK(project.instruments[2].transposeEnabled == 1);
}

TEST_CASE_FIXTURE(InstrumentSwapFixture, "instrumentSwap should swap default tables") {
  project.tables[1].rows[0].pitchFlag = 1;
  project.tables[1].rows[0].pitchOffset = 10;
  project.tables[1].rows[0].volume = 15;

  project.tables[2].rows[0].pitchFlag = 0;
  project.tables[2].rows[0].pitchOffset = 20;
  project.tables[2].rows[0].volume = 8;

  instrumentSwap(&project, 1, 2);

  CHECK(project.tables[1].rows[0].pitchFlag == 0);
  CHECK(project.tables[1].rows[0].pitchOffset == 20);
  CHECK(project.tables[1].rows[0].volume == 8);

  CHECK(project.tables[2].rows[0].pitchFlag == 1);
  CHECK(project.tables[2].rows[0].pitchOffset == 10);
  CHECK(project.tables[2].rows[0].volume == 15);
}

TEST_CASE_FIXTURE(InstrumentSwapFixture, "instrumentSwap should handle same instrument") {
  project.instruments[1].type = instAY1;
  std::strcpy(project.instruments[1].name, "Test");

  instrumentSwap(&project, 1, 1);

  CHECK(project.instruments[1].type == instAY1);
  CHECK(std::strcmp(project.instruments[1].name, "Test") == 0);
}

TEST_CASE_FIXTURE(InstrumentSwapFixture, "instrumentSwap should handle invalid indices") {
  project.instruments[1].type = instAY1;
  std::strcpy(project.instruments[1].name, "Test");

  instrumentSwap(&project, 1, PROJECT_MAX_INSTRUMENTS);

  CHECK(project.instruments[1].type == instAY1);
  CHECK(std::strcmp(project.instruments[1].name, "Test") == 0);
}

} // TEST_SUITE("instrument_swap")
