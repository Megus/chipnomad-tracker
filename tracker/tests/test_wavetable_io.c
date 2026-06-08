#include "../external/unity/unity.h"
#include "../src/wavetable_io.h"
#include <string.h>
#include <unistd.h>

static uint8_t testWavetables[256][32];

void setUp(void) {
  // Clear all wavetables
  memset(testWavetables, 0, sizeof(testWavetables));
}

void tearDown(void) {
  // Clean up test files
  unlink("test_wavetable_save.txt");
  unlink("test_wavetable_load.txt");
}

// Test saving a single wavetable
void test_wavetableSave_single(void) {
  // Create a test wavetable
  for (int i = 0; i < 32; i++) {
    testWavetables[0][i] = i % 16;  // 0123456789ABCDEF0123456789ABCDEF
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 0, 1);
  TEST_ASSERT_EQUAL(1, result);

  // Verify file contents
  FILE* file = fopen("test_wavetable_save.txt", "r");
  TEST_ASSERT_NOT_NULL(file);

  char line[256];
  TEST_ASSERT_NOT_NULL(fgets(line, sizeof(line), file));
  TEST_ASSERT_EQUAL_STRING("0123456789ABCDEF0123456789ABCDEF\n", line);

  fclose(file);
}

// Test saving multiple wavetables
void test_wavetableSave_multiple(void) {
  // Create test wavetables
  for (int w = 0; w < 3; w++) {
    for (int i = 0; i < 32; i++) {
      testWavetables[w][i] = (w * 2 + i) % 16;
    }
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 0, 3);
  TEST_ASSERT_EQUAL(1, result);

  // Verify file has 3 lines
  FILE* file = fopen("test_wavetable_save.txt", "r");
  TEST_ASSERT_NOT_NULL(file);

  char line[256];
  int lineCount = 0;
  while (fgets(line, sizeof(line), file)) {
    lineCount++;
  }
  TEST_ASSERT_EQUAL(3, lineCount);

  fclose(file);
}

// Test saving from a non-zero starting index
void test_wavetableSave_from_middle(void) {
  // Create test wavetable at index 10
  for (int i = 0; i < 32; i++) {
    testWavetables[10][i] = 0xF;  // All F's
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 10, 1);
  TEST_ASSERT_EQUAL(1, result);

  // Verify file contents
  FILE* file = fopen("test_wavetable_save.txt", "r");
  TEST_ASSERT_NOT_NULL(file);

  char line[256];
  TEST_ASSERT_NOT_NULL(fgets(line, sizeof(line), file));
  TEST_ASSERT_EQUAL_STRING("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n", line);

  fclose(file);
}

// Test saving at array boundary (don't save past 255)
void test_wavetableSave_boundary(void) {
  // Try to save 10 wavetables starting at index 250
  // Should only save 6 (250-255)
  for (int i = 250; i < 256; i++) {
    for (int j = 0; j < 32; j++) {
      testWavetables[i][j] = 0xA;
    }
  }

  int result = wavetableSave("test_wavetable_save.txt", testWavetables, 250, 10);
  TEST_ASSERT_EQUAL(1, result);

  // Verify file has exactly 6 lines
  FILE* file = fopen("test_wavetable_save.txt", "r");
  TEST_ASSERT_NOT_NULL(file);

  char line[256];
  int lineCount = 0;
  while (fgets(line, sizeof(line), file)) {
    lineCount++;
  }
  TEST_ASSERT_EQUAL(6, lineCount);

  fclose(file);
}

// Test loading a single wavetable
void test_wavetableLoad_single(void) {
  // Create a test file
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789ABCDEF0123456789ABCDEF\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(1, result);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(i % 16, testWavetables[0][i]);
  }
}

// Test loading multiple wavetables
void test_wavetableLoad_multiple(void) {
  // Create a test file with 3 wavetables
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "00000000000000000000000000000000\n");
  fprintf(file, "11111111111111111111111111111111\n");
  fprintf(file, "22222222222222222222222222222222\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(3, result);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(0, testWavetables[0][i]);
    TEST_ASSERT_EQUAL(1, testWavetables[1][i]);
    TEST_ASSERT_EQUAL(2, testWavetables[2][i]);
  }
}

// Test loading to a non-zero starting index
void test_wavetableLoad_to_middle(void) {
  // Create a test file
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 10);
  TEST_ASSERT_EQUAL(1, result);

  // Verify loaded data at index 10
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(0xF, testWavetables[10][i]);
  }

  // Verify other indices are still 0
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(0, testWavetables[0][i]);
    TEST_ASSERT_EQUAL(0, testWavetables[9][i]);
  }
}

// Test loading with empty lines (should skip)
void test_wavetableLoad_skip_empty_lines(void) {
  // Create a test file with empty lines
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "00000000000000000000000000000000\n");
  fprintf(file, "\n");
  fprintf(file, "11111111111111111111111111111111\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(2, result);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(0, testWavetables[0][i]);
    TEST_ASSERT_EQUAL(1, testWavetables[1][i]);
  }
}

// Test loading with comments (should skip)
void test_wavetableLoad_skip_comments(void) {
  // Create a test file with comment lines
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "# This is a comment\n");
  fprintf(file, "00000000000000000000000000000000\n");
  fprintf(file, "# Another comment\n");
  fprintf(file, "11111111111111111111111111111111\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(2, result);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(0, testWavetables[0][i]);
    TEST_ASSERT_EQUAL(1, testWavetables[1][i]);
  }
}

// Test loading invalid format (wrong length)
void test_wavetableLoad_invalid_length(void) {
  // Create a test file with wrong line length
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789ABCDEF\n");  // Only 16 digits, should be 32
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(0, result);  // Should fail
}

// Test loading invalid format (invalid hex)
void test_wavetableLoad_invalid_hex(void) {
  // Create a test file with invalid hex character
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789ABCDEFG123456789ABCDEF\n");  // 'G' is invalid
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(0, result);  // Should fail
}

// Test loading lowercase hex
void test_wavetableLoad_lowercase_hex(void) {
  // Create a test file with lowercase hex
  FILE* file = fopen("test_wavetable_load.txt", "w");
  fprintf(file, "0123456789abcdef0123456789abcdef\n");
  fclose(file);

  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(1, result);

  // Verify loaded data
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT_EQUAL(i % 16, testWavetables[0][i]);
  }
}

// Test loading at boundary (don't load past 255)
void test_wavetableLoad_boundary(void) {
  // Create a test file with 10 wavetables
  FILE* file = fopen("test_wavetable_load.txt", "w");
  for (int i = 0; i < 10; i++) {
    fprintf(file, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
  }
  fclose(file);

  // Try to load starting at index 250
  // Should only load 6 (250-255)
  int result = wavetableLoad("test_wavetable_load.txt", testWavetables, 250);
  TEST_ASSERT_EQUAL(6, result);

  // Verify loaded data
  for (int i = 250; i < 256; i++) {
    for (int j = 0; j < 32; j++) {
      TEST_ASSERT_EQUAL(0xA, testWavetables[i][j]);
    }
  }
}

// Test save and load round-trip
void test_wavetable_roundtrip(void) {
  // Create test wavetables
  for (int w = 0; w < 5; w++) {
    for (int i = 0; i < 32; i++) {
      testWavetables[w][i] = (w + i) % 16;
    }
  }

  // Save
  int saveResult = wavetableSave("test_wavetable_save.txt", testWavetables, 0, 5);
  TEST_ASSERT_EQUAL(1, saveResult);

  // Clear wavetables
  memset(testWavetables, 0, sizeof(testWavetables));

  // Load
  int loadResult = wavetableLoad("test_wavetable_save.txt", testWavetables, 0);
  TEST_ASSERT_EQUAL(5, loadResult);

  // Verify data matches original
  for (int w = 0; w < 5; w++) {
    for (int i = 0; i < 32; i++) {
      TEST_ASSERT_EQUAL((w + i) % 16, testWavetables[w][i]);
    }
  }
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_wavetableSave_single);
  RUN_TEST(test_wavetableSave_multiple);
  RUN_TEST(test_wavetableSave_from_middle);
  RUN_TEST(test_wavetableSave_boundary);

  RUN_TEST(test_wavetableLoad_single);
  RUN_TEST(test_wavetableLoad_multiple);
  RUN_TEST(test_wavetableLoad_to_middle);
  RUN_TEST(test_wavetableLoad_skip_empty_lines);
  RUN_TEST(test_wavetableLoad_skip_comments);
  RUN_TEST(test_wavetableLoad_invalid_length);
  RUN_TEST(test_wavetableLoad_invalid_hex);
  RUN_TEST(test_wavetableLoad_lowercase_hex);
  RUN_TEST(test_wavetableLoad_boundary);

  RUN_TEST(test_wavetable_roundtrip);

  return UNITY_END();
}
