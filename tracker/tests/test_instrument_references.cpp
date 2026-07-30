#include "doctest.h"
#include "project_utils.h"

#include "project.h"

#include <cstring>

TEST_SUITE("instrument_references") {

// Test fixture
struct InstrumentReferencesFixture {
  Project project;

  InstrumentReferencesFixture() {
    projectInit(&project);
  }
  ~InstrumentReferencesFixture() = default;
};

TEST_CASE_FIXTURE(InstrumentReferencesFixture, "instrumentSwap should update phrase references") {
  project.instruments[1].type = InstrumentType::AY1;
  std::strcpy(project.instruments[1].name, "Test1");
  project.instruments[2].type = InstrumentType::none;
  std::strcpy(project.instruments[2].name, "Test2");

  project.phrases[0].rows[0].instrument = 1;
  project.phrases[0].rows[5].instrument = 2;
  project.phrases[1].rows[2].instrument = 1;
  project.phrases[1].rows[10].instrument = 3;

  instrumentSwap(&project, 1, 2);

  CHECK(project.instruments[1].type == InstrumentType::none);
  CHECK(std::strcmp(project.instruments[1].name, "Test2") == 0);
  CHECK(project.instruments[2].type == InstrumentType::AY1);
  CHECK(std::strcmp(project.instruments[2].name, "Test1") == 0);

  CHECK(project.phrases[0].rows[0].instrument == 2);
  CHECK(project.phrases[0].rows[5].instrument == 1);
  CHECK(project.phrases[1].rows[2].instrument == 2);
  CHECK(project.phrases[1].rows[10].instrument == 3);
}

} // TEST_SUITE("instrument_references")
