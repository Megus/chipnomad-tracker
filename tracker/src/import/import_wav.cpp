#include "import_wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// WAV file structures (little-endian)
struct WavHeader {
  char riffId[4];        // "RIFF"
  uint32_t fileSize;     // File size - 8
  char waveId[4];        // "WAVE"
};

struct WavFmtChunk {
  char chunkId[4];       // "fmt "
  uint32_t chunkSize;    // Size of fmt chunk (16 for PCM)
  uint16_t audioFormat;  // 1 = PCM
  uint16_t numChannels;  // 1 = mono, 2 = stereo
  uint32_t sampleRate;   // Sample rate in Hz
  uint32_t byteRate;     // sampleRate * numChannels * bitsPerSample/8
  uint16_t blockAlign;   // numChannels * bitsPerSample/8
  uint16_t bitsPerSample; // 8, 16, 24, or 32
};

struct WavDataChunkHeader {
  char chunkId[4];       // "data"
  uint32_t chunkSize;    // Size of data in bytes
};

// Error messages
static const char* errorMessages[] = {
  "Success",
  "File not found or cannot be opened",
  "Not a valid WAV file",
  "Unsupported WAV format (only PCM 8/16/24/32-bit supported)",
  "Invalid or corrupted WAV data",
  "Sample too large (exceeds maximum size)",
  "Memory allocation failed"
};

const char* getWavLoadErrorMessage(WavLoadResult result) {
  if (result < 0 || result >= sizeof(errorMessages) / sizeof(errorMessages[0])) {
    return "Unknown error";
  }
  return errorMessages[result];
}

// Helper: Read and validate RIFF/WAVE header
static int readWavHeader(FILE* file, WavHeader* header) {
  if (fread(header, sizeof(WavHeader), 1, file) != 1) {
    return 0;
  }

  // Validate RIFF header
  if (memcmp(header->riffId, "RIFF", 4) != 0) {
    return 0;
  }

  // Validate WAVE format
  if (memcmp(header->waveId, "WAVE", 4) != 0) {
    return 0;
  }

  return 1;
}

// Helper: Find and read fmt chunk
static int readFmtChunk(FILE* file, WavFmtChunk* fmt) {
  char chunkId[4];
  uint32_t chunkSize;

  // Search for fmt chunk (skip other chunks if needed)
  while (1) {
    if (fread(chunkId, 4, 1, file) != 1) {
      return 0;
    }
    if (fread(&chunkSize, 4, 1, file) != 1) {
      return 0;
    }

    if (memcmp(chunkId, "fmt ", 4) == 0) {
      // Found fmt chunk
      if (chunkSize < 16) {
        return 0; // fmt chunk too small
      }

      // Read fmt data
      if (fread(&fmt->audioFormat, 2, 1, file) != 1) return 0;
      if (fread(&fmt->numChannels, 2, 1, file) != 1) return 0;
      if (fread(&fmt->sampleRate, 4, 1, file) != 1) return 0;
      if (fread(&fmt->byteRate, 4, 1, file) != 1) return 0;
      if (fread(&fmt->blockAlign, 2, 1, file) != 1) return 0;
      if (fread(&fmt->bitsPerSample, 2, 1, file) != 1) return 0;

      // Copy chunk info
      memcpy(fmt->chunkId, chunkId, 4);
      fmt->chunkSize = chunkSize;

      // Skip any extra fmt data
      if (chunkSize > 16) {
        fseek(file, chunkSize - 16, SEEK_CUR);
      }

      return 1;
    } else {
      // Skip this chunk
      fseek(file, chunkSize, SEEK_CUR);
    }
  }
}

// Helper: Find and position at data chunk
static int findDataChunk(FILE* file, WavDataChunkHeader* dataHeader) {
  char chunkId[4];
  uint32_t chunkSize;

  // Search for data chunk
  while (1) {
    if (fread(chunkId, 4, 1, file) != 1) {
      return 0;
    }
    if (fread(&chunkSize, 4, 1, file) != 1) {
      return 0;
    }

    if (memcmp(chunkId, "data", 4) == 0) {
      // Found data chunk
      memcpy(dataHeader->chunkId, chunkId, 4);
      dataHeader->chunkSize = chunkSize;
      return 1;
    } else {
      // Skip this chunk
      fseek(file, chunkSize, SEEK_CUR);
    }
  }
}

// Helper: Read a single sample frame (all channels averaged) from file into a signed 16-bit value
// For 8-bit WAV: converts unsigned 0-255 to signed -128..127 range (as 16-bit: -32768..32512)
// For 16-bit WAV: keeps as-is
// For 24/32-bit WAV: scales down to 16-bit
static int readSampleAs16Bit(FILE* file, const WavFmtChunk* fmt, int16_t* out) {
  int32_t sampleSum = 0;

  for (int ch = 0; ch < fmt->numChannels; ch++) {
    int32_t sample = 0;

    switch (fmt->bitsPerSample) {
      case 8: {
        uint8_t s;
        if (fread(&s, 1, 1, file) != 1) return 0;
        // Convert unsigned 8-bit to signed 16-bit range
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
        sample = (int32_t)((bytes[0]) | (bytes[1] << 8) | (bytes[2] << 16));
        if (sample & 0x800000) {
          sample |= 0xFF000000;
        }
        // Scale 24-bit to 16-bit
        sample >>= 8;
        break;
      }

      case 32: {
        int32_t s;
        if (fread(&s, 4, 1, file) != 1) return 0;
        // Scale 32-bit to 16-bit
        sample = s >> 16;
        break;
      }
    }

    sampleSum += sample;
  }

  // Average channels if stereo
  if (fmt->numChannels > 1) {
    sampleSum /= fmt->numChannels;
  }

  // Clamp to 16-bit range
  if (sampleSum < -32768) sampleSum = -32768;
  if (sampleSum > 32767) sampleSum = 32767;

  *out = (int16_t)sampleSum;
  return 1;
}

// Helper: Read and convert sample data
static uint8_t* readAndConvertSamples(FILE* file, const WavFmtChunk* fmt,
                                      uint32_t dataSize, uint16_t maxLength,
                                      uint16_t* outLength, WavLoadResult* result,
                                      bool normalize) {
  // Calculate number of samples
  uint32_t bytesPerSample = fmt->bitsPerSample / 8;
  uint32_t totalSamples = dataSize / (bytesPerSample * fmt->numChannels);

  // Check if sample count exceeds maximum - truncate if needed
  if (totalSamples > maxLength) {
    totalSamples = maxLength;
  }

  *outLength = (uint16_t)totalSamples;

  // Read all samples into 16-bit buffer (shared for both paths)
  int16_t* buf16 = (int16_t*)malloc(totalSamples * sizeof(int16_t));
  if (buf16 == NULL) {
    *result = WAV_ERROR_MEMORY;
    return NULL;
  }

  for (uint32_t i = 0; i < totalSamples; i++) {
    if (!readSampleAs16Bit(file, fmt, &buf16[i])) {
      free(buf16);
      *result = WAV_ERROR_INVALID_DATA;
      return NULL;
    }
  }

  // Allocate output buffer
  uint8_t* output = (uint8_t*)malloc(totalSamples);
  if (output == NULL) {
    free(buf16);
    *result = WAV_ERROR_MEMORY;
    return NULL;
  }

  if (normalize) {
    // Find peak absolute value in the truncated sample
    int32_t peak = 0;
    for (uint32_t i = 0; i < totalSamples; i++) {
      int32_t abs_val = buf16[i] < 0 ? -(int32_t)buf16[i] : (int32_t)buf16[i];
      if (abs_val > peak) peak = abs_val;
    }

    // Normalize and convert to 8-bit unsigned
    if (peak == 0) {
      // Silent sample - fill with center value
      memset(output, 128, totalSamples);
    } else {
      for (uint32_t i = 0; i < totalSamples; i++) {
        // Scale to full 16-bit range: sample * 32767 / peak
        int32_t normalized = (int32_t)buf16[i] * 32767 / peak;
        // Convert signed 16-bit to unsigned 8-bit: (normalized + 32768) >> 8
        int32_t scaled = (normalized + 32768) >> 8;
        if (scaled < 0) scaled = 0;
        if (scaled > 255) scaled = 255;
        output[i] = (uint8_t)scaled;
      }
    }
  } else {
    // Straight 16-bit to 8-bit conversion (no normalization)
    for (uint32_t i = 0; i < totalSamples; i++) {
      int32_t scaled = ((int32_t)buf16[i] + 32768) >> 8;
      if (scaled < 0) scaled = 0;
      if (scaled > 255) scaled = 255;
      output[i] = (uint8_t)scaled;
    }
  }

  free(buf16);
  *result = WAV_SUCCESS;
  return output;
}

uint8_t* loadWavFile(const char* path, uint16_t maxLength,
                     uint16_t* outLength, uint16_t* outSampleRate,
                     WavLoadResult* outResult, bool normalize) {
  FILE* file = NULL;
  WavHeader header;
  WavFmtChunk fmt;
  WavDataChunkHeader dataHeader;
  uint8_t* sampleData = NULL;

  // Initialize output parameters
  *outLength = 0;
  *outSampleRate = 0;
  *outResult = WAV_SUCCESS;

  // Open file
  file = fopen(path, "rb");
  if (file == NULL) {
    *outResult = WAV_ERROR_FILE_NOT_FOUND;
    return NULL;
  }

  // Read and validate WAV header
  if (!readWavHeader(file, &header)) {
    *outResult = WAV_ERROR_NOT_WAV;
    fclose(file);
    return NULL;
  }

  // Read fmt chunk
  if (!readFmtChunk(file, &fmt)) {
    *outResult = WAV_ERROR_INVALID_DATA;
    fclose(file);
    return NULL;
  }

  // Validate format
  if (fmt.audioFormat != 1) {
    *outResult = WAV_ERROR_UNSUPPORTED_FORMAT; // Not PCM
    fclose(file);
    return NULL;
  }

  if (fmt.bitsPerSample != 8 && fmt.bitsPerSample != 16 &&
      fmt.bitsPerSample != 24 && fmt.bitsPerSample != 32) {
    *outResult = WAV_ERROR_UNSUPPORTED_FORMAT;
    fclose(file);
    return NULL;
  }

  if (fmt.numChannels < 1 || fmt.numChannels > 2) {
    *outResult = WAV_ERROR_UNSUPPORTED_FORMAT;
    fclose(file);
    return NULL;
  }

  if (fmt.sampleRate < 1000 || fmt.sampleRate > 96000) {
    *outResult = WAV_ERROR_UNSUPPORTED_FORMAT;
    fclose(file);
    return NULL;
  }

  // Find data chunk
  if (!findDataChunk(file, &dataHeader)) {
    *outResult = WAV_ERROR_INVALID_DATA;
    fclose(file);
    return NULL;
  }

  // Validate data size
  if (dataHeader.chunkSize == 0) {
    *outResult = WAV_ERROR_INVALID_DATA;
    fclose(file);
    return NULL;
  }

  // Read and convert sample data
  sampleData = readAndConvertSamples(file, &fmt, dataHeader.chunkSize,
                                     maxLength, outLength, outResult,
                                     normalize);

  if (sampleData != NULL) {
    *outSampleRate = (uint16_t)fmt.sampleRate;
  }

  fclose(file);
  return sampleData;
}
