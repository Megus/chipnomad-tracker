#include "doctest.h"

#include "wav_file.h"
#include "project_constants.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

TEST_SUITE("wav_file") {

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

TEST_CASE("WavFile load 8bit mono") {
  const char* testFile = "test_8bit.wav";
  uint8_t testSamples[] = {0, 64, 128, 192, 255};
  createTestWav8Bit(testFile, 8000, testSamples, 5);

  WavFile wav(testFile);
  CHECK(wav.getResult() == WAV_OK);
  CHECK(wav.isOpen());
  CHECK(wav.getSampleRate() == 8000);
  CHECK(wav.getTotalSamples() == 5);
  CHECK(wav.getBitsPerSample() == 8);
  CHECK(wav.getNumChannels() == 1);

  uint16_t length;
  uint8_t* data = wav.loadTruncated(PROJECT_MAX_SAMPLE_SIZE, &length, false);

  CHECK(data != nullptr);
  CHECK(length == 5);

  // Verify samples (8-bit unsigned stays the same)
  for (int i = 0; i < 5; i++) {
    CHECK(data[i] == testSamples[i]);
  }

  std::free(data);
  std::remove(testFile);
}

TEST_CASE("WavFile load 16bit mono") {
  const char* testFile = "test_16bit.wav";
  int16_t testSamples[] = {-32768, -16384, 0, 16384, 32767};
  createTestWav16Bit(testFile, 16000, testSamples, 5);

  WavFile wav(testFile);
  CHECK(wav.getResult() == WAV_OK);
  CHECK(wav.getSampleRate() == 16000);
  CHECK(wav.getTotalSamples() == 5);

  uint16_t length;
  uint8_t* data = wav.loadTruncated(PROJECT_MAX_SAMPLE_SIZE, &length, false);

  CHECK(data != nullptr);
  CHECK(length == 5);

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

TEST_CASE("WavFile file not found") {
  WavFile wav("nonexistent.wav");
  CHECK(wav.getResult() == WAV_ERR_FILE_NOT_FOUND);
  CHECK(!wav.isOpen());
}

TEST_CASE("WavFile invalid file") {
  const char* testFile = "test_invalid.wav";
  FILE* f = std::fopen(testFile, "wb");
  std::fwrite("NOT A WAV FILE", 14, 1, f);
  std::fclose(f);

  WavFile wav(testFile);
  CHECK(wav.getResult() == WAV_ERR_NOT_WAV);
  CHECK(!wav.isOpen());

  std::remove(testFile);
}

TEST_CASE("WavFile truncation") {
  const char* testFile = "test_large.wav";
  uint8_t testSamples[1000];
  for (int i = 0; i < 1000; i++) {
    testSamples[i] = (uint8_t)(i & 0xFF);
  }
  createTestWav8Bit(testFile, 8000, testSamples, 1000);

  WavFile wav(testFile);
  CHECK(wav.getResult() == WAV_OK);
  CHECK(wav.getTotalSamples() == 1000);

  uint16_t length;
  uint8_t* data = wav.loadTruncated(100, &length, false);

  CHECK(data != nullptr);
  CHECK(length == 100);

  std::free(data);
  std::remove(testFile);
}

TEST_CASE("WavFile streaming read") {
  const char* testFile = "test_stream.wav";
  int16_t testSamples[] = {-32768, -16384, 0, 16384, 32767};
  createTestWav16Bit(testFile, 44100, testSamples, 5);

  WavFile wav(testFile);
  CHECK(wav.getResult() == WAV_OK);

  // Read samples in streaming mode
  int16_t buffer[3];
  uint32_t read = wav.readSamples16(buffer, 3);
  CHECK(read == 3);
  CHECK(buffer[0] == -32768);
  CHECK(buffer[1] == -16384);
  CHECK(buffer[2] == 0);

  // Read remaining
  read = wav.readSamples16(buffer, 3);
  CHECK(read == 2);  // Only 2 left
  CHECK(buffer[0] == 16384);
  CHECK(buffer[1] == 32767);

  CHECK(wav.isFinished());

  std::remove(testFile);
}

TEST_CASE("WavFile seek") {
  const char* testFile = "test_seek.wav";
  int16_t testSamples[] = {100, 200, 300, 400, 500};
  createTestWav16Bit(testFile, 22050, testSamples, 5);

  WavFile wav(testFile);
  CHECK(wav.getResult() == WAV_OK);

  // Seek to sample 3
  wav.seek(3);
  int16_t buffer[2];
  uint32_t read = wav.readSamples16(buffer, 2);
  CHECK(read == 2);
  CHECK(buffer[0] == 400);
  CHECK(buffer[1] == 500);

  // Seek back to start
  wav.seek(0);
  read = wav.readSamples16(buffer, 1);
  CHECK(read == 1);
  CHECK(buffer[0] == 100);

  std::remove(testFile);
}

TEST_CASE("WavFile error messages") {
  CHECK(std::strcmp(WavFile::getErrorMessage(WAV_OK), "Success") == 0);
  CHECK(std::strcmp(WavFile::getErrorMessage(WAV_ERR_FILE_NOT_FOUND),
                    "File not found or cannot be opened") == 0);
  CHECK(std::strcmp(WavFile::getErrorMessage(WAV_ERR_NOT_WAV),
                    "Not a valid WAV file") == 0);
}

} // TEST_SUITE("wav_file")
