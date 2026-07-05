#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "playback.h"

// VGM header size (version 1.51+, AY uses offset 0x74)
#define VGM_HEADER_SIZE 0x80

// VGM header offsets
#define VGM_EOF_OFFSET        0x04  // End of file offset (relative to 0x04)
#define VGM_VERSION           0x08  // VGM version (BCD)
#define VGM_AY_CLOCK          0x74  // AY8910 clock
#define VGM_AY_TYPE           0x78  // AY8910 chip type
#define VGM_AY_FLAGS          0x79  // AY8910 flags
#define VGM_TOTAL_SAMPLES     0x18  // Total # samples (at 44100Hz)
#define VGM_RATE              0x24  // Recording rate (Hz)
#define VGM_DATA_OFFSET       0x34  // VGM data offset (relative to 0x34)

// Register dump chip implementation for VGM export
// Simulates chip clock to call timer function at correct rate
class SoundChipRegDump : public SoundChip {
  private:
    uint8_t lastRegs[256];
    uint8_t regs[256];
    float step;   // Timer ticks per audio sample (clock/divider/sampleRate)
    float x;      // Fractional accumulator

  public:
    SoundChipRegDump(int sampleRate, int clockRate, int clockDivider) : x(0.0f) {
      memset(regs, 0, sizeof(regs));
      memset(lastRegs, 0, sizeof(lastRegs));
      // Timer fires at clock/divider rate; step = how many timer ticks per audio sample
      step = (float)clockRate / (float)clockDivider / (float)sampleRate;
    }

    ~SoundChipRegDump() override {}

    void setRegister(uint16_t reg, uint8_t value) override {
      if (reg > 255) return;
      regs[reg] = value;

      // AY needs to explicitly capture reg 13 writes even if the value is the same
      if (reg == 13) {
        lastRegs[reg] = value + 1; // Force a change to ensure it's captured
      }
    }

    uint8_t getRegister(uint16_t reg) override {
      if (reg > 255) return 0;
      return regs[reg];
    }

    uint8_t getLastRegister(uint16_t reg) {
      if (reg > 255) return 0;
      return lastRegs[reg];
    }

    void consolidateRegisters() {
      memcpy(lastRegs, regs, sizeof(regs));
    }

    // Advance by one audio sample, calling timer at clock/divider rate
    void render(float* buffer, int samples) override {
      for (int i = 0; i < samples; i++) {
        x += step;
        while (x >= 1.0f) {
          x -= 1.0f;
          if (timerFunc) {
            timerFunc(this, timerUserdata);
          }
        }
        buffer[i * 2] = 0.0f;
        buffer[i * 2 + 1] = 0.0f;
      }
    }

    void setQuality(ChipNomadQuality quality) override {}
};

static SoundChip* regDumpChipFactory(int chipIndex, int sampleRate, ChipSetup setup) {
  return new SoundChipRegDump(sampleRate, setup.ay.clock, 8);  // AY clock divider = 8
}

///////////////////////////////////////////////////////////////////////////////
// ExporterVGM
///////////////////////////////////////////////////////////////////////////////

ExporterVGM::ExporterVGM(const char* filename, Project* project, int startRow)
  : Exporter(project, startRow) {
  waitSamples = 0;
  totalSamples = 0;
  strncpy(baseFilename, filename, sizeof(baseFilename) - 1);
  baseFilename[sizeof(baseFilename) - 1] = 0;

  file = fopen(filename, "wb");
  if (file) {
    // Write placeholder header (will be finalized in finish())
    uint8_t header[VGM_HEADER_SIZE];
    memset(header, 0, sizeof(header));
    fwrite(header, 1, VGM_HEADER_SIZE, file);
  }

  chipnomadInitChips(chipnomadState, 44100, regDumpChipFactory);
}

int ExporterVGM::next() {
  if (!file) return -1;

  int chipCount = chipnomadState->project.chipsCount;
  if (chipCount > 2) chipCount = 2;

  float dummyBuffer[2];

  for (int s = 0; s < 44100 * 10; s++) {
    int rendered = chipnomadRender(chipnomadState, dummyBuffer, 1);

    if (rendered < 1) {
      // Playback ended
      if (waitSamples > 0) {
        writeWait();
      }
      uint8_t endCmd = 0x66;
      fwrite(&endCmd, 1, 1, file);
      return -1;
    }

    // Check for register changes across all chips
    bool hasChanges = false;
    for (int c = 0; c < chipCount && !hasChanges; c++) {
      SoundChipRegDump* chip = static_cast<SoundChipRegDump*>(chipnomadState->chips[c]);
      for (int r = 0; r < 14; r++) {
        if (chip->getLastRegister(r) != chip->getRegister(r)) {
          hasChanges = true;
          break;
        }
      }
    }

    if (hasChanges) {
      if (waitSamples > 0) {
        writeWait();
      }

      for (int c = 0; c < chipCount; c++) {
        SoundChipRegDump* chip = static_cast<SoundChipRegDump*>(chipnomadState->chips[c]);
        for (int r = 0; r < 14; r++) {
          if (chip->getLastRegister(r) != chip->getRegister(r)) {
            uint8_t regData[3] = { 0xa0, (uint8_t)(r + c * 0x80), chip->getRegister(r) };
            fwrite(regData, 3, 1, file);
          }
        }
        chip->consolidateRegisters();
      }
    }

    waitSamples++;
    totalSamples++;
  }

  renderedSeconds += 10;
  return renderedSeconds;
}

int ExporterVGM::finish() {
  if (!file) return 1;

  // Write finalized VGM header
  long fileSize = ftell(file);

  fseek(file, 0, SEEK_SET);
  uint8_t header[VGM_HEADER_SIZE];
  memset(header, 0, sizeof(header));

  // "Vgm " ident
  header[0] = 'V'; header[1] = 'g'; header[2] = 'm'; header[3] = ' ';

  // EOF offset (relative to offset 0x04)
  uint32_t eofOffset = (uint32_t)(fileSize - 4);
  memcpy(&header[VGM_EOF_OFFSET], &eofOffset, 4);

  // Version 1.51 (0x00000151 in BCD)
  uint32_t version = 0x00000151;
  memcpy(&header[VGM_VERSION], &version, 4);

  // Total samples
  uint32_t samples = (uint32_t)totalSamples;
  memcpy(&header[VGM_TOTAL_SAMPLES], &samples, 4);

  // Rate (tick rate in Hz)
  uint32_t rate = (uint32_t)(chipnomadState->project.tickRate + 0.5f);
  memcpy(&header[VGM_RATE], &rate, 4);

  // VGM data offset (relative to 0x34) - data starts at VGM_HEADER_SIZE
  uint32_t dataOffset = VGM_HEADER_SIZE - 0x34;
  memcpy(&header[VGM_DATA_OFFSET], &dataOffset, 4);

  // AY8910 clock (bit 30 set = dual chip)
  uint32_t ayClock = (uint32_t)chipnomadState->project.chipSetup.ay.clock;
  if (chipnomadState->project.chipsCount > 1) {
    ayClock |= 0x40000000; // Dual chip flag
  }
  memcpy(&header[VGM_AY_CLOCK], &ayClock, 4);

  // AY8910 chip type: 0x00 = AY8910, 0x01 = AY8912, 0x02 = AY8913, 0x03 = YM2149
  header[VGM_AY_TYPE] = chipnomadState->project.chipSetup.ay.isYM ? 0x03 : 0x00;

  // AY8910 flags: 0x01 = legacy output
  header[VGM_AY_FLAGS] = 0x01;

  fwrite(header, 1, VGM_HEADER_SIZE, file);

  fclose(file);
  file = NULL;
  return 0;
}

void ExporterVGM::cancel() {
  if (file) {
    fclose(file);
    file = NULL;
    remove(baseFilename);
  }
}

void ExporterVGM::writeWait() {
  while (waitSamples > 0) {
    if (waitSamples == 735) {
      // Wait exactly 1/60 second
      uint8_t cmd = 0x62;
      fwrite(&cmd, 1, 1, file);
      waitSamples = 0;
    } else if (waitSamples == 882) {
      // Wait exactly 1/50 second
      uint8_t cmd = 0x63;
      fwrite(&cmd, 1, 1, file);
      waitSamples = 0;
    } else if (waitSamples <= 16) {
      // Short wait (0x7n = wait n+1 samples)
      uint8_t cmd = 0x70 + (uint8_t)(waitSamples - 1);
      fwrite(&cmd, 1, 1, file);
      waitSamples = 0;
    } else if (waitSamples <= 65535) {
      // Generic wait (0x61 nnnn)
      uint8_t cmd[3] = { 0x61, (uint8_t)(waitSamples & 0xFF), (uint8_t)((waitSamples >> 8) & 0xFF) };
      fwrite(cmd, 3, 1, file);
      waitSamples = 0;
    } else {
      // Wait more than 65535 samples - split into multiple waits
      uint8_t cmd[3] = { 0x61, 0xFF, 0xFF };
      fwrite(cmd, 3, 1, file);
      waitSamples -= 65535;
    }
  }
}
