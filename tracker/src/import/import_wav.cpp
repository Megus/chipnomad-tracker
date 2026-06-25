#include "import_wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// WAV file structures (little-endian)
typedef struct {
  char riffId[4];        // "RIFF"
  uint32_t fileSize;     // File size - 8
  char waveId[4];        // "WAVE"
} WavHeader;

typedef struct {
  char chunkId[4];       // "fmt "
  uint32_t chunkSize;    // Size of fmt chunk (16 for PCM)
  uint16_t audioFormat;  // 1 = PCM
  uint16_t numChannels;  // 1 = mono, 2 = stereo
  uint32_t sampleRate;   // Sample rate in Hz
  uint32_t byteRate;     // sampleRate * numChannels * bitsPerSample/8
  uint16_t blockAlign;   // numChannels * bitsPerSample/8
  uint16_t bitsPerSample; // 8, 16, 24, or 32
} WavFmtChunk;

typedef struct {
  char chunkId[4];       // "data"
  uint32_t chunkSize;    // Size of data in bytes
} WavDataChunkHeader;

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

// Helper: Convert sample to 8-bit unsigned
static inline uint8_t convertSampleTo8Bit(int32_t sample, int bitsPerSample) {
  // Scale to 8-bit range and convert to unsigned
  int32_t scaled;

  switch (bitsPerSample) {
    case 8:
      // 8-bit WAV is already unsigned (0-255)
      return (uint8_t)sample;

    case 16:
      // 16-bit signed (-32768 to 32767) -> 8-bit unsigned (0-255)
      scaled = (sample + 32768) >> 8;
      break;

    case 24:
      // 24-bit signed -> 8-bit unsigned
      scaled = (sample + 8388608) >> 16;
      break;

    case 32:
      // 32-bit signed -> 8-bit unsigned
      scaled = ((int64_t)sample + 2147483648LL) >> 24;
      break;

    default:
      return 128; // Silence
  }

  // Clamp to 0-255
  if (scaled < 0) scaled = 0;
  if (scaled > 255) scaled = 255;

  return (uint8_t)scaled;
}

// Helper: Read and convert sample data
static uint8_t* readAndConvertSamples(FILE* file, const WavFmtChunk* fmt,
                                      uint32_t dataSize, uint16_t maxLength,
                                      uint16_t* outLength, WavLoadResult* result) {
  // Calculate number of samples
  uint32_t bytesPerSample = fmt->bitsPerSample / 8;
  uint32_t totalSamples = dataSize / (bytesPerSample * fmt->numChannels);

  // Check if sample count exceeds maximum - truncate if needed
  if (totalSamples > maxLength) {
    totalSamples = maxLength;
  }

  *outLength = (uint16_t)totalSamples;

  // Allocate output buffer
  uint8_t* output = (uint8_t*)malloc(totalSamples);
  if (output == NULL) {
    *result = WAV_ERROR_MEMORY;
    return NULL;
  }

  // Read and convert samples
  for (uint32_t i = 0; i < totalSamples; i++) {
    int32_t sampleSum = 0;

    // Read all channels and average them
    for (int ch = 0; ch < fmt->numChannels; ch++) {
      int32_t sample = 0;

      // Read sample based on bit depth
      switch (fmt->bitsPerSample) {
        case 8: {
          uint8_t s;
          if (fread(&s, 1, 1, file) != 1) {
            free(output);
            *result = WAV_ERROR_INVALID_DATA;
            return NULL;
          }
          sample = s;
          break;
        }

        case 16: {
          int16_t s;
          if (fread(&s, 2, 1, file) != 1) {
            free(output);
            *result = WAV_ERROR_INVALID_DATA;
            return NULL;
          }
          sample = s;
          break;
        }

        case 24: {
          uint8_t bytes[3];
          if (fread(bytes, 3, 1, file) != 1) {
            free(output);
            *result = WAV_ERROR_INVALID_DATA;
            return NULL;
          }
          // Reconstruct 24-bit signed value (little-endian)
          sample = (int32_t)((bytes[0]) | (bytes[1] << 8) | (bytes[2] << 16));
          // Sign extend from 24-bit to 32-bit
          if (sample & 0x800000) {
            sample |= 0xFF000000;
          }
          break;
        }

        case 32: {
          int32_t s;
          if (fread(&s, 4, 1, file) != 1) {
            free(output);
            *result = WAV_ERROR_INVALID_DATA;
            return NULL;
          }
          sample = s;
          break;
        }
      }

      sampleSum += sample;
    }

    // Average channels if stereo
    if (fmt->numChannels > 1) {
      sampleSum /= fmt->numChannels;
    }

    // Convert to 8-bit unsigned
    output[i] = convertSampleTo8Bit(sampleSum, fmt->bitsPerSample);
  }

  *result = WAV_SUCCESS;
  return output;
}

uint8_t* loadWavFile(const char* path, uint16_t maxLength,
                     uint16_t* outLength, uint16_t* outSampleRate,
                     WavLoadResult* outResult) {
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
                                     maxLength, outLength, outResult);

  if (sampleData != NULL) {
    *outSampleRate = (uint16_t)fmt.sampleRate;
  }

  fclose(file);
  return sampleData;
}
