#include "doctest.h"

extern "C" {
#include "project.h"
}

#include <cstring>

TEST_SUITE("project") {

// Test fixture
struct ProjectFixture {
  Project p;

  ProjectFixture() {
    projectInit(&p);
  }
  ~ProjectFixture() = default;
};

// projectInit tests

TEST_CASE_FIXTURE(ProjectFixture, "projectInit song is empty") {
  for (int r = 0; r < PROJECT_MAX_LENGTH; r++)
    for (int c = 0; c < PROJECT_MAX_TRACKS; c++)
      CHECK(p.song[r][c] == EMPTY_VALUE_16);
}

TEST_CASE_FIXTURE(ProjectFixture, "projectInit default groove") {
  CHECK(p.grooves[0].speed[0] == 6);
  CHECK(p.grooves[0].speed[1] == 6);
  CHECK(p.grooves[0].speed[2] == EMPTY_VALUE_8);
}

TEST_CASE_FIXTURE(ProjectFixture, "projectInit other grooves empty") {
  for (int g = 1; g < PROJECT_MAX_GROOVES; g++)
    CHECK(grooveIsEmpty(&p, g));
}

TEST_CASE_FIXTURE(ProjectFixture, "projectInit instruments empty") {
  for (int i = 0; i < PROJECT_MAX_INSTRUMENTS; i++)
    CHECK(instrumentIsEmpty(&p, i));
}

TEST_CASE_FIXTURE(ProjectFixture, "projectInit phrases empty") {
  for (int i = 0; i < PROJECT_MAX_PHRASES; i++)
    CHECK(phraseIsEmpty(&p, i));
}

TEST_CASE_FIXTURE(ProjectFixture, "projectInit chains empty") {
  for (int i = 0; i < PROJECT_MAX_CHAINS; i++)
    CHECK(chainIsEmpty(&p, i));
}

TEST_CASE_FIXTURE(ProjectFixture, "projectInit tables empty") {
  for (int i = 0; i < PROJECT_MAX_TABLES; i++)
    CHECK(tableIsEmpty(&p, i));
}

// instrumentIsEmpty tests

TEST_CASE_FIXTURE(ProjectFixture, "instrumentIsEmpty true for none") {
  CHECK(instrumentIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "instrumentIsEmpty false for ay") {
  p.instruments[0].type = instAY1;
  CHECK_FALSE(instrumentIsEmpty(&p, 0));
}

// chainIsEmpty tests

TEST_CASE_FIXTURE(ProjectFixture, "chainIsEmpty true after init") {
  CHECK(chainIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "chainIsEmpty false with phrase") {
  p.chains[0].rows[5].phrase = 1;
  CHECK_FALSE(chainIsEmpty(&p, 0));
}

// phraseIsEmpty tests

TEST_CASE_FIXTURE(ProjectFixture, "phraseIsEmpty true after init") {
  CHECK(phraseIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "phraseIsEmpty false with note") {
  p.phrases[0].rows[0].note = 42;
  CHECK_FALSE(phraseIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "phraseIsEmpty false with instrument") {
  p.phrases[0].rows[3].instrument = 1;
  CHECK_FALSE(phraseIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "phraseIsEmpty false with volume") {
  p.phrases[0].rows[7].volume = 10;
  CHECK_FALSE(phraseIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "phraseIsEmpty false with fx") {
  p.phrases[0].rows[0].fx[1][0] = fxARP;
  CHECK_FALSE(phraseIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "phraseIsEmpty false with fx value") {
  p.phrases[0].rows[0].fx[2][1] = 0x50;
  CHECK_FALSE(phraseIsEmpty(&p, 0));
}

// tableIsEmpty tests

TEST_CASE_FIXTURE(ProjectFixture, "tableIsEmpty true after init") {
  CHECK(tableIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "tableIsEmpty false with pitch flag") {
  p.tables[0].rows[0].pitchFlag = 1;
  CHECK_FALSE(tableIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "tableIsEmpty false with pitch offset") {
  p.tables[0].rows[0].pitchOffset = 5;
  CHECK_FALSE(tableIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "tableIsEmpty false with volume") {
  p.tables[0].rows[0].volume = 10;
  CHECK_FALSE(tableIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "tableIsEmpty false with fx") {
  p.tables[0].rows[0].fx[3][0] = fxVOL;
  CHECK_FALSE(tableIsEmpty(&p, 0));
}

// grooveIsEmpty tests

TEST_CASE_FIXTURE(ProjectFixture, "grooveIsEmpty true for unused") {
  CHECK(grooveIsEmpty(&p, 1));
}

TEST_CASE_FIXTURE(ProjectFixture, "grooveIsEmpty false with speed") {
  p.grooves[1].speed[0] = 8;
  CHECK_FALSE(grooveIsEmpty(&p, 1));
}

// Clear function tests

TEST_CASE_FIXTURE(ProjectFixture, "phraseClear") {
  p.phrases[0].rows[0].note = 42;
  p.phrases[0].rows[5].instrument = 1;
  p.phrases[0].rows[10].fx[0][0] = fxARP;
  p.phrases[0].rows[10].fx[0][1] = 0x37;
  phraseClear(&p.phrases[0]);
  CHECK(phraseIsEmpty(&p, 0));
}

TEST_CASE_FIXTURE(ProjectFixture, "chainClear") {
  p.chains[0].rows[0].phrase = 5;
  p.chains[0].rows[0].transpose = 3;
  chainClear(&p.chains[0]);
  CHECK(chainIsEmpty(&p, 0));
  CHECK(p.chains[0].rows[0].transpose == 0);
}

TEST_CASE_FIXTURE(ProjectFixture, "instrumentClear") {
  p.instruments[0].type = instAY1;
  std::strcpy(p.instruments[0].name, "Test");
  instrumentClear(&p.instruments[0]);
  CHECK(instrumentIsEmpty(&p, 0));
  CHECK(std::strcmp(p.instruments[0].name, "") == 0);
}

TEST_CASE_FIXTURE(ProjectFixture, "tableClear") {
  p.tables[0].rows[0].pitchFlag = 1;
  p.tables[0].rows[0].pitchOffset = 10;
  p.tables[0].rows[0].fx[0][0] = fxVOL;
  tableClear(&p.tables[0]);
  CHECK(tableIsEmpty(&p, 0));
}

// noteName tests

TEST_CASE_FIXTURE(ProjectFixture, "noteName off") {
  CHECK(std::strcmp(noteName(&p, NOTE_OFF), "OFF") == 0);
}

TEST_CASE_FIXTURE(ProjectFixture, "noteName empty") {
  CHECK(std::strcmp(noteName(&p, EMPTY_VALUE_8), "---") == 0);
}

TEST_CASE_FIXTURE(ProjectFixture, "noteName out of range") {
  p.pitchTable.length = 12;
  CHECK(std::strcmp(noteName(&p, 13), "---") == 0);
}

TEST_CASE_FIXTURE(ProjectFixture, "noteName valid") {
  p.pitchTable.length = 12;
  std::strcpy(p.pitchTable.noteNames[0], "C-4");
  CHECK(std::strcmp(noteName(&p, 0), "C-4") == 0);
}

// Track count tests

TEST_CASE_FIXTURE(ProjectFixture, "getChipTracks ay") {
  CHECK(projectGetChipTracks(&p, 0) == 3);
}

TEST_CASE_FIXTURE(ProjectFixture, "getTotalTracks single chip") {
  p.chipsCount = 1;
  CHECK(projectGetTotalTracks(&p) == 3);
}

TEST_CASE_FIXTURE(ProjectFixture, "getTotalTracks multiple chips") {
  p.chipsCount = 3;
  CHECK(projectGetTotalTracks(&p) == 9);
}

// fillFXNames tests

TEST_CASE("fillFXNames common") {
  fillFXNames();
  CHECK(std::strcmp(fxNames[fxARP].name, "ARP") == 0);
  CHECK(std::strcmp(fxNames[fxHOP].name, "HOP") == 0);
  CHECK(std::strcmp(fxNames[fxVOL].name, "VOL") == 0);
  CHECK(std::strcmp(fxNames[fxVSL].name, "VSL") == 0);
}

TEST_CASE("fillFXNames ay") {
  fillFXNames();
  CHECK(std::strcmp(fxNames[fxAYM].name, "AYM") == 0);
  CHECK(std::strcmp(fxNames[fxEPH].name, "EPH") == 0);
}

TEST_CASE("fillFXNames unknown") {
  fillFXNames();
  CHECK(std::strcmp(fxNames[200].name, "---") == 0);
}

} // TEST_SUITE("project")
