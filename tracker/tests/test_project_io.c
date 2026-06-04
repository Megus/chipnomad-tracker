#include "unity.h"
#include "project.h"
#include "project_io_common.h"
#include <string.h>

void setUp(void) {
  resetPeekConsume();
}

void tearDown(void) {
}

///////////////////////////////////////////////////////////////////////////////
// Tests

void test_projectSaveLoad_song_data_preserved(void) {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up project with some song data
  strcpy(p1.title, "Test Song");
  strcpy(p1.author, "Test Author");
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  // Set up pitch table
  strcpy(p1.pitchTable.name, "Equal Temperament");
  p1.pitchTable.length = 3;
  strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;
  strcpy(p1.pitchTable.noteNames[1], "C#4");
  p1.pitchTable.values[1] = 1100;
  strcpy(p1.pitchTable.noteNames[2], "D-4");
  p1.pitchTable.values[2] = 1200;

  // Set up song data - this is what we're testing!
  p1.song[0][0] = 0x02;
  p1.song[0][1] = 0x00;
  p1.song[0][2] = 0x01;
  p1.songHighlight[0][0] = 0;
  p1.songHighlight[0][1] = 0;
  p1.songHighlight[0][2] = 0;

  p1.song[1][0] = 0x11;
  p1.song[1][1] = 0x10;
  p1.song[1][2] = 0x12;
  p1.songHighlight[1][0] = 0;
  p1.songHighlight[1][1] = 0;
  p1.songHighlight[1][2] = 0;

  p1.song[2][0] = EMPTY_VALUE_16;
  p1.song[2][1] = EMPTY_VALUE_16;
  p1.song[2][2] = EMPTY_VALUE_16;

  // Save project
  int result = projectSave(&p1, "tests/test_output.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Load project
  result = projectLoad(&p2, "tests/test_output.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Verify song data - especially the first row!
  TEST_ASSERT_EQUAL_HEX16(0x02, p2.song[0][0]);
  TEST_ASSERT_EQUAL_HEX16(0x00, p2.song[0][1]);
  TEST_ASSERT_EQUAL_HEX16(0x01, p2.song[0][2]);

  TEST_ASSERT_EQUAL_HEX16(0x11, p2.song[1][0]);
  TEST_ASSERT_EQUAL_HEX16(0x10, p2.song[1][1]);
  TEST_ASSERT_EQUAL_HEX16(0x12, p2.song[1][2]);
}

void test_projectLoad_version_1_0_format(void) {
  Project p;
  projectInit(&p);

  int result = projectLoad(&p, "tests/test_v1_format.cnm");
  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(1, projectFileVersion);
  TEST_ASSERT_EQUAL_STRING("Legacy Project", p.title);
}

void test_projectSave_always_version_2_0(void) {
  Project p;
  projectInit(&p);

  strcpy(p.title, "New Project");
  p.tickRate = 50.0f;
  p.chipsCount = 1;
  p.chipType = chipAY;
  p.chipSetup.ay.clock = 1773400;
  p.chipSetup.ay.isYM = 0;
  p.chipSetup.ay.stereoMode = ayStereoABC;
  p.chipSetup.ay.stereoSeparation = 100;
  p.chipSetup.ay.pwmFullRange = 0;
  p.tracksCount = 3;

  strcpy(p.pitchTable.name, "Test");
  p.pitchTable.length = 1;
  strcpy(p.pitchTable.noteNames[0], "C-4");
  p.pitchTable.values[0] = 1000;

  int result = projectSave(&p, "tests/test_v2_output.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Read the file and check it starts with version 2.0
  FILE* file = fopen("tests/test_v2_output.cnm", "r");
  TEST_ASSERT_NOT_NULL(file);
  char firstLine[256];
  char* readResult = fgets(firstLine, sizeof(firstLine), file);
  TEST_ASSERT_NOT_NULL(readResult);
  TEST_ASSERT_TRUE(strstr(firstLine, "# ChipNomad Tracker Module 2.0") != NULL);
  fclose(file);
}

void test_projectLoad_real_file_skytrain_funk(void) {
  // This test loads an actual project file to catch real-world issues
  Project p;
  projectInit(&p);

  int result = projectLoad(&p, "packaging/common/projects/SkyTrain Funk.cnm");
  if (result != 0) {
    printf("Failed to load SkyTrain Funk.cnm: %s\n", projectFileError);
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, result, "Should load SkyTrain Funk.cnm successfully");

  // Basic sanity checks
  TEST_ASSERT_EQUAL_STRING("SkyTrain Funk", p.title);
  TEST_ASSERT_EQUAL_STRING("Megus", p.author);
  TEST_ASSERT_EQUAL(1, projectFileVersion);  // It's a v1.0 file
}

void test_projectLoad_octaveSize_calculated(void) {
  Project p;
  projectInit(&p);

  // Load a real project file
  int result = projectLoad(&p, "packaging/common/projects/SkyTrain Funk.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Verify octaveSize is set correctly (should be 12 for standard 12TET)
  TEST_ASSERT_EQUAL(12, p.pitchTable.octaveSize);
  TEST_ASSERT_GREATER_THAN(0, p.pitchTable.length);
}

void test_projectSaveLoad_octaveSize_preserved(void) {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up a project with a 12-note octave pitch table
  strcpy(p1.title, "Octave Test");
  strcpy(p1.author, "Test");
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
  strcpy(p1.pitchTable.name, "12TET Test");
  p1.pitchTable.length = 24;
  p1.pitchTable.octaveSize = 12;

  // First octave (octave 4) - fill all 12 notes
  const char* noteNames[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
  for (int i = 0; i < 12; i++) {
    strcpy(p1.pitchTable.noteNames[i], noteNames[i]);
    p1.pitchTable.noteNames[i][2] = '4';  // Octave 4
    p1.pitchTable.noteNames[i][3] = 0;
    p1.pitchTable.values[i] = 1000 + i * 100;
  }

  // Second octave (octave 5) - fill all 12 notes
  for (int i = 0; i < 12; i++) {
    strcpy(p1.pitchTable.noteNames[12 + i], noteNames[i]);
    p1.pitchTable.noteNames[12 + i][2] = '5';  // Octave 5
    p1.pitchTable.noteNames[12 + i][3] = 0;
    p1.pitchTable.values[12 + i] = 2000 + i * 100;
  }

  // Save and load
  int result = projectSave(&p1, "tests/test_octave_size.cnm");
  TEST_ASSERT_EQUAL(0, result);

  result = projectLoad(&p2, "tests/test_octave_size.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Verify octaveSize is correctly calculated after load
  TEST_ASSERT_EQUAL(12, p2.pitchTable.octaveSize);
  TEST_ASSERT_EQUAL(24, p2.pitchTable.length);
}

void test_projectSaveLoad_empty_title(void) {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up project with empty title (this should be allowed)
  p1.title[0] = '\0';  // Empty title
  strcpy(p1.author, "Test Author");
  p1.tickRate = 50.0f;
  p1.chipsCount = 1;
  p1.chipType = chipAY;
  p1.chipSetup.ay.clock = 1773400;
  p1.chipSetup.ay.isYM = 0;
  p1.chipSetup.ay.stereoMode = ayStereoABC;
  p1.chipSetup.ay.stereoSeparation = 100;
  p1.chipSetup.ay.pwmFullRange = 0;
  p1.tracksCount = 3;

  strcpy(p1.pitchTable.name, "Test");
  p1.pitchTable.length = 1;
  strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;

  // Save project with empty title
  int result = projectSave(&p1, "tests/test_empty_title.cnm");
  TEST_ASSERT_EQUAL_MESSAGE(0, result, "Should save project with empty title");

  // Load project - this currently fails!
  result = projectLoad(&p2, "tests/test_empty_title.cnm");
  if (result != 0) {
    printf("Failed to load project with empty title: %s\n", projectFileError);
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, result, "Should load project with empty title");

  // Verify the title is empty
  TEST_ASSERT_EQUAL_STRING("", p2.title);
  TEST_ASSERT_EQUAL_STRING("Test Author", p2.author);
}

void test_projectSaveLoad_empty_author(void) {
  Project p1, p2;
  projectInit(&p1);
  projectInit(&p2);

  // Set up project with empty author (this already works)
  strcpy(p1.title, "Test Title");
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

  strcpy(p1.pitchTable.name, "Test");
  p1.pitchTable.length = 1;
  strcpy(p1.pitchTable.noteNames[0], "C-4");
  p1.pitchTable.values[0] = 1000;

  // Save and load
  int result = projectSave(&p1, "tests/test_empty_author.cnm");
  TEST_ASSERT_EQUAL(0, result);

  result = projectLoad(&p2, "tests/test_empty_author.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Verify
  TEST_ASSERT_EQUAL_STRING("Test Title", p2.title);
  TEST_ASSERT_EQUAL_STRING("", p2.author);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_projectSaveLoad_song_data_preserved);
  RUN_TEST(test_projectLoad_version_1_0_format);
  RUN_TEST(test_projectSave_always_version_2_0);
  RUN_TEST(test_projectLoad_real_file_skytrain_funk);
  RUN_TEST(test_projectLoad_octaveSize_calculated);
  RUN_TEST(test_projectSaveLoad_octaveSize_preserved);
  RUN_TEST(test_projectSaveLoad_empty_title);
  RUN_TEST(test_projectSaveLoad_empty_author);
  return UNITY_END();
}
