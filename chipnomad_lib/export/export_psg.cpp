#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "playback.h"

static void writePSGHeader(FILE* file) {
  const char header[17] = "PSG\x1a\0\0\0\0\0\0\0\0\0\0\0\0";
  fwrite(header, 16, 1, file);
}

// PSG recording chip implementation
class SoundChipPSG : public SoundChip {
  private:
    FILE* file;
    uint8_t lastRegs[16];
    uint8_t regs[16];

  public:
    SoundChipPSG(FILE* f) : file(f) {
      memset(regs, 0, sizeof(regs));
      regs[7] = 0x3f;
      memset(lastRegs, 0, sizeof(lastRegs));
      lastRegs[7] = 0x3f;
    }

    ~SoundChipPSG() override {}

    void setRegister(uint16_t reg, uint8_t value) override {
      if (reg > 13) return;
      regs[reg] = value;

      if (lastRegs[reg] != value || reg == 13) {
        uint8_t regData[2] = {(uint8_t)reg, value};
        fwrite(regData, 2, 1, file);
        lastRegs[reg] = value;
      }
    }

    uint8_t getRegister(uint16_t reg) override {
      if (reg > 13) return 0;
      return regs[reg];
    }
};

// File pointers for PSG factory
static FILE* psgFiles[3];

static SoundChip* psgChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  return new SoundChipPSG(psgFiles[chipIndex]);
}

///////////////////////////////////////////////////////////////////////////////
// ExporterPSG
///////////////////////////////////////////////////////////////////////////////

ExporterPSG::ExporterPSG(const char* filename, Project* project, int startRow)
  : Exporter(project, startRow) {
  numChips = project->chipsCount;

  // Extract base filename (remove .psg extension if present)
  strncpy(baseFilename, filename, sizeof(baseFilename) - 1);
  baseFilename[sizeof(baseFilename) - 1] = 0;
  char* ext = strstr(baseFilename, ".psg");
  if (ext) *ext = 0;

  // Open files for each chip
  for (int i = 0; i < numChips; i++) {
    char chipFilename[1024];
    if (numChips > 1) {
      snprintf(chipFilename, sizeof(chipFilename), "%s-%d.psg", baseFilename, i + 1);
    } else {
      snprintf(chipFilename, sizeof(chipFilename), "%s.psg", baseFilename);
    }
    files[i] = fopen(chipFilename, "wb");
    if (files[i]) {
      writePSGHeader(files[i]);
    }
  }

  // Set up PSG chip factory
  for (int i = 0; i < numChips; i++) {
    psgFiles[i] = files[i];
  }
  chipnomadInitChips(chipnomadState, 44100, psgChipFactory);
}

int ExporterPSG::next() {
  int framesPerChunk = (int)(chipnomadState->project.tickRate * 10 + 0.5f); // 10 seconds
  int framesRendered = 0;
  int done = 0;

  while (framesRendered < framesPerChunk && !done) {
    uint8_t frameMarker = 0xFF;
    for (int i = 0; i < numChips; i++) {
      fwrite(&frameMarker, 1, 1, files[i]);
    }

    done = playbackNextFrame(chipnomadState);
    framesRendered++;
  }

  if (done) return -1;

  renderedSeconds += 10;
  return renderedSeconds;
}

int ExporterPSG::finish() {
  for (int i = 0; i < numChips; i++) {
    if (files[i]) {
      fclose(files[i]);
      files[i] = NULL;
    }
  }
  return 0;
}

void ExporterPSG::cancel() {
  for (int i = 0; i < numChips; i++) {
    if (files[i]) {
      fclose(files[i]);
      files[i] = NULL;

      char filename[1024];
      if (numChips > 1) {
        snprintf(filename, sizeof(filename), "%s-%d.psg", baseFilename, i + 1);
      } else {
        snprintf(filename, sizeof(filename), "%s.psg", baseFilename);
      }
      remove(filename);
    }
  }
}
