#include "unity.h"
#include "../src/import/import_wav.h"
#include "../chipnomad_lib/project_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// Helper: Create a simple 8-bit mono WAV file for testing
static void createTestWav8Bit(const char* path, uint16_t sampleRate,
                              const uint8_t* samples, uint16_t numSamples) {
  FILE* f = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(f);

  // RIFF header
  fwrite("RIFF", 4, 1, f);
  uint32_t fileSize = 36 + numSamples;
  fwrite(&fileSize, 4, 1, f);
  fwrite("WAVE", 4, 1, f);

  // fmt chunk
  fwrite("fmt ", 4, 1, f);
  uint32_t fmtSize = 16;
  fwrite(&fmtSize, 4, 1, f);
  uint16_t audioFormat = 1; // PCM
  fwrite(&audioFormat, 2, 1, f);
  uint16_t numChannels = 1; // Mono
  fwrite(&numChannels, 2, 1, f);
  uint32_t sr = sampleRate;
  fwrite(&sr, 4, 1, f);
  uint32_t byteRate = sampleRate;
  fwrite(&byteRate, 4, 1, f);
  uint16_t blockAlign = 1;
  fwrite(&blockAlign, 2, 1, f);
  uint16_t bitsPerSample = 8;
  fwrite(&bitsPerSample, 2, 1, f);

  // data chunk
  fwrite("data", 4, 1, f);
  uint32_t dataSize = numSamples;
  fwrite(&dataSize, 4, 1, f);
  fwrite(samples, 1, numSamples, f);

  fclose(f);
}

// Helper: Create a 16-bit mono WAV file
static void createTestWav16Bit(const char* path, uint16_t sampleRate,
                               const int16_t* samples, uint16_t numSamples) {
  FILE* f = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(f);

  // RIFF header
  fwrite("RIFF", 4, 1, f);
  uint32_t fileSize = 36 + numSamples * 2;
  fwrite(&fileSize, 4, 1, f);
  fwrite("WAVE", 4, 1, f);

  // fmt chunk
  fwrite("fmt ", 4, 1, f);
  uint32_t fmtSize = 16;
  fwrite(&fmtSize, 4, 1, f);
  uint16_t audioFormat = 1; // PCM
  fwrite(&audioFormat, 2, 1, f);
  uint16_t numChannels = 1; // Mono
  fwrite(&numChannels, 2, 1, f);
  uint32_t sr = sampleRate;
  fwrite(&sr, 4, 1, f);
  uint32_t byteRate = sampleRate * 2;
  fwrite(&byteRate, 4, 1, f);
  uint16_t blockAlign = 2;
  fwrite(&blockAlign, 2, 1, f);
  uint16_t bitsPerSample = 16;
  fwrite(&bitsPerSample, 2, 1, f);

  // data chunk
  fwrite("data", 4, 1, f);
  uint32_t dataSize = numSamples * 2;
  fwrite(&dataSize, 4, 1, f);
  fwrite(samples, 2, numSamples, f);

  fclose(f);
}

// Test loading 8-bit WAV
void test_load_wav_8bit_mono(void) {
  const char* testFile = "test_8bit.wav";
  uint8_t testSamples[] = {0, 64, 128, 192, 255};
  createTestWav8Bit(testFile, 8000, testSamples, 5);

  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile(testFile, PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  TEST_ASSERT_EQUAL(WAV_SUCCESS, result);
  TEST_ASSERT_NOT_NULL(data);
  TEST_ASSERT_EQUAL(5, length);
  TEST_ASSERT_EQUAL(8000, sampleRate);

  // Verify samples
  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_EQUAL_UINT8(testSamples[i], data[i]);
  }

  free(data);
  remove(testFile);
}

// Test loading 16-bit WAV and conversion
void test_load_wav_16bit_mono(void) {
  const char* testFile = "test_16bit.wav";
  int16_t testSamples[] = {-32768, -16384, 0, 16384, 32767};
  createTestWav16Bit(testFile, 16000, testSamples, 5);

  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile(testFile, PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  TEST_ASSERT_EQUAL(WAV_SUCCESS, result);
  TEST_ASSERT_NOT_NULL(data);
  TEST_ASSERT_EQUAL(5, length);
  TEST_ASSERT_EQUAL(16000, sampleRate);

  // Verify conversion (16-bit signed to 8-bit unsigned)
  // -32768 -> 0, -16384 -> 64, 0 -> 128, 16384 -> 192, 32767 -> 255
  TEST_ASSERT_EQUAL_UINT8(0, data[0]);
  TEST_ASSERT_EQUAL_UINT8(64, data[1]);
  TEST_ASSERT_EQUAL_UINT8(128, data[2]);
  TEST_ASSERT_EQUAL_UINT8(192, data[3]);
  TEST_ASSERT_EQUAL_UINT8(255, data[4]);

  free(data);
  remove(testFile);
}

// Test file not found
void test_load_wav_file_not_found(void) {
  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile("nonexistent.wav", PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  TEST_ASSERT_NULL(data);
  TEST_ASSERT_EQUAL(WAV_ERROR_FILE_NOT_FOUND, result);
}

// Test invalid WAV file
void test_load_wav_invalid_file(void) {
  const char* testFile = "test_invalid.wav";
  FILE* f = fopen(testFile, "wb");
  fwrite("NOT A WAV FILE", 14, 1, f);
  fclose(f);

  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile(testFile, PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  TEST_ASSERT_NULL(data);
  TEST_ASSERT_EQUAL(WAV_ERROR_NOT_WAV, result);

  remove(testFile);
}

// Test size limit
void test_load_wav_size_limit(void) {
  const char* testFile = "test_large.wav";
  uint8_t testSamples[1000];
  for (int i = 0; i < 1000; i++) {
    testSamples[i] = (uint8_t)(i & 0xFF);
  }
  createTestWav8Bit(testFile, 8000, testSamples, 1000);

  uint16_t length, sampleRate;
  WavLoadResult result;
  // Limit to 100 samples
  uint8_t* data = loadWavFile(testFile, 100, &length, &sampleRate, &result);

  TEST_ASSERT_EQUAL(WAV_SUCCESS, result);
  TEST_ASSERT_NOT_NULL(data);
  TEST_ASSERT_EQUAL(100, length); // Should be limited
  TEST_ASSERT_EQUAL(8000, sampleRate);

  free(data);
  remove(testFile);
}

// Test error messages
void test_wav_error_messages(void) {
  TEST_ASSERT_EQUAL_STRING("Success",
                          getWavLoadErrorMessage(WAV_SUCCESS));
  TEST_ASSERT_EQUAL_STRING("File not found or cannot be opened",
                          getWavLoadErrorMessage(WAV_ERROR_FILE_NOT_FOUND));
  TEST_ASSERT_EQUAL_STRING("Not a valid WAV file",
                          getWavLoadErrorMessage(WAV_ERROR_NOT_WAV));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_load_wav_8bit_mono);
  RUN_TEST(test_load_wav_16bit_mono);
  RUN_TEST(test_load_wav_file_not_found);
  RUN_TEST(test_load_wav_invalid_file);
  RUN_TEST(test_load_wav_size_limit);
  RUN_TEST(test_wav_error_messages);

  return UNITY_END();
}
