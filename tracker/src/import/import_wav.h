#ifndef IMPORT_WAV_H
#define IMPORT_WAV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Result codes for WAV loading
typedef enum {
  WAV_SUCCESS = 0,
  WAV_ERROR_FILE_NOT_FOUND,
  WAV_ERROR_NOT_WAV,
  WAV_ERROR_UNSUPPORTED_FORMAT,
  WAV_ERROR_INVALID_DATA,
  WAV_ERROR_TOO_LARGE,
  WAV_ERROR_MEMORY
} WavLoadResult;

// Load a WAV file and convert to 8-bit unsigned PCM (0-255)
// Returns pointer to allocated sample data (caller must free), or NULL on error
// Parameters:
//   path: Path to WAV file
//   maxLength: Maximum sample length in bytes (e.g., PROJECT_MAX_SAMPLE_SIZE)
//   outLength: Pointer to store actual sample length
//   outSampleRate: Pointer to store sample rate from WAV file
//   outResult: Pointer to store result code
// Returns: Pointer to sample data (uint8_t array), or NULL on failure
uint8_t* loadWavFile(const char* path, uint16_t maxLength,
                     uint16_t* outLength, uint16_t* outSampleRate,
                     WavLoadResult* outResult);

// Get human-readable error message for result code
const char* getWavLoadErrorMessage(WavLoadResult result);


#ifdef __cplusplus
}
#endif

#endif
