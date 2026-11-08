#include "wav_file.h"
#include <stdlib.h>
#include <string.h>

// WAV file structures (little-endian)
struct WavHeader {
  char riffId[4];        // "RIFF"
  uint32_t fileSize;     // File size - 8
  char waveId[4];        // "WAVE"
};

struct WavFmtChunk {
  uint16_t audioFormat;  // 1 = PCM
  uint16_t numChannels;  // 1 = mono, 2 = stereo
  uint32_t sampleRate;   // Sample rate in Hz
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample; // 8, 16, 24, or 32
};

static const char* errorMessages[] = {
  "Success",
  "File not found or cannot be opened",
  "Not a valid WAV file",
  "Unsupported WAV format (only PCM 8/16/24/32-bit supported)",
  "Invalid or corrupted WAV data",
  "Memory allocation failed"
};

const char* WavFile::getErrorMessage(WavResult r) {
  if (r < 0 || r >= (int)(sizeof(errorMessages) / sizeof(errorMessages[0]))) {
    return "Unknown error";
  }
  return errorMessages[r];
}

// Find a chunk by ID, return its size. File position is left at chunk data start.
static int findChunk(FILE* f, const char* id, uint32_t* outSize) {
  char chunkId[4];
  uint32_t chunkSize;

  while (1) {
    if (fread(chunkId, 4, 1, f) != 1) return 0;
    if (fread(&chunkSize, 4, 1, f) != 1) return 0;

    if (memcmp(chunkId, id, 4) == 0) {
      *outSize = chunkSize;
      return 1;
    }
    // Skip this chunk
    fseek(f, chunkSize, SEEK_CUR);
  }
}

WavFile::WavFile(const char* path) {
  file = NULL;
  result = WAV_OK;
  sampleRate = 0;
  numChannels = 0;
  bitsPerSample = 0;
  totalSamples = 0;
  dataStartOffset = 0;
  streamPosition = 0;

  file = fopen(path, "rb");
  if (!file) {
    result = WAV_ERR_FILE_NOT_FOUND;
    return;
  }

  // Read and validate RIFF/WAVE header
  WavHeader header;
  if (fread(&header, sizeof(WavHeader), 1, file) != 1 ||
      memcmp(header.riffId, "RIFF", 4) != 0 ||
      memcmp(header.waveId, "WAVE", 4) != 0) {
    result = WAV_ERR_NOT_WAV;
    fclose(file);
    file = NULL;
    return;
  }

  // Find and read fmt chunk
  uint32_t fmtSize;
  if (!findChunk(file, "fmt ", &fmtSize) || fmtSize < 16) {
    result = WAV_ERR_INVALID_DATA;
    fclose(file);
    file = NULL;
    return;
  }

  WavFmtChunk fmt;
  if (fread(&fmt.audioFormat, 2, 1, file) != 1) { result = WAV_ERR_INVALID_DATA; fclose(file); file = NULL; return; }
  if (fread(&fmt.numChannels, 2, 1, file) != 1) { result = WAV_ERR_INVALID_DATA; fclose(file); file = NULL; return; }
  if (fread(&fmt.sampleRate, 4, 1, file) != 1) { result = WAV_ERR_INVALID_DATA; fclose(file); file = NULL; return; }
  if (fread(&fmt.byteRate, 4, 1, file) != 1) { result = WAV_ERR_INVALID_DATA; fclose(file); file = NULL; return; }
  if (fread(&fmt.blockAlign, 2, 1, file) != 1) { result = WAV_ERR_INVALID_DATA; fclose(file); file = NULL; return; }
  if (fread(&fmt.bitsPerSample, 2, 1, file) != 1) { result = WAV_ERR_INVALID_DATA; fclose(file); file = NULL; return; }

  // Skip extra fmt data
  if (fmtSize > 16) {
    fseek(file, fmtSize - 16, SEEK_CUR);
  }

  // Validate format
  if (fmt.audioFormat != 1) {
    result = WAV_ERR_UNSUPPORTED_FORMAT;
    fclose(file);
    file = NULL;
    return;
  }
  if (fmt.bitsPerSample != 8 && fmt.bitsPerSample != 16 &&
      fmt.bitsPerSample != 24 && fmt.bitsPerSample != 32) {
    result = WAV_ERR_UNSUPPORTED_FORMAT;
    fclose(file);
    file = NULL;
    return;
  }
  if (fmt.numChannels < 1 || fmt.numChannels > 2) {
    result = WAV_ERR_UNSUPPORTED_FORMAT;
    fclose(file);
    file = NULL;
    return;
  }
  if (fmt.sampleRate < 1000 || fmt.sampleRate > 96000) {
    result = WAV_ERR_UNSUPPORTED_FORMAT;
    fclose(file);
    file = NULL;
    return;
  }

  // Store format info
  sampleRate = fmt.sampleRate;
  numChannels = fmt.numChannels;
  bitsPerSample = fmt.bitsPerSample;

  // Find data chunk
  uint32_t dataSize;
  if (!findChunk(file, "data", &dataSize) || dataSize == 0) {
    result = WAV_ERR_INVALID_DATA;
    fclose(file);
    file = NULL;
    return;
  }

  uint32_t bytesPerFrame = (bitsPerSample / 8) * numChannels;
  totalSamples = dataSize / bytesPerFrame;
  dataStartOffset = ftell(file);
  streamPosition = 0;
}

WavFile::~WavFile() {
  if (file) {
    fclose(file);
    file = NULL;
  }
}

int WavFile::readOneSample16(int16_t* out) {
  if (!file || streamPosition >= totalSamples) return 0;

  int32_t sampleSum = 0;

  for (int ch = 0; ch < numChannels; ch++) {
    int32_t sample = 0;

    switch (bitsPerSample) {
      case 8: {
        uint8_t s;
        if (fread(&s, 1, 1, file) != 1) return 0;
        sample = ((int32_t)s - 128) * 256;
        break;
      }
      case 16: {
        int16_t s;
        if (fread(&s, 2, 1, file) != 1) return 0;
        sample = s;
        break;
      }
      case 24: {
        uint8_t bytes[3];
        if (fread(bytes, 3, 1, file) != 1) return 0;
        sample = (int32_t)(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
        if (sample & 0x800000) sample |= 0xFF000000;
        sample >>= 8;
        break;
      }
      case 32: {
        int32_t s;
        if (fread(&s, 4, 1, file) != 1) return 0;
        sample = s >> 16;
        break;
      }
    }

    sampleSum += sample;
  }

  if (numChannels > 1) {
    sampleSum /= numChannels;
  }

  if (sampleSum < -32768) sampleSum = -32768;
  if (sampleSum > 32767) sampleSum = 32767;

  *out = (int16_t)sampleSum;
  streamPosition++;
  return 1;
}

uint32_t WavFile::readSamples16(int16_t* buffer, uint32_t count) {
  uint32_t read = 0;
  for (uint32_t i = 0; i < count; i++) {
    if (!readOneSample16(&buffer[i])) break;
    read++;
  }
  return read;
}

void WavFile::seek(uint32_t samplePosition) {
  if (!file) return;
  if (samplePosition > totalSamples) samplePosition = totalSamples;

  uint32_t bytesPerFrame = (bitsPerSample / 8) * numChannels;
  long offset = dataStartOffset + (long)samplePosition * bytesPerFrame;
  fseek(file, offset, SEEK_SET);
  streamPosition = samplePosition;
}

uint8_t* WavFile::loadTruncated(uint16_t maxLength, uint16_t* outLength, bool normalize) {
  if (!file || result != WAV_OK) {
    return NULL;
  }

  // Seek to start of data
  seek(0);

  // Calculate how many samples to read
  uint32_t samplesToRead = totalSamples;
  if (samplesToRead > maxLength) {
    samplesToRead = maxLength;
  }

  *outLength = (uint16_t)samplesToRead;

  // Read all samples into 16-bit buffer
  int16_t* buf16 = (int16_t*)malloc(samplesToRead * sizeof(int16_t));
  if (!buf16) {
    result = WAV_ERR_MEMORY;
    return NULL;
  }

  uint32_t actualRead = readSamples16(buf16, samplesToRead);
  if (actualRead < samplesToRead) {
    *outLength = (uint16_t)actualRead;
    samplesToRead = actualRead;
  }

  // Allocate output buffer
  uint8_t* output = (uint8_t*)malloc(samplesToRead);
  if (!output) {
    free(buf16);
    result = WAV_ERR_MEMORY;
    return NULL;
  }

  if (normalize) {
    // Find peak absolute value
    int32_t peak = 0;
    for (uint32_t i = 0; i < samplesToRead; i++) {
      int32_t abs_val = buf16[i] < 0 ? -(int32_t)buf16[i] : (int32_t)buf16[i];
      if (abs_val > peak) peak = abs_val;
    }

    if (peak == 0) {
      memset(output, 128, samplesToRead);
    } else {
      for (uint32_t i = 0; i < samplesToRead; i++) {
        int32_t normalized = (int32_t)buf16[i] * 32767 / peak;
        int32_t scaled = (normalized + 32768) >> 8;
        if (scaled < 0) scaled = 0;
        if (scaled > 255) scaled = 255;
        output[i] = (uint8_t)scaled;
      }
    }
  } else {
    for (uint32_t i = 0; i < samplesToRead; i++) {
      int32_t scaled = ((int32_t)buf16[i] + 32768) >> 8;
      if (scaled < 0) scaled = 0;
      if (scaled > 255) scaled = 255;
      output[i] = (uint8_t)scaled;
    }
  }

  free(buf16);
  return output;
}
