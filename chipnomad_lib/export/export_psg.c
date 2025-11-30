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
  int fileIds[3];
  int numChips;
  int allTracksStopped;
  int renderedSeconds;
  char baseFilename[1024];
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

// File IDs for PSG factory
static int psgFileIds[3];

static SoundChip psgChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  PSGChipData* data = malloc(sizeof(PSGChipData));
  data->fileId = psgFileIds[chipIndex];

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

  int framesPerChunk = (int)(self->chipnomadState->project.tickRate * 10 + 0.5f); // 10 seconds
  int framesRendered = 0;

  while (framesRendered < framesPerChunk && !data->allTracksStopped) {
    uint8_t frameMarker = 0xFF;
    for (int i = 0; i < data->numChips; i++) {
      fileWrite(data->fileIds[i], &frameMarker, 1);
    }

    data->allTracksStopped = playbackNextFrame(&self->chipnomadState->playbackState, self->chipnomadState->chips);
    framesRendered++;
  }

  if (data->allTracksStopped) return -1;

  data->renderedSeconds += 10;
  return data->renderedSeconds;
}

static int psgFinish(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;

  chipnomadDestroy(self->chipnomadState);
  for (int i = 0; i < data->numChips; i++) {
    fileClose(data->fileIds[i]);
  }
  free(data);
  free(self);
  return 0;
}

static void psgCancel(Exporter* self) {
  PSGExporterData* data = (PSGExporterData*)self->data;

  chipnomadDestroy(self->chipnomadState);
  for (int i = 0; i < data->numChips; i++) {
    fileClose(data->fileIds[i]);
    
    char filename[1024];
    if (data->numChips > 1) {
      snprintf(filename, sizeof(filename), "%s-%d.psg", data->baseFilename, i + 1);
    } else {
      snprintf(filename, sizeof(filename), "%s.psg", data->baseFilename);
    }
    fileDelete(filename);
  }
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

  data->numChips = project->chipsCount;
  data->allTracksStopped = 0;
  data->renderedSeconds = 0;

  // Extract base filename (remove .psg extension if present)
  strncpy(data->baseFilename, filename, sizeof(data->baseFilename) - 1);
  data->baseFilename[sizeof(data->baseFilename) - 1] = 0;
  char* ext = strstr(data->baseFilename, ".psg");
  if (ext) *ext = 0;

  // Open files for each chip
  for (int i = 0; i < data->numChips; i++) {
    char chipFilename[1024];
    if (data->numChips > 1) {
      snprintf(chipFilename, sizeof(chipFilename), "%s-%d.psg", data->baseFilename, i + 1);
    } else {
      snprintf(chipFilename, sizeof(chipFilename), "%s.psg", data->baseFilename);
    }

    data->fileIds[i] = fileOpen(chipFilename, 1);
    if (data->fileIds[i] == -1) {
      // Close previously opened files
      for (int j = 0; j < i; j++) {
        fileClose(data->fileIds[j]);
      }
      free(data);
      free(exporter);
      return NULL;
    }
    writePSGHeader(data->fileIds[i]);
  }

  // Create ChipNomad state and initialize
  exporter->chipnomadState = chipnomadCreate();
  if (!exporter->chipnomadState) {
    for (int i = 0; i < data->numChips; i++) {
      fileClose(data->fileIds[i]);
    }
    free(data);
    free(exporter);
    return NULL;
  }

  // Copy project data and reinitialize playback
  exporter->chipnomadState->project = *project;
  playbackInit(&exporter->chipnomadState->playbackState, &exporter->chipnomadState->project);

  // Set up PSG factory and create chips
  for (int i = 0; i < data->numChips; i++) {
    psgFileIds[i] = data->fileIds[i];
  }
  chipnomadInitChips(exporter->chipnomadState, 44100, psgChipFactory); // Sample rate doesn't matter for PSG
  for (int i = 0; i < data->numChips; i++) {
    SoundChip* chip = &exporter->chipnomadState->chips[i];
    if (chip->init) {
      chip->init(chip);
    }
  }
  playbackStartSong(&exporter->chipnomadState->playbackState, startRow, 0, 0);

  exporter->data = data;
  exporter->next = psgNext;
  exporter->finish = psgFinish;
  exporter->cancel = psgCancel;

  return exporter;
}