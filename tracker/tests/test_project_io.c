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
  p.tracksCount = 3;

  strcpy(p.pitchTable.name, "Test");
  p.pitchTable.length = 1;
  strcpy(p.pitchTable.noteNames[0], "C-4");
  p.pitchTable.values[0] = 1000;

  int result = projectSave(&p, "tests/test_v2_output.cnm");
  TEST_ASSERT_EQUAL(0, result);

  // Read the file and check it starts with version 2.0
  int fileId = fileOpen("tests/test_v2_output.cnm", 0);
  TEST_ASSERT_NOT_EQUAL(-1, fileId);
  char* firstLine = fileReadString(fileId);
  TEST_ASSERT_NOT_NULL(firstLine);
  TEST_ASSERT_TRUE(strstr(firstLine, "# ChipNomad Tracker Module 2.0") != NULL);
  fileClose(fileId);
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

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_projectSaveLoad_song_data_preserved);
  RUN_TEST(test_projectLoad_version_1_0_format);
  RUN_TEST(test_projectSave_always_version_2_0);
  RUN_TEST(test_projectLoad_real_file_skytrain_funk);
  return UNITY_END();
}
