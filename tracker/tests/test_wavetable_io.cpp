#include "doctest.h"

extern "C" {
#include "../src/wavetable_io.h"
}

#include <cstring>
#include <unistd.h>

TEST_SUITE("wavetable_io") {

struct TestFixture {
  uint8_t testWavetables[256][32];

  TestFixture() {
    // Clear all wavetables
    std::memset(testWavetables, 0, sizeof(testWavetables));
  }

  ~TestFixture() {
    // Clean up test files
    unlink("test_wavetable_save.txt");
    unlink("test_wavetable_load.txt");
  }
};

// Test saving a single wavetable
TEST_CASE_FIXTURE(TestFixture, "wavetableSave_single") {
  // Create a test wavetable
  for (int i = 0; i < 32; i++) {
    testWavetables[0][i] = i % 16;  // 0123456789ABCDEF0123456789ABCDEF
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 0, 1);
  CHECK(result == 1);

  // Verify file contents
  FILE* file = fopen("test_wavetable_save.txt", "r");
  CHECK(file != nullptr);

  char line[256];
  CHECK(fgets(line, sizeof(line), file) != nullptr);
  CHECK(std::strcmp(line, "0123456789ABCDEF0123456789ABCDEF\n") == 0);

  fclose(file);
}

// Test saving multiple wavetables
TEST_CASE_FIXTURE(TestFixture, "wavetableSave_multiple") {
  // Create test wavetables
  for (int w = 0; w < 3; w++) {
    for (int i = 0; i < 32; i++) {
      testWavetables[w][i] = (w * 2 + i) % 16;
    }
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 0, 3);
  CHECK(result == 1);

  // Verify file has 3 lines
  FILE* file = fopen("test_wavetable_save.txt", "r");
  CHECK(file != nullptr);

  char line[256];
  int lineCount = 0;
  while (fgets(line, sizeof(line), file)) {
    lineCount++;
  }
  CHECK(lineCount == 3);

  fclose(file);
}

// Test saving from a non-zero starting index
TEST_CASE_FIXTURE(TestFixture, "wavetableSave_from_middle") {
  // Create test wavetable at index 10
  for (int i = 0; i < 32; i++) {
    testWavetables[10][i] = 0xF;  // All F's
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 10, 1);
  CHECK(result == 1);

  // Verify file contents
  FILE* file = fopen("test_wavetable_save.txt", "r");
  CHECK(file != nullptr);

  char line[256];
  CHECK(fgets(line, sizeof(line), file) != nullptr);
  CHECK(std::strcmp(line, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n") == 0);

  fclose(file);
}

// Test saving at array boundary (don't save past 255)
TEST_CASE_FIXTURE(TestFixture, "wavetableSave_boundary") {
  // Try to save 10 wavetables starting at index 250
  // Should only save 6 (250-255)
  for (int i = 250; i < 256; i++) {
    for (int j = 0; j < 32; j++) {
      testWavetables[i][j] = 0xA;
    }
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 250, 10);
  CHECK(result == 1);

  // Verify file has exactly 6 lines
  FILE* file = fopen("test_wavetable_save.txt", "r");
  CHECK(file != nullptr);

  char line[256];
  int lineCount = 0;
  while (fgets(line, sizeof(line), file)) {
    lineCount++;
  }
  CHECK(lineCount == 6);

  fclose(file);
}

// Test loading a single wavetable
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_single") {
  // Create a test file
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789ABCDEF0123456789ABCDEF\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 1);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[0][i] == i % 16);
  }
}

// Test loading multiple wavetables
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_multiple") {
  // Create a test file with 3 wavetables
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "00000000000000000000000000000000\n");
  fprintf(file, "11111111111111111111111111111111\n");
  fprintf(file, "22222222222222222222222222222222\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 3);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[0][i] == 0);
    CHECK(testWavetables[1][i] == 1);
    CHECK(testWavetables[2][i] == 2);
  }
}

// Test loading to a non-zero starting index
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_to_middle") {
  // Create a test file
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 10);
  CHECK(result == 1);

  // Verify loaded data at index 10
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[10][i] == 0xF);
  }

  // Verify other indices are still 0
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[0][i] == 0);
    CHECK(testWavetables[9][i] == 0);
  }
}

// Test loading with empty lines (should skip)
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_skip_empty_lines") {
  // Create a test file with empty lines
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "00000000000000000000000000000000\n");
  fprintf(file, "\n");
  fprintf(file, "11111111111111111111111111111111\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 2);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[0][i] == 0);
    CHECK(testWavetables[1][i] == 1);
  }
}

// Test loading with comments (should skip)
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_skip_comments") {
  // Create a test file with comment lines
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "# This is a comment\n");
  fprintf(file, "00000000000000000000000000000000\n");
  fprintf(file, "# Another comment\n");
  fprintf(file, "11111111111111111111111111111111\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 2);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[0][i] == 0);
    CHECK(testWavetables[1][i] == 1);
  }
}

// Test loading invalid format (wrong length)
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_invalid_length") {
  // Create a test file with wrong line length
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789ABCDEF\n");  // Only 16 digits, should be 32
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 0);  // Should fail
}

// Test loading invalid format (invalid hex)
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_invalid_hex") {
  // Create a test file with invalid hex character
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789ABCDEFG123456789ABCDEF\n");  // 'G' is invalid
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 0);  // Should fail
}

// Test loading lowercase hex
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_lowercase_hex") {
  // Create a test file with lowercase hex
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789abcdef0123456789abcdef\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  CHECK(result == 1);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    CHECK(testWavetables[0][i] == i % 16);
  }
}

// Test loading at boundary (don't load past 255)
TEST_CASE_FIXTURE(TestFixture, "wavetableLoad_boundary") {
  // Create a test file with 10 wavetables
  FILE* file = fopen("test_wavetable_load.txt", "w");
  for (int i = 0; i < 10; i++) {
    fprintf(file, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
  }
  fclose(file);

  // Try to load starting at index 250
  // Should only load 6 (250-255)
  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 250);
  CHECK(result == 6);

  // Verify loaded data
  for (int i = 250; i < 256; i++) {
    for (int j = 0; j < 32; j++) {
      CHECK(testWavetables[i][j] == 0xA);
    }
  }
}

// Test save and load round-trip
TEST_CASE_FIXTURE(TestFixture, "wavetable_roundtrip") {
  // Create test wavetables
  for (int w = 0; w < 5; w++) {
    for (int i = 0; i < 32; i++) {
      testWavetables[w][i] = (w + i) % 16;
    }
  }

  // Save
  int saveResult = wavetableSave("test_wavetable_save.txt", testWavetables, 0, 5);
  CHECK(saveResult == 1);

  // Clear wavetables
  std::memset(testWavetables, 0, sizeof(testWavetables));

  // Load
  int loadResult = wavetableLoad("test_wavetable_save.txt", testWavetables, 0);
  CHECK(loadResult == 5);

  // Verify data matches original
  for (int w = 0; w < 5; w++) {
    for (int i = 0; i < 32; i++) {
      CHECK(testWavetables[w][i] == (w + i) % 16);
    }
  }
}

} // TEST_SUITE("wavetable_io")
