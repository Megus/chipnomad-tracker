#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "playback.h"

struct WAVHeader {
  char riff[4];
  uint32_t fileSize;
  char wave[4];
  char fmt[4];
  uint32_t fmtSize;
  uint16_t audioFormat;
  uint16_t channels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  char data[4];
  uint32_t dataSize;
};

static void writeWAVHeader(FILE* file, int sampleRate, int channels, int bitDepth, int dataSize) {
  WAVHeader header;

  memcpy(header.riff, "RIFF", 4);
  header.fileSize = 36 + dataSize;
  memcpy(header.wave, "WAVE", 4);
  memcpy(header.fmt, "fmt ", 4);
  header.fmtSize = 16;
  header.audioFormat = (bitDepth == 32) ? 3 : 1;
  header.channels = channels;
  header.sampleRate = sampleRate;
  header.bitsPerSample = bitDepth;
  header.byteRate = sampleRate * channels * (bitDepth / 8);
  header.blockAlign = channels * (bitDepth / 8);
  memcpy(header.data, "data", 4);
  header.dataSize = dataSize;

  fwrite(&header, sizeof(WAVHeader), 1, file);
}

///////////////////////////////////////////////////////////////////////////////
// ExporterWAV
///////////////////////////////////////////////////////////////////////////////

ExporterWAV::ExporterWAV(const char* path, Project* project, int startRow, int sampleRate, int bitDepth, float mixVolume, bool stems)
  : Exporter(project, startRow) {
  this->sampleRate = sampleRate;
  this->bitDepth = bitDepth;
  this->stems = stems;
  channels = 2;
  totalSamples = 0;
  currentTrack = 0;
  strncpy(this->basePath, path, sizeof(this->basePath) - 1);
  this->basePath[sizeof(this->basePath) - 1] = 0;

  if (stems) {
    fileCount = project->chipsCount * 3;
  } else {
    fileCount = 1;
  }

  files = (FILE**)malloc(sizeof(FILE*) * fileCount);
  for (int i = 0; i < fileCount; i++) {
    char fname[1024];
    if (stems) {
      snprintf(fname, sizeof(fname), "%s-%02d.wav", path, i + 1);
    } else {
      strncpy(fname, path, sizeof(fname) - 1);
      fname[sizeof(fname) - 1] = 0;
    }
    files[i] = fopen(fname, "wb");
    if (files[i]) {
      writeWAVHeader(files[i], sampleRate, channels, bitDepth, 0);
    }
  }

  renderBuffer = (float*)malloc(sizeof(float) * sampleRate * channels);

  chipnomadInitChips(chipnomadState, sampleRate, NULL);
  chipnomadSetQuality(chipnomadState, ChipNomadQuality::best);
  chipnomadState->mixVolume = mixVolume;

  if (stems) {
    for (int t = 0; t < PROJECT_MAX_TRACKS; t++) {
      chipnomadState->playbackState.trackEnabled[t] = (t == 0) ? 1 : 0;
    }
  }
}

int ExporterWAV::next() {
  int samplesRendered = chipnomadRender(chipnomadState, renderBuffer, sampleRate);

  if (samplesRendered > 0 && files[currentTrack]) {
    writeSamples(files[currentTrack], renderBuffer, samplesRendered);
  }

  if (samplesRendered < sampleRate) {
    if (stems) {
      currentTrack++;
      if (currentTrack >= fileCount) {
        return -1;
      }
      playbackStartSong(&chipnomadState->playbackState, 0, 0, 0);
      for (int t = 0; t < PROJECT_MAX_TRACKS; t++) {
        chipnomadState->playbackState.trackEnabled[t] = (t == currentTrack) ? 1 : 0;
      }
      totalSamples = 0;
    } else {
      return -1;
    }
  }

  return ++renderedSeconds;
}

int ExporterWAV::finish() {
  for (int i = 0; i < fileCount; i++) {
    if (files[i]) {
      int dataSize = totalSamples * channels * (bitDepth / 8);
      fseek(files[i], 0, 0);
      writeWAVHeader(files[i], sampleRate, channels, bitDepth, dataSize);
      fclose(files[i]);
      files[i] = NULL;
    }
  }
  free(files);
  files = NULL;
  free(renderBuffer);
  renderBuffer = NULL;
  return 0;
}

void ExporterWAV::cancel() {
  if (files) {
    for (int i = 0; i < fileCount; i++) {
      if (files[i]) {
        fclose(files[i]);
        files[i] = NULL;
      }
      // Remove the file
      char fname[1024];
      if (stems) {
        snprintf(fname, sizeof(fname), "%s-%02d.wav", basePath, i + 1);
      } else {
        strncpy(fname, basePath, sizeof(fname) - 1);
        fname[sizeof(fname) - 1] = 0;
      }
      remove(fname);
    }
    free(files);
    files = NULL;
  }
  if (renderBuffer) {
    free(renderBuffer);
    renderBuffer = NULL;
  }
}

void ExporterWAV::writeSamples(FILE* f, float* buffer, int samples) {
  if (bitDepth == 16) {
    for (int i = 0; i < samples * channels; i++) {
      int sample = (int)(buffer[i] * 32767.0f);
      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;
      int16_t finalSample = (int16_t)sample;
      fwrite(&finalSample, sizeof(int16_t), 1, f);
    }
  } else if (bitDepth == 24) {
    for (int i = 0; i < samples * channels; i++) {
      int sample = (int)(buffer[i] * 8388607.0f);
      if (sample > 8388607) sample = 8388607;
      if (sample < -8388608) sample = -8388608;
      uint8_t bytes[3] = {(uint8_t)(sample & 0xFF), (uint8_t)((sample >> 8) & 0xFF), (uint8_t)((sample >> 16) & 0xFF)};
      fwrite(bytes, 3, 1, f);
    }
  } else if (bitDepth == 32) {
    for (int i = 0; i < samples * channels; i++) {
      float sample = buffer[i];
      fwrite(&sample, sizeof(float), 1, f);
    }
  }
  totalSamples += samples;
}
