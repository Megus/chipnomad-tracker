#ifndef WAV_FILE_H
#define WAV_FILE_H

#include <stdint.h>
#include <stdio.h>
#include "audio_source.h"

// Result codes for WAV operations
enum WavResult {
  WAV_OK = 0,
  WAV_ERR_FILE_NOT_FOUND,
  WAV_ERR_NOT_WAV,
  WAV_ERR_UNSUPPORTED_FORMAT,
  WAV_ERR_INVALID_DATA,
  WAV_ERR_MEMORY
};

// WAV file class: opens a WAV file, provides metadata, supports both
// truncated loading (for instruments) and streaming playback (for preview).
class WavFile : public AudioSource {
public:
  // Open and parse a WAV file. Check getResult() after construction.
  WavFile(const char* path);
  ~WavFile();

  // Status
  WavResult getResult() const { return result; }
  bool isOpen() const { return file != NULL; }
  static const char* getErrorMessage(WavResult r);

  // Metadata (valid after successful open)
  uint32_t getSampleRate() const override { return sampleRate; }
  uint16_t getNumChannels() const { return numChannels; }
  uint16_t getBitsPerSample() const { return bitsPerSample; }
  uint32_t getTotalSamples() const { return totalSamples; }

  // Load truncated sample as 8-bit unsigned PCM (for instrument loading).
  // Caller must free() the returned buffer.
  // Returns NULL on failure (check getResult()).
  uint8_t* loadTruncated(uint16_t maxLength, uint16_t* outLength, bool normalize);

  // Streaming: read next N sample frames as signed 16-bit mono.
  // Returns actual number of samples read (0 = end of file).
  uint32_t readSamples16(int16_t* buffer, uint32_t count) override;

  // Streaming: seek to a sample position (0 = start of audio data)
  void seek(uint32_t samplePosition);

  // Streaming: check if all samples have been read
  bool isFinished() const override { return streamPosition >= totalSamples; }

private:
  FILE* file;
  WavResult result;

  // Format info
  uint32_t sampleRate;
  uint16_t numChannels;
  uint16_t bitsPerSample;

  // Data chunk info
  uint32_t totalSamples;
  long dataStartOffset;  // File offset where audio data begins

  // Streaming state
  uint32_t streamPosition;

  // Internal: read one sample frame as 16-bit mono
  int readOneSample16(int16_t* out);

  // Prevent copying
  WavFile(const WavFile&);
  WavFile& operator=(const WavFile&);
};

#endif // WAV_FILE_H
