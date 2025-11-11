#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <corelib_file.h>
#include <playback.h>
#include <chips.h>

static void writePSGHeader(int fileId) {
  char header[16] = "PSG\x1a\0\0\0\0\0\0\0\0\0\0\0\0";
  fileWrite(fileId, header, 16);
}

PSGExporter* psgExportStart(const char* filename, int tickRate) {
  PSGExporter* exporter = malloc(sizeof(PSGExporter));
  if (!exporter) return NULL;

  exporter->fileId = fileOpen(filename, 1);
  if (exporter->fileId == -1) {
    free(exporter);
    return NULL;
  }

  writePSGHeader(exporter->fileId);
  return exporter;
}



int psgExportFinish(PSGExporter* exporter) {
  if (!exporter || exporter->fileId == -1) return -1;

  fileClose(exporter->fileId);
  free(exporter);
  return 0;
}

// PSG recording chip implementation
typedef struct {
  PSGExporter* exporter;
  uint8_t lastRegs[14];
} PSGChipData;

static int psgChipInit(struct SoundChip* self) {
  PSGChipData* data = (PSGChipData*)self->userdata;
  for (int i = 0; i < 14; i++) {
    data->lastRegs[i] = 0;
  }
  data->lastRegs[7] = 0x3f; // Default mixer value
  return 0;
}

static void psgChipSetRegister(struct SoundChip* self, uint16_t reg, uint8_t value) {
  if (reg > 13) return;

  PSGChipData* data = (PSGChipData*)self->userdata;
  self->regs[reg] = value;

  // Only write if value changed (except reg 13 - envelope shape always writes)
  if (data->lastRegs[reg] != value || reg == 13) {
    uint8_t regData[2] = {reg, value};
    fileWrite(data->exporter->fileId, regData, 2);
    data->lastRegs[reg] = value;
  }
}

static void psgChipRender(struct SoundChip* self, float* buffer, int samples) {
  // No audio rendering for PSG export
  for (int i = 0; i < samples * 2; i++) {
    buffer[i] = 0.0f;
  }
}

static int psgChipCleanup(struct SoundChip* self) {
  free(self->userdata);
  return 0;
}

static struct SoundChip createPSGChip(PSGExporter* exporter) {
  PSGChipData* data = malloc(sizeof(PSGChipData));
  data->exporter = exporter;

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

int exportProjectToPSG(const char* filename, struct Project* project, int startRow) {
  PSGExporter* exporter = psgExportStart(filename, (int)project->tickRate);
  if (!exporter) return -1;

  // Initialize playback
  struct PlaybackState playbackState;
  playbackInit(&playbackState, project);

  // Create PSG recording chip
  struct SoundChip chip = createPSGChip(exporter);
  chip.init(&chip);

  // Start song playback without looping
  playbackStartSong(&playbackState, startRow, 0, 0);

  int allTracksStopped = 0;
  do {
    // Write frame marker
    uint8_t frameMarker = 0xFF;
    fileWrite(exporter->fileId, &frameMarker, 1);

    allTracksStopped = playbackNextFrame(&playbackState, &chip);
  } while (!allTracksStopped);

  chip.cleanup(&chip);
  return psgExportFinish(exporter);
}