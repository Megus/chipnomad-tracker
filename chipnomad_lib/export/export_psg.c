#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "corelib/corelib_file.h"
#include "playback.h"
#include "chips/chips.h"
#include "chipnomad_lib.h"

// PSG exporter state
typedef struct {
  int fileId;
  ChipNomadState* chipnomadState;
  int allTracksStopped;
  int renderedSeconds;
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

static int psgChipInit(SoundChip* self) {
  PSGChipData* data = (PSGChipData*)self->userdata;
  for (int i = 0; i < 14; i++) {
    data->lastRegs[i] = 0;
  }
  data->lastRegs[7] = 0x3f;
  return 0;
}

static void psgChipSetRegister(SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg > 13) return;

  PSGChipData* data = (PSGChipData*)self->userdata;
  self->regs[reg] = value;

  if (data->lastRegs[reg] != value || reg == 13) {
    uint8_t regData[2] = {reg, value};
    fileWrite(data->fileId, regData, 2);
    data->lastRegs[reg] = value;
  }
}

static void psgChipRender(SoundChip* self, float* buffer, int samples) {
  for (int i = 0; i < samples * 2; i++) {
    buffer[i] = 0.0f;
  }
}

static int psgChipCleanup(SoundChip* self) {
  free(self->userdata);
  return 0;
}

// File ID for PSG factory
static int psgFileId;

static SoundChip psgChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  PSGChipData* data = malloc(sizeof(PSGChipData));
  data->fileId = psgFileId;

  SoundChip chip = {
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

  int framesPerChunk = (int)(data->chipnomadState->project.tickRate * 10 + 0.5f); // 10 seconds
  int framesRendered = 0;

  while (framesRendered < framesPerChunk && !data->allTracksStopped) {
    uint8_t frameMarker = 0xFF;
    fileWrite(data->fileId, &frameMarker, 1);

    data->allTracksStopped = playbackNextFrame(&data->chipnomadState->playbackState, data->chipnomadState->chips);
    framesRendered++;
  }

  if (data->allTracksStopped) return -1;

  data->renderedSeconds += 10;
  return data->renderedSeconds;
}

static int psgFinish(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;

  chipnomadDestroy(data->chipnomadState);
  fileClose(data->fileId);
  free(data);
  free(self);
  return 0;
}

static void psgCancel(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;

  chipnomadDestroy(data->chipnomadState);
  fileClose(data->fileId);
  fileDelete(data->filename);
  free(data);
  free(self);
}

Exporter* createPSGExporter(const char* filename, Project* project, int startRow) {
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
  strncpy(data->filename, filename, sizeof(data->filename) - 1);
  data->filename[sizeof(data->filename) - 1] = 0;

  // Create ChipNomad state and initialize
  data->chipnomadState = chipnomadCreate();
  if (!data->chipnomadState) {
    fileClose(data->fileId);
    free(data);
    free(exporter);
    return NULL;
  }

  // Copy project data and reinitialize playback
  data->chipnomadState->project = *project;
  playbackInit(&data->chipnomadState->playbackState, &data->chipnomadState->project);

  // Set up PSG factory and create chip
  psgFileId = data->fileId;
  chipnomadInitChips(data->chipnomadState, 44100, psgChipFactory); // Sample rate doesn't matter for PSG
  SoundChip* chip = &data->chipnomadState->chips[0];
  if (chip->init) {
    chip->init(chip);
  }
  playbackStartSong(&data->chipnomadState->playbackState, startRow, 0, 0);

  exporter->data = data;
  exporter->next = psgNext;
  exporter->finish = psgFinish;
  exporter->cancel = psgCancel;

  return exporter;
}