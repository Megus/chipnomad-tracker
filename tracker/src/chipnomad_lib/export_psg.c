#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <corelib_file.h>
#include <playback.h>
#include <chips.h>

// PSG implementation data
typedef struct {
    int fileId;
    struct PlaybackState playbackState;
    struct SoundChip chip;
    int allTracksStopped;
    int renderedSeconds;
    float tickRate;
    char filename[1024];
} PSGExporterData;

static void writePSGHeader(int fileId) {
  char header[16] = "PSG\x1a\0\0\0\0\0\0\0\0\0\0\0\0";
  fileWrite(fileId, header, 16);
}

// PSG recording chip implementation
typedef struct {
  int fileId;
  uint8_t lastRegs[14];
} PSGChipData;

static int psgChipInit(struct SoundChip* self) {
  PSGChipData* data = (PSGChipData*)self->userdata;
  for (int i = 0; i < 14; i++) {
    data->lastRegs[i] = 0;
  }
  data->lastRegs[7] = 0x3f;
  return 0;
}

static void psgChipSetRegister(struct SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg > 13) return;

  PSGChipData* data = (PSGChipData*)self->userdata;
  self->regs[reg] = value;

  if (data->lastRegs[reg] != value || reg == 13) {
    uint8_t regData[2] = {reg, value};
    fileWrite(data->fileId, regData, 2);
    data->lastRegs[reg] = value;
  }
}

static void psgChipRender(struct SoundChip* self, float* buffer, int samples) {
  for (int i = 0; i < samples * 2; i++) {
    buffer[i] = 0.0f;
  }
}

static int psgChipCleanup(struct SoundChip* self) {
  free(self->userdata);
  return 0;
}

static struct SoundChip createPSGChip(int fileId) {
  PSGChipData* data = malloc(sizeof(PSGChipData));
  data->fileId = fileId;

  struct SoundChip chip = {
    .userdata = data,
    .init = psgChipInit,
    .setRegister = psgChipSetRegister,
    .render = psgChipRender,
    .cleanup = psgChipCleanup,
  };

  for (int i = 0; i < 256; i++) {
    chip.regs[i] = 0;
  }
  chip.regs[7] = 0x3f;

  return chip;
}

// PSG exporter methods
static int psgNext(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;
  if (data->allTracksStopped) return -1;

  int framesPerChunk = (int)(data->tickRate * 10 + 0.5f); // 10 seconds
  int framesRendered = 0;

  while (framesRendered < framesPerChunk && !data->allTracksStopped) {
    uint8_t frameMarker = 0xFF;
    fileWrite(data->fileId, &frameMarker, 1);
    
    data->allTracksStopped = playbackNextFrame(&data->playbackState, &data->chip);
    framesRendered++;
  }

  if (data->allTracksStopped) return -1;
  
  data->renderedSeconds += 10;
  return data->renderedSeconds;
}

static int psgFinish(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;
  
  data->chip.cleanup(&data->chip);
  fileClose(data->fileId);
  free(data);
  free(self);
  return 0;
}

static void psgCancel(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;
  
  data->chip.cleanup(&data->chip);
  fileClose(data->fileId);
  fileDelete(data->filename);
  free(data);
  free(self);
}

Exporter* createPSGExporter(const char* filename, struct Project* project, int startRow) {
  Exporter* exporter = malloc(sizeof(Exporter));
  if (!exporter) return NULL;

  PSGExporterData* data = malloc(sizeof(PSGExporterData));
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

  writePSGHeader(data->fileId);
  
  data->allTracksStopped = 0;
  data->renderedSeconds = 0;
  data->tickRate = project->tickRate;
  strncpy(data->filename, filename, sizeof(data->filename) - 1);
  data->filename[sizeof(data->filename) - 1] = 0;
  
  playbackInit(&data->playbackState, project);
  data->chip = createPSGChip(data->fileId);
  data->chip.init(&data->chip);
  playbackStartSong(&data->playbackState, startRow, 0, 0);

  exporter->data = data;
  exporter->next = psgNext;
  exporter->finish = psgFinish;
  exporter->cancel = psgCancel;

  return exporter;
}