#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "corelib/corelib_file.h"
#include "playback.h"
#include "chips/chips.h"
#include "chipnomad_lib.h"



// WAV implementation data
typedef struct {
  int fileId;
  int sampleRate;
  int channels;
  int bitDepth;
  int totalSamples;
  int allTracksStopped;
  int renderedSeconds;
  char filename[1024];
} WAVExporterData;

// WAV Stems implementation data
typedef struct {
  int* fileIds;
  int trackCount;
  int currentTrack;
  int sampleRate;
  int channels;
  int bitDepth;
  int totalSamples;
  int allTracksStopped;
  int renderedSeconds;
  char basePath[512];
} WAVStemsExporterData;

typedef struct {
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
} WAVHeader;

static void writeWAVHeader(int fileId, int sampleRate, int channels, int bitDepth, int dataSize) {
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

  fileWrite(fileId, &header, sizeof(WAVHeader));
}

static int wavExportWrite(WAVExporterData* data, float* buffer, int samples) {
  if (data->bitDepth == 16) {
    for (int i = 0; i < samples * data->channels; i++) {
      int sample = (int)(buffer[i] * 32767.0f);
      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;
      int16_t finalSample = (int16_t)sample;
      fileWrite(data->fileId, &finalSample, sizeof(int16_t));
    }
  } else if (data->bitDepth == 24) {
    for (int i = 0; i < samples * data->channels; i++) {
      int sample = (int)(buffer[i] * 8388607.0f);
      if (sample > 8388607) sample = 8388607;
      if (sample < -8388608) sample = -8388608;
      uint8_t bytes[3] = {sample & 0xFF, (sample >> 8) & 0xFF, (sample >> 16) & 0xFF};
      fileWrite(data->fileId, bytes, 3);
    }
  } else if (data->bitDepth == 32) {
    for (int i = 0; i < samples * data->channels; i++) {
      float sample = buffer[i];
      fileWrite(data->fileId, &sample, sizeof(float));
    }
  }
  data->totalSamples += samples;
  return 0;
}

// WAV exporter methods
static int wavNext(Exporter* self) {
  WAVExporterData* data = (WAVExporterData*)self->data;
  if (data->allTracksStopped) return -1;

  float buffer[data->sampleRate * 2];
  int samplesRendered = chipnomadRender(self->chipnomadState, buffer, data->sampleRate);

  if (samplesRendered > 0) {
    wavExportWrite(data, buffer, samplesRendered);
  }

  if (samplesRendered < data->sampleRate) {
    data->allTracksStopped = 1;
    return -1;
  }

  return ++data->renderedSeconds;
}

static int wavFinish(Exporter* self) {
  WAVExporterData* data = (WAVExporterData*)self->data;

  int dataSize = data->totalSamples * data->channels * (data->bitDepth / 8);
  fileSeek(data->fileId, 0, 0);
  writeWAVHeader(data->fileId, data->sampleRate, data->channels, data->bitDepth, dataSize);

  chipnomadDestroy(self->chipnomadState);
  fileClose(data->fileId);
  free(data);
  free(self);
  return 0;
}

static void wavCancel(Exporter* self) {
  WAVExporterData* data = (WAVExporterData*)self->data;

  chipnomadDestroy(self->chipnomadState);
  fileClose(data->fileId);
  fileDelete(data->filename);
  free(data);
  free(self);
}

Exporter* createWAVExporter(const char* filename, Project* project, int startRow, int sampleRate, int bitDepth) {
  Exporter* exporter = malloc(sizeof(Exporter));
  if (!exporter) return NULL;

  WAVExporterData* data = malloc(sizeof(WAVExporterData));
  if (!data) {
    free(exporter);
    return NULL;
  }

  data->fileId = fileOpen(filename, 1);
  if (data->fileId == -1) {
    free(data);
    free(exporter);
    return NULL;
  }

  data->sampleRate = sampleRate;
  data->channels = 2;
  data->bitDepth = bitDepth;
  data->totalSamples = 0;
  data->allTracksStopped = 0;
  data->renderedSeconds = 0;
  strncpy(data->filename, filename, sizeof(data->filename) - 1);
  data->filename[sizeof(data->filename) - 1] = 0;

  writeWAVHeader(data->fileId, sampleRate, 2, bitDepth, 0);

  // Create ChipNomad state and initialize
  exporter->chipnomadState = chipnomadCreate();
  if (!exporter->chipnomadState) {
    fileClose(data->fileId);
    free(data);
    free(exporter);
    return NULL;
  }

  // Copy project data and reinitialize playback
  exporter->chipnomadState->project = *project;
  playbackInit(&exporter->chipnomadState->playbackState, &exporter->chipnomadState->project);

  // Initialize chips
  chipnomadInitChips(exporter->chipnomadState, sampleRate, NULL);

  // Use best quality for export
  chipnomadSetQuality(exporter->chipnomadState, CHIPNOMAD_QUALITY_BEST);

  playbackStartSong(&exporter->chipnomadState->playbackState, startRow, 0, 0);

  exporter->data = data;
  exporter->next = wavNext;
  exporter->finish = wavFinish;
  exporter->cancel = wavCancel;

  return exporter;
}

// WAV Stems exporter methods
static int wavStemsNext(Exporter* self) {
  WAVStemsExporterData* data = (WAVStemsExporterData*)self->data;
  if (data->allTracksStopped) return -1;

  float buffer[data->sampleRate * 2];
  int samplesRendered = chipnomadRender(self->chipnomadState, buffer, data->sampleRate);

  if (samplesRendered > 0) {
    wavExportWrite((WAVExporterData*)&(WAVExporterData){
      .fileId = data->fileIds[data->currentTrack],
      .sampleRate = data->sampleRate,
      .channels = data->channels,
      .bitDepth = data->bitDepth,
      .totalSamples = 0
    }, buffer, samplesRendered);
    data->totalSamples += samplesRendered;
  }

  if (samplesRendered < data->sampleRate) {
    data->currentTrack++;
    if (data->currentTrack >= data->trackCount) {
      data->allTracksStopped = 1;
      return -1;
    }

    playbackStartSong(&self->chipnomadState->playbackState, 0, 0, 0);
    for (int t = 0; t < PROJECT_MAX_TRACKS; t++) {
      self->chipnomadState->playbackState.trackEnabled[t] = (t == data->currentTrack) ? 1 : 0;
    }
    data->totalSamples = 0;
  }

  return ++data->renderedSeconds;
}

static int wavStemsFinish(Exporter* self) {
  WAVStemsExporterData* data = (WAVStemsExporterData*)self->data;

  for (int i = 0; i < data->trackCount; i++) {
    int dataSize = data->totalSamples * data->channels * (data->bitDepth / 8);
    fileSeek(data->fileIds[i], 0, 0);
    writeWAVHeader(data->fileIds[i], data->sampleRate, data->channels, data->bitDepth, dataSize);
    fileClose(data->fileIds[i]);
  }

  chipnomadDestroy(self->chipnomadState);
  free(data->fileIds);
  free(data);
  free(self);
  return 0;
}

static void wavStemsCancel(Exporter* self) {
  WAVStemsExporterData* data = (WAVStemsExporterData*)self->data;

  for (int i = 0; i < data->trackCount; i++) {
    fileClose(data->fileIds[i]);
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s-%02d.wav", data->basePath, i + 1);
    fileDelete(filename);
  }

  chipnomadDestroy(self->chipnomadState);
  free(data->fileIds);
  free(data);
  free(self);
}

Exporter* createWAVStemsExporter(const char* basePath, Project* project, int startRow, int sampleRate, int bitDepth) {
  Exporter* exporter = malloc(sizeof(Exporter));
  if (!exporter) return NULL;

  WAVStemsExporterData* data = malloc(sizeof(WAVStemsExporterData));
  if (!data) {
    free(exporter);
    return NULL;
  }

  int trackCount = project->chipsCount * 3;
  data->fileIds = malloc(sizeof(int) * trackCount);
  if (!data->fileIds) {
    free(data);
    free(exporter);
    return NULL;
  }

  for (int i = 0; i < trackCount; i++) {
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s-%02d.wav", basePath, i + 1);
    data->fileIds[i] = fileOpen(filename, 1);
    if (data->fileIds[i] == -1) {
      for (int j = 0; j < i; j++) {
        fileClose(data->fileIds[j]);
      }
      free(data->fileIds);
      free(data);
      free(exporter);
      return NULL;
    }
    writeWAVHeader(data->fileIds[i], sampleRate, 2, bitDepth, 0);
  }

  data->trackCount = trackCount;
  data->currentTrack = 0;
  data->sampleRate = sampleRate;
  data->channels = 2;
  data->bitDepth = bitDepth;
  data->totalSamples = 0;
  data->allTracksStopped = 0;
  data->renderedSeconds = 0;
  strncpy(data->basePath, basePath, sizeof(data->basePath) - 1);
  data->basePath[sizeof(data->basePath) - 1] = 0;

  exporter->chipnomadState = chipnomadCreate();
  if (!exporter->chipnomadState) {
    for (int i = 0; i < trackCount; i++) {
      fileClose(data->fileIds[i]);
    }
    free(data->fileIds);
    free(data);
    free(exporter);
    return NULL;
  }

  exporter->chipnomadState->project = *project;
  playbackInit(&exporter->chipnomadState->playbackState, &exporter->chipnomadState->project);
  chipnomadInitChips(exporter->chipnomadState, sampleRate, NULL);
  chipnomadSetQuality(exporter->chipnomadState, CHIPNOMAD_QUALITY_BEST);

  playbackStartSong(&exporter->chipnomadState->playbackState, startRow, 0, 0);
  for (int t = 0; t < PROJECT_MAX_TRACKS; t++) {
    exporter->chipnomadState->playbackState.trackEnabled[t] = (t == 0) ? 1 : 0;
  }

  exporter->data = data;
  exporter->next = wavStemsNext;
  exporter->finish = wavStemsFinish;
  exporter->cancel = wavStemsCancel;

  return exporter;
}

