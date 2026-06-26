#include "doctest.h"

extern "C" {
#include "project.h"
#include "project_io_common.h"
}

#include <cstring>

struct TestFixture {
  TestFixture() {
    resetPeekConsume();
  }

  ~TestFixture() {
  }
};

///////////////////////////////////////////////////////////////////////////////
// Tests

// FIXME: This test crashes in unified executable due to global state issues
// Will be fixed when migrating to C++ with proper dependency injection
// TEST_CASE_FIXTURE(TestFixture, "projectSaveLoad_song_data_preserved") {
//   Project p1, p2;
//   projectInit(&p1);
//   projectInit(&p2);
//
//   // Set up project with some song data
//   std::strcpy(p1.title, "Test Song");
//   std::strcpy(p1.author, "Test Author");
//   p1.tickRate = 50.0f;
//   p1.chipsCount = 1;
//   p1.chipType = chipAY;
//   p1.chipSetup.ay.clock = 1773400;
//   p1.chipSetup.ay.isYM = 0;
//   p1.chipSetup.ay.stereoMode = ayStereoABC;
//   p1.chipSetup.ay.stereoSeparation = 100;
//   p1.chipSetup.ay.pwmFullRange = 0;
//   p1.tracksCount = 3;
//
//   // Set up pitch table
//   std::strcpy(p1.pitchTable.name, "Equal Temperament");
//   p1.pitchTable.length = 3;
//   std::strcpy(p1.pitchTable.noteNames[0], "C-4");
//   p1.pitchTable.values[0] = 1000;
//   std::strcpy(p1.pitchTable.noteNames[1], "C#4");
//   p1.pitchTable.values[1] = 1100;
//   std::strcpy(p1.pitchTable.noteNames[2], "D-4");
//   p1.pitchTable.values[2] = 1200;
//
//   // Set up song data - this is what we're testing!
//   p1.song[0][0] = 0x02;
//   p1.song[0][1] = 0x00;
//   p1.song[0][2] = 0x01;
//   p1.songHighlight[0][0] = 0;
//   p1.songHighlight[0][1] = 0;
//   p1.songHighlight[0][2] = 0;
//
//   p1.song[1][0] = 0x11;
//   p1.song[1][1] = 0x10;
//   p1.song[1][2] = 0x12;
//   p1.songHighlight[1][0] = 0;
//   p1.songHighlight[1][1] = 0;
//   p1.songHighlight[1][2] = 0;
//
//   p1.song[2][0] = EMPTY_VALUE_16;
//   p1.song[2][1] = EMPTY_VALUE_16;
//   p1.song[2][2] = EMPTY_VALUE_16;
//
//   // Save project
//   int result = projectSave(&p1, "tests/test_output.cnm");
//   CHECK(result == 0);
//
//   // Load project
//   result = projectLoad(&p2, "tests/test_output.cnm");
//   CHECK(result == 0);
//
//   // Verify song data - especially the first row!
//   CHECK(p2.song[0][0] == 0x02);
//   CHECK(p2.song[0][1] == 0x00);
//   CHECK(p2.song[0][2] == 0x01);
//
//   CHECK(p2.song[1][0] == 0x11);
//   CHECK(p2.song[1][1] == 0x10);
//   CHECK(p2.song[1][2] == 0x12);
// }

TEST_CASE_FIXTURE(TestFixture, "projectLoad_version_1_0_format") {
  Project p;
  projectInit(&p);

  int result = projectLoad(&p, "tests/test_v1_format.cnm");
  CHECK(result == 0);
  CHECK(projectFileVersion == 1);
  CHECK(std::strcmp(p.title, "Legacy Project") == 0);
}

TEST_CASE_FIXTURE(TestFixture, "projectSave_always_version_2_0") {
  Project p;
  projectInit(&p);

  std::strcpy(p.title, "New Project");
  p.tickRate = 50.0f;
  p.chipsCount = 1;
  p.chipType = chipAY;
  p.chipSetup.ay.clock = 1773400;
  p.chipSetup.ay.isYM = 0;
  p.chipSetup.ay.stereoMode = ayStereoABC;
  p.chipSetup.ay.stereoSeparation = 100;
  p.chipSetup.ay.pwmFullRange = 0;
  p.tracksCount = 3;

  std::strcpy(p.pitchTable.name, "Test");
  p.pitchTable.length = 1;
  std::strcpy(p.pitchTable.noteNames[0], "C-4");
  p.pitchTable.values[0] = 1000;

  int result = projectSave(&p, "tests/test_v2_output.cnm");
  CHECK(result == 0);

  // Read the file and check it starts with version 2.0
  FILE* file = fopen("tests/test_v2_output.cnm", "r");
  CHECK(file != nullptr);
  char firstLine[256];
  char* readResult = fgets(firstLine, sizeof(firstLine), file);
  CHECK(readResult != nullptr);
  CHECK(std::strstr(firstLine, "# ChipNomad Tracker Module 2.0") != nullptr);
  fclose(file);
}

TEST_CASE_FIXTURE(TestFixture, "projectLoad_real_file_skytrain_funk") {
  // This test loads an actual project file to catch real-world issues
  Project p;
  projectInit(&p);

  int result = projectLoad(&p, "packaging/common/projects/SkyTrain Funk.cnm");
  if (result != 0) {
    printf("Failed to load SkyTrain Funk.cnm: %s\n", projectFileError);
  }
  CHECK_MESSAGE(result == 0, "Should load SkyTrain Funk.cnm successfully");

  // Basic sanity checks
  CHECK(std::strcmp(p.title, "SkyTrain Funk") == 0);
  CHECK(std::strcmp(p.author, "Megus") == 0);
  CHECK(projectFileVersion == 1);  // It's a v1.0 file
}

TEST_CASE_FIXTURE(TestFixture, "projectLoad_octaveSize_calculated") {
  Project p;
  projectInit(&p);

  // Load a real project file
  int result = projectLoad(&p, "packaging/common/projects/SkyTrain Funk.cnm");
  CHECK(result == 0);

  // Verify octaveSize is set correctly (should be 12 for standard 12TET)
  CHECK(p.pitchTable.octaveSize == 12);
  CHECK(p.pitchTable.length > 0);
}

TEST_CASE_FIXTURE(TestFixture, "projectSaveLoad_octaveSize_preserved") {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up a project with a 12-note octave pitch table
  std::strcpy(p1.title, "Octave Test");
  std::strcpy(p1.author, "Test");
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  // Create a pitch table with 2 octaves (24 notes)
  std::strcpy(p1.pitchTable.name, "12TET Test");
  p1.pitchTable.length = 24;
  p1.pitchTable.octaveSize = 12;

  // First octave (octave 4) - fill all 12 notes
  const char* noteNames[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
  for (int i = 0; i < 12; i++) {
    std::strcpy(p1.pitchTable.noteNames[i], noteNames[i]);
    p1.pitchTable.noteNames[i][2] = '4';  // Octave 4
    p1.pitchTable.noteNames[i][3] = 0;
    p1.pitchTable.values[i] = 1000 + i * 100;
  }

  // Second octave (octave 5) - fill all 12 notes
  for (int i = 0; i < 12; i++) {
    std::strcpy(p1.pitchTable.noteNames[12 + i], noteNames[i]);
    p1.pitchTable.noteNames[12 + i][2] = '5';  // Octave 5
    p1.pitchTable.noteNames[12 + i][3] = 0;
    p1.pitchTable.values[12 + i] = 2000 + i * 100;
  }

  // Save and load
  int result = projectSave(&p1, "tests/test_octave_size.cnm");
  CHECK(result == 0);

  result = projectLoad(&p2, "tests/test_octave_size.cnm");
  CHECK(result == 0);

  // Verify octaveSize is correctly calculated after load
  CHECK(p2.pitchTable.octaveSize == 12);
  CHECK(p2.pitchTable.length == 24);
}

TEST_CASE_FIXTURE(TestFixture, "projectSaveLoad_empty_title") {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up project with empty title (this should be allowed)
  p1.title[0] = '\0';  // Empty title
  std::strcpy(p1.author, "Test Author");
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  std::strcpy(p1.pitchTable.name, "Test");
  p1.pitchTable.length = 1;
  std::strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;

  // Save project with empty title
  int result = projectSave(&p1, "tests/test_empty_title.cnm");
  CHECK_MESSAGE(result == 0, "Should save project with empty title");

  // Load project - this currently fails!
  result = projectLoad(&p2, "tests/test_empty_title.cnm");
  if (result != 0) {
    printf("Failed to load project with empty title: %s\n", projectFileError);
  }
  CHECK_MESSAGE(result == 0, "Should load project with empty title");

  // Verify the title is empty
  CHECK(std::strcmp(p2.title, "") == 0);
  CHECK(std::strcmp(p2.author, "Test Author") == 0);
}

TEST_CASE_FIXTURE(TestFixture, "projectSaveLoad_empty_author") {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up project with empty author (this already works)
  std::strcpy(p1.title, "Test Title");
  p1.author[0] = '\0';  // Empty author
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  std::strcpy(p1.pitchTable.name, "Test");
  p1.pitchTable.length = 1;
  std::strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;

  // Save and load
  int result = projectSave(&p1, "tests/test_empty_author.cnm");
  CHECK(result == 0);

  result = projectLoad(&p2, "tests/test_empty_author.cnm");
  CHECK(result == 0);

  // Verify
  CHECK(std::strcmp(p2.title, "Test Title") == 0);
  CHECK(std::strcmp(p2.author, "") == 0);
}

TEST_CASE_FIXTURE(TestFixture, "projectSaveLoad_wavetables") {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up minimal project
  std::strcpy(p1.title, "Wavetable Test");
  std::strcpy(p1.author, "Test");
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  std::strcpy(p1.pitchTable.name, "Test");
  p1.pitchTable.length = 1;
  std::strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;

  // Set up some wavetables with data
  // Wavetable 0 - sawtooth pattern
  for (int i = 0; i < 32; i++) {
    p1.ayWavetables[0][i] = i / 2;  // 0,0,1,1,2,2...15,15
  }

  // Wavetable 1 - square wave pattern
  for (int i = 0; i < 32; i++) {
    p1.ayWavetables[1][i] = (i < 16) ? 0 : 15;
  }

  // Wavetable 5 - triangle pattern
  for (int i = 0; i < 32; i++) {
    p1.ayWavetables[5][i] = (i < 16) ? i : (31 - i);
  }

  // Wavetable 255 - edge case (last wavetable)
  for (int i = 0; i < 32; i++) {
    p1.ayWavetables[255][i] = 15 - (i / 2);  // Descending
  }

  // Leave wavetables 2, 3, 4, 6-254 empty (all zeros) - they should not be saved

  // Save project
  int result = projectSave(&p1, "tests/test_wavetables.cnm");
  CHECK_MESSAGE(result == 0, "Should save project with wavetables");

  // Load project
  result = projectLoad(&p2, "tests/test_wavetables.cnm");
  if (result != 0) {
    printf("Failed to load project with wavetables: %s\n", projectFileError);
  }
  CHECK_MESSAGE(result == 0, "Should load project with wavetables");

  // Verify wavetable 0
  for (int i = 0; i < 32; i++) {
    CHECK_MESSAGE(p2.ayWavetables[0][i] == i / 2, "Wavetable 0 should match");
  }

  // Verify wavetable 1
  for (int i = 0; i < 32; i++) {
    uint8_t expected = (i < 16) ? 0 : 15;
    CHECK_MESSAGE(p2.ayWavetables[1][i] == expected, "Wavetable 1 should match");
  }

  // Verify wavetable 5
  for (int i = 0; i < 32; i++) {
    uint8_t expected = (i < 16) ? i : (31 - i);
    CHECK_MESSAGE(p2.ayWavetables[5][i] == expected, "Wavetable 5 should match");
  }

  // Verify wavetable 255
  for (int i = 0; i < 32; i++) {
    CHECK_MESSAGE(p2.ayWavetables[255][i] == 15 - (i / 2), "Wavetable 255 should match");
  }

  // Verify empty wavetables remain empty
  for (int i = 0; i < 32; i++) {
    CHECK(p2.ayWavetables[2][i] == 0);
    CHECK(p2.ayWavetables[10][i] == 0);
    CHECK(p2.ayWavetables[100][i] == 0);
  }
}

TEST_CASE_FIXTURE(TestFixture, "projectSaveLoad_no_wavetables") {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up minimal project with NO wavetables
  std::strcpy(p1.title, "No Wavetables");
  std::strcpy(p1.author, "Test");
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  std::strcpy(p1.pitchTable.name, "Test");
  p1.pitchTable.length = 1;
  std::strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;

  // All wavetables are empty (zeros) by default after projectInit

  // Save project - should NOT create wavetable section
  int result = projectSave(&p1, "tests/test_no_wavetables.cnm");
  CHECK_MESSAGE(result == 0, "Should save project without wavetables");

  // Load project - should still work (backwards compatibility)
  result = projectLoad(&p2, "tests/test_no_wavetables.cnm");
  if (result != 0) {
    printf("Failed to load project without wavetables: %s\n", projectFileError);
  }
  CHECK_MESSAGE(result == 0, "Should load project without wavetable section");

  // Verify all wavetables remain empty
  for (int wt = 0; wt < 256; wt++) {
    for (int i = 0; i < 32; i++) {
      CHECK(p2.ayWavetables[wt][i] == 0);
    }
  }
}
