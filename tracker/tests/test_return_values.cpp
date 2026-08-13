#include "doctest.h"
#include "chipnomad_lib.h"
#include "project.h"
#include "project_instruments.h"
#include "project_utils.h"
#include "pitch_table_utils.h"
#include "import_vts.h"
#include "error.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

// Stubs needed by VTS import
extern "C" {
extern int cInstrument;
}

extern ChipNomadState* chipnomadState;

TEST_SUITE("return_values") {

struct ReturnValueFixture {
  ChipNomadState* state;

  ReturnValueFixture() {
    state = chipnomadCreate();
    projectInit(&state->project);
    chipnomadState = state;
    cInstrument = 0;
    fillFXNames();
  }

  ~ReturnValueFixture() {
    chipnomadDestroy(state);
    chipnomadState = nullptr;
  }
};

// ============================================================================
// Project save/load
// ============================================================================

TEST_CASE_FIXTURE(ReturnValueFixture, "projectSave returns 1 on success") {
  const char* testFile = "test_rv_save.cnm";
  projectInitAY(&state->project);
  int result = projectSave(&state->project, testFile);
  CHECK(result == 1);
  std::remove(testFile);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "projectSave returns 0 on failure") {
  // Try to save to an invalid path (directory that doesn't exist)
  int result = projectSave(&state->project, "/nonexistent_dir/test.cnm");
  CHECK(result == 0);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "projectLoad returns 1 on success") {
  // Save a valid project first
  const char* testFile = "test_rv_load.cnm";
  projectInitAY(&state->project);
  int saveResult = projectSave(&state->project, testFile);
  REQUIRE(saveResult == 1);

  Project p2;
  projectInit(&p2);
  int result = projectLoad(&p2, testFile);
  if (!result) {
    std::printf("projectLoad error: %s\n", chipnomad::Error::message);
  }
  CHECK(result == 1);
  std::remove(testFile);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "projectLoad returns 0 on missing file") {
  Project p2;
  projectInit(&p2);
  int result = projectLoad(&p2, "nonexistent_file.cnm");
  CHECK(result == 0);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "projectLoad returns 0 on invalid file") {
  // Create a file with garbage content
  const char* testFile = "test_rv_invalid.cnm";
  FILE* f = std::fopen(testFile, "w");
  std::fprintf(f, "NOT A VALID PROJECT FILE\n");
  std::fclose(f);

  Project p2;
  projectInit(&p2);
  int result = projectLoad(&p2, testFile);
  CHECK(result == 0);
  // Error message should be set
  CHECK(std::strlen(chipnomad::Error::message) > 0);
  std::remove(testFile);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "projectLoad bundled project") {
  Project p2;
  projectInit(&p2);
  int result = projectLoad(&p2, "packaging/common/projects/SkyTrain Funk.cnm");
  if (!result) {
    std::printf("Load error: %s\n", chipnomad::Error::message);
  }
  CHECK(result == 1);
}

// ============================================================================
// Instrument save/load
// ============================================================================

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentSave returns 1 on success") {
  getInstrumentFunctions(InstrumentType::AY1).init(&state->project.instruments[0]);
  std::strcpy(state->project.instruments[0].name, "TestInst");

  const char* testFile = "test_rv_inst.cni";
  int result = instrumentSave(&state->project, testFile, 0);
  CHECK(result == 1);
  std::remove(testFile);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentSave returns 0 on failure") {
  int result = instrumentSave(&state->project, "/nonexistent_dir/test.cni", 0);
  CHECK(result == 0);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentLoad returns 1 on success") {
  getInstrumentFunctions(InstrumentType::AY2).init(&state->project.instruments[0]);
  std::strcpy(state->project.instruments[0].name, "AY2Inst");

  const char* testFile = "test_rv_inst_load.cni";
  instrumentSave(&state->project, testFile, 0);

  int result = instrumentLoad(&state->project, testFile, 1);
  CHECK(result == 1);
  CHECK(state->project.instruments[1].type == InstrumentType::AY2);
  CHECK(std::strcmp(state->project.instruments[1].name, "AY2Inst") == 0);
  std::remove(testFile);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentLoad returns 0 on missing file") {
  int result = instrumentLoad(&state->project, "nonexistent.cni", 0);
  CHECK(result == 0);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentLoad returns 0 on invalid file") {
  const char* testFile = "test_rv_inst_bad.cni";
  FILE* f = std::fopen(testFile, "w");
  std::fprintf(f, "GARBAGE DATA\n");
  std::fclose(f);

  int result = instrumentLoad(&state->project, testFile, 0);
  CHECK(result == 0);
  std::remove(testFile);
}

// ============================================================================
// Pitch table CSV
// ============================================================================

TEST_CASE_FIXTURE(ReturnValueFixture, "pitchTableLoadCSV returns 0 on missing file") {
  int result = pitchTableLoadCSV(&state->project, "nonexistent.csv");
  CHECK(result == 0);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "pitchTableLoadCSV returns 0 on invalid CSV") {
  const char* testFile = "test_rv_pitch_bad.csv";
  FILE* f = std::fopen(testFile, "w");
  std::fprintf(f, "no,valid,columns\n1,2,3\n");
  std::fclose(f);

  int result = pitchTableLoadCSV(&state->project, testFile);
  CHECK(result == 0);
  std::remove(testFile);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "pitchTableSaveCSV returns 1 on success") {
  // Set up a minimal pitch table
  state->project.pitchTable.length = 2;
  std::strcpy(state->project.pitchTable.noteNames[0], "C-4");
  std::strcpy(state->project.pitchTable.noteNames[1], "D-4");
  state->project.pitchTable.values[0] = 1000;
  state->project.pitchTable.values[1] = 900;

  int result = pitchTableSaveCSV(&state->project, ".", "test_rv_pitch");
  CHECK(result == 1);
  std::remove("./test_rv_pitch.csv");
}

TEST_CASE_FIXTURE(ReturnValueFixture, "pitchTableSaveCSV returns 0 on invalid path") {
  int result = pitchTableSaveCSV(&state->project, "/nonexistent_dir", "test");
  CHECK(result == 0);
}

// ============================================================================
// VTS import
// ============================================================================

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentLoadVTS returns 0 on missing file") {
  int result = instrumentLoadVTS("nonexistent.vts", 0);
  CHECK(result == 0);
}

TEST_CASE_FIXTURE(ReturnValueFixture, "instrumentLoadVTS returns 0 on invalid index") {
  int result = instrumentLoadVTS("somefile.vts", -1);
  CHECK(result == 0);
}

} // TEST_SUITE("return_values")
