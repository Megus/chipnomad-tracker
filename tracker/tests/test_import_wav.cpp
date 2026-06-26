#include "doctest.h"

extern "C" {
#include "import_wav.h"
#include "project_constants.h"
}

#include <cstdio>
#include <cstdlib>
#include <cstring>

TEST_SUITE("import_wav") {

// Helper: Create a simple 8-bit mono WAV file for testing
static void createTestWav8Bit(const char* path, uint16_t sampleRate,
                              const uint8_t* samples, uint16_t numSamples) {
  FILE* f = std::fopen(path, "wb");
  REQUIRE(f != nullptr);

  // RIFF header
  std::fwrite("RIFF", 4, 1, f);
  uint32_t fileSize = 36 + numSamples;
  std::fwrite(&fileSize, 4, 1, f);
  std::fwrite("WAVE", 4, 1, f);

  // fmt chunk
  std::fwrite("fmt ", 4, 1, f);
  uint32_t fmtSize = 16;
  std::fwrite(&fmtSize, 4, 1, f);
  uint16_t audioFormat = 1; // PCM
  std::fwrite(&audioFormat, 2, 1, f);
  uint16_t numChannels = 1; // Mono
  std::fwrite(&numChannels, 2, 1, f);
  uint32_t sr = sampleRate;
  std::fwrite(&sr, 4, 1, f);
  uint32_t byteRate = sampleRate;
  std::fwrite(&byteRate, 4, 1, f);
  uint16_t blockAlign = 1;
  std::fwrite(&blockAlign, 2, 1, f);
  uint16_t bitsPerSample = 8;
  std::fwrite(&bitsPerSample, 2, 1, f);

  // data chunk
  std::fwrite("data", 4, 1, f);
  uint32_t dataSize = numSamples;
  std::fwrite(&dataSize, 4, 1, f);
  std::fwrite(samples, 1, numSamples, f);

  std::fclose(f);
}

// Helper: Create a 16-bit mono WAV file
static void createTestWav16Bit(const char* path, uint16_t sampleRate,
                               const int16_t* samples, uint16_t numSamples) {
  FILE* f = std::fopen(path, "wb");
  REQUIRE(f != nullptr);

  // RIFF header
  std::fwrite("RIFF", 4, 1, f);
  uint32_t fileSize = 36 + numSamples * 2;
  std::fwrite(&fileSize, 4, 1, f);
  std::fwrite("WAVE", 4, 1, f);

  // fmt chunk
  std::fwrite("fmt ", 4, 1, f);
  uint32_t fmtSize = 16;
  std::fwrite(&fmtSize, 4, 1, f);
  uint16_t audioFormat = 1; // PCM
  std::fwrite(&audioFormat, 2, 1, f);
  uint16_t numChannels = 1; // Mono
  std::fwrite(&numChannels, 2, 1, f);
  uint32_t sr = sampleRate;
  std::fwrite(&sr, 4, 1, f);
  uint32_t byteRate = sampleRate * 2;
  std::fwrite(&byteRate, 4, 1, f);
  uint16_t blockAlign = 2;
  std::fwrite(&blockAlign, 2, 1, f);
  uint16_t bitsPerSample = 16;
  std::fwrite(&bitsPerSample, 2, 1, f);

  // data chunk
  std::fwrite("data", 4, 1, f);
  uint32_t dataSize = numSamples * 2;
  std::fwrite(&dataSize, 4, 1, f);
  std::fwrite(samples, 2, numSamples, f);

  std::fclose(f);
}

TEST_CASE("load wav 8bit mono") {
  const char* testFile = "test_8bit.wav";
  uint8_t testSamples[] = {0, 64, 128, 192, 255};
  createTestWav8Bit(testFile, 8000, testSamples, 5);

  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile(testFile, PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  CHECK(result == WAV_SUCCESS);
  CHECK(data != nullptr);
  CHECK(length == 5);
  CHECK(sampleRate == 8000);

  // Verify samples
  for (int i = 0; i < 5; i++) {
    CHECK(data[i] == testSamples[i]);
  }

  std::free(data);
  std::remove(testFile);
}

TEST_CASE("load wav 16bit mono") {
  const char* testFile = "test_16bit.wav";
  int16_t testSamples[] = {-32768, -16384, 0, 16384, 32767};
  createTestWav16Bit(testFile, 16000, testSamples, 5);

  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile(testFile, PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  CHECK(result == WAV_SUCCESS);
  CHECK(data != nullptr);
  CHECK(length == 5);
  CHECK(sampleRate == 16000);

  // Verify conversion (16-bit signed to 8-bit unsigned)
  // -32768 -> 0, -16384 -> 64, 0 -> 128, 16384 -> 192, 32767 -> 255
  CHECK(data[0] == 0);
  CHECK(data[1] == 64);
  CHECK(data[2] == 128);
  CHECK(data[3] == 192);
  CHECK(data[4] == 255);

  std::free(data);
  std::remove(testFile);
}

TEST_CASE("load wav file not found") {
  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile("nonexistent.wav", PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  CHECK(data == nullptr);
  CHECK(result == WAV_ERROR_FILE_NOT_FOUND);
}

TEST_CASE("load wav invalid file") {
  const char* testFile = "test_invalid.wav";
  FILE* f = std::fopen(testFile, "wb");
  std::fwrite("NOT A WAV FILE", 14, 1, f);
  std::fclose(f);

  uint16_t length, sampleRate;
  WavLoadResult result;
  uint8_t* data = loadWavFile(testFile, PROJECT_MAX_SAMPLE_SIZE,
                               &length, &sampleRate, &result);

  CHECK(data == nullptr);
  CHECK(result == WAV_ERROR_NOT_WAV);

  std::remove(testFile);
}

TEST_CASE("load wav size limit") {
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

  CHECK(result == WAV_SUCCESS);
  CHECK(data != nullptr);
  CHECK(length == 100); // Should be limited
  CHECK(sampleRate == 8000);

  std::free(data);
  std::remove(testFile);
}

TEST_CASE("wav error messages") {
  CHECK(std::strcmp(getWavLoadErrorMessage(WAV_SUCCESS), "Success") == 0);
  CHECK(std::strcmp(getWavLoadErrorMessage(WAV_ERROR_FILE_NOT_FOUND),
                    "File not found or cannot be opened") == 0);
  CHECK(std::strcmp(getWavLoadErrorMessage(WAV_ERROR_NOT_WAV),
                    "Not a valid WAV file") == 0);
}

} // TEST_SUITE("import_wav")
