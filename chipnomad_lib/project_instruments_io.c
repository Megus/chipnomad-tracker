#include "project.h"
#include "project_io_common.h"
#include <stdio.h>
#include <string.h>

// Load AY1 instrument data (legacy format - version 1.0)
static int loadInstrumentAY1Legacy(FILE* file, Instrument* instrument) {
  while (1) {
    char* line = peekLine(file);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

    if (strncmp(line, "- Volume envelope: ", 19) == 0) {
      // Read ADSR values into modulation struct
      Modulation* ve = &instrument->chip.ay.volumeEnvelope;
      ve->type = modADSR;
      ve->destination = 1;
      ve->amount = 127;
      sscanf(line, "- Volume envelope: %hhu,%hhu,%hhu,%hhu",
        &ve->p1, &ve->p2, &ve->p3, &ve->p4);  // A, D, S, R
    } else if (strncmp(line, "- Auto envelope: ", 17) == 0) {
      sscanf(line, "- Auto envelope: %hhu,%hhu",
        &instrument->chip.ay.autoEnvN, &instrument->chip.ay.autoEnvD);
    } else if (strncmp(line, "- Default mixer: ", 17) == 0) {
      sscanf(line, "- Default mixer: %hhX", &instrument->chip.ay.defaultMixer);
    }
    consumeLine(file);
  }
}

// Load AY1 instrument data (new format - version 2.0)
static int loadInstrumentAY1(FILE* file, Instrument* instrument) {
  while (1) {
    char* line = peekLine(file);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

    if (strncmp(line, "- Volume envelope: ", 19) == 0) {
      // Read ADSR values into modulation struct
      Modulation* ve = &instrument->chip.ay.volumeEnvelope;
      ve->type = modADSR;
      ve->destination = 1;
      ve->amount = 127;
      sscanf(line, "- Volume envelope: %hhu,%hhu,%hhu,%hhu",
        &ve->p1, &ve->p2, &ve->p3, &ve->p4);  // A, D, S, R
    } else if (strncmp(line, "- Auto envelope: ", 17) == 0) {
      sscanf(line, "- Auto envelope: %hhu,%hhu",
        &instrument->chip.ay.autoEnvN, &instrument->chip.ay.autoEnvD);
    } else if (strncmp(line, "- Default mixer: ", 17) == 0) {
      sscanf(line, "- Default mixer: %hhX", &instrument->chip.ay.defaultMixer);
    }
    consumeLine(file);
  }
}

// Load AY2 instrument data
static int loadInstrumentAY2(FILE* file, Instrument* instrument) {
  while (1) {
    char* line = peekLine(file);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

    // Tone oscillator
    if (strncmp(line, "- Tone on: ", 11) == 0) {
      sscanf(line, "- Tone on: %hhu", &instrument->chip.ay2.oscTone.isOn);
    } else if (strncmp(line, "- Tone pitch flag: ", 19) == 0) {
      sscanf(line, "- Tone pitch flag: %hhu", &instrument->chip.ay2.oscTone.pitchFlag);
    } else if (strncmp(line, "- Tone pitch offset: ", 21) == 0) {
      sscanf(line, "- Tone pitch offset: %hhd", &instrument->chip.ay2.oscTone.pitchOffset);
    } else if (strncmp(line, "- Tone fine tune: ", 18) == 0) {
      sscanf(line, "- Tone fine tune: %hhd", &instrument->chip.ay2.oscTone.fineTune);
    }
    // Noise oscillator
    else if (strncmp(line, "- Noise on: ", 12) == 0) {
      sscanf(line, "- Noise on: %hhu", &instrument->chip.ay2.oscNoise.isOn);
    } else if (strncmp(line, "- Noise period: ", 16) == 0) {
      sscanf(line, "- Noise period: %hhu", &instrument->chip.ay2.oscNoise.noisePeriod);
    }
    // Envelope oscillator
    else if (strncmp(line, "- Envelope shape: ", 18) == 0) {
      sscanf(line, "- Envelope shape: %hhu", &instrument->chip.ay2.oscEnvelope.shape);
    } else if (strncmp(line, "- Envelope auto N: ", 19) == 0) {
      sscanf(line, "- Envelope auto N: %hhu", &instrument->chip.ay2.oscEnvelope.autoEnvN);
    } else if (strncmp(line, "- Envelope auto D: ", 19) == 0) {
      sscanf(line, "- Envelope auto D: %hhu", &instrument->chip.ay2.oscEnvelope.autoEnvD);
    } else if (strncmp(line, "- Envelope pitch flag: ", 23) == 0) {
      sscanf(line, "- Envelope pitch flag: %hhu", &instrument->chip.ay2.oscEnvelope.pitchFlag);
    } else if (strncmp(line, "- Envelope pitch offset: ", 25) == 0) {
      sscanf(line, "- Envelope pitch offset: %hhd", &instrument->chip.ay2.oscEnvelope.pitchOffset);
    } else if (strncmp(line, "- Envelope fine tune: ", 22) == 0) {
      sscanf(line, "- Envelope fine tune: %hhd", &instrument->chip.ay2.oscEnvelope.fineTune);
    }
    // Software oscillator
    else if (strncmp(line, "- Software type: ", 17) == 0) {
      sscanf(line, "- Software type: %hhu", (uint8_t*)&instrument->chip.ay2.oscSoftware.type);
    } else if (strncmp(line, "- Software pitch flag: ", 23) == 0) {
      sscanf(line, "- Software pitch flag: %hhu", &instrument->chip.ay2.oscSoftware.pitchFlag);
    } else if (strncmp(line, "- Software pitch offset: ", 25) == 0) {
      sscanf(line, "- Software pitch offset: %hhd", &instrument->chip.ay2.oscSoftware.pitchOffset);
    } else if (strncmp(line, "- Software fine tune: ", 22) == 0) {
      sscanf(line, "- Software fine tune: %hhd", &instrument->chip.ay2.oscSoftware.fineTune);
    } else if (strncmp(line, "- Pulse Width: ", 15) == 0) {
      sscanf(line, "- Pulse Width: %hhu", &instrument->chip.ay2.oscSoftware.pulseWidth);
    } else if (strncmp(line, "- Pulse Low: ", 13) == 0) {
      sscanf(line, "- Pulse Low: %hhu", &instrument->chip.ay2.oscSoftware.pulseLow);
    } else if (strncmp(line, "- Wavetable Index: ", 19) == 0) {
      sscanf(line, "- Wavetable Index: %hhu", &instrument->chip.ay2.oscSoftware.wavetableIndex);
    }
    consumeLine(file);
  }
}

// Load AYSample instrument data
static int loadInstrumentAYSample(FILE* file, Instrument* instrument) {
  while (1) {
    char* line = peekLine(file);
    if (line == NULL) break;
    if (line[0] == '#') break;

    // Tone oscillator
    if (strncmp(line, "- Tone on: ", 11) == 0) {
      sscanf(line, "- Tone on: %hhu", &instrument->chip.aySample.oscTone.isOn);
    } else if (strncmp(line, "- Tone pitch flag: ", 19) == 0) {
      sscanf(line, "- Tone pitch flag: %hhu", &instrument->chip.aySample.oscTone.pitchFlag);
    } else if (strncmp(line, "- Tone pitch offset: ", 21) == 0) {
      sscanf(line, "- Tone pitch offset: %hhd", &instrument->chip.aySample.oscTone.pitchOffset);
    } else if (strncmp(line, "- Tone fine tune: ", 18) == 0) {
      sscanf(line, "- Tone fine tune: %hhd", &instrument->chip.aySample.oscTone.fineTune);
    }
    // Noise oscillator
    else if (strncmp(line, "- Noise on: ", 12) == 0) {
      sscanf(line, "- Noise on: %hhu", &instrument->chip.aySample.oscNoise.isOn);
    } else if (strncmp(line, "- Noise period: ", 16) == 0) {
      sscanf(line, "- Noise period: %hhu", &instrument->chip.aySample.oscNoise.noisePeriod);
    }
    // Sample parameters
    else if (strncmp(line, "- Sample name: ", 15) == 0) {
      sscanf(line, "- Sample name: %[^\n]", instrument->chip.aySample.sampleName);
    } else if (strncmp(line, "- Sample rate: ", 15) == 0) {
      sscanf(line, "- Sample rate: %hu", &instrument->chip.aySample.sampleRate);
    } else if (strncmp(line, "- Sample start: ", 16) == 0) {
      sscanf(line, "- Sample start: %hX", &instrument->chip.aySample.sampleStart);
    } else if (strncmp(line, "- Sample length: ", 17) == 0) {
      sscanf(line, "- Sample length: %hX", &instrument->chip.aySample.sampleLength);
    } else if (strncmp(line, "- Sample loop start: ", 21) == 0) {
      sscanf(line, "- Sample loop start: %hX", &instrument->chip.aySample.sampleLoopStart);
    } else if (strncmp(line, "- Sample pitch offset: ", 23) == 0) {
      sscanf(line, "- Sample pitch offset: %hhd", &instrument->chip.aySample.pitchOffset);
    } else if (strncmp(line, "- Sample fine tune: ", 20) == 0) {
      sscanf(line, "- Sample fine tune: %hhd", &instrument->chip.aySample.fineTune);
    }

    consumeLine(file);
  }

  // Check for #### Sample Data section
  char* line = peekLine(file);
  if (line != NULL && strncmp(line, "#### Sample Data", 16) == 0) {
    consumeLine(file);  // Consume "#### Sample Data"

    // Read "- Length:" line
    line = peekLine(file);
    if (line != NULL && strncmp(line, "- Length: ", 10) == 0) {
      uint16_t dataLen = 0;
      sscanf(line, "- Length: %hX", &dataLen);
      consumeLine(file);

      // Validate length
      if (dataLen > PROJECT_MAX_SAMPLE_SIZE) {
        dataLen = PROJECT_MAX_SAMPLE_SIZE;
      }
      instrument->chip.aySample.fileLength = dataLen;

      // Read "- Data:" line
      line = peekLine(file);
      if (line != NULL && strncmp(line, "- Data:", 7) == 0) {
        consumeLine(file);

        // Load binary data
        if (dataLen > 0) {
          if (loadBinaryData(file, &instrument->chip.aySample.sampleData, &dataLen, PROJECT_MAX_SAMPLE_SIZE)) {
            return 1;
          }
        }
      }
    }
  }

  return 0;
}

// Load modulation data
static int loadModulation(FILE* file, Instrument* instrument) {
  for (int i = 0; i < 4; i++) {
    char* line = peekLine(file);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

    char modPrefix[32];
    sprintf(modPrefix, "- Mod%d: ", i + 1);

    if (strncmp(line, modPrefix, strlen(modPrefix)) == 0) {
      sscanf(line + strlen(modPrefix), "%hhu,%hhu,%hhd,%hhu,%hhu,%hhu,%hhu",
        (uint8_t*)&instrument->modulation[i].type,
        &instrument->modulation[i].destination,
        &instrument->modulation[i].amount,
        &instrument->modulation[i].p1,
        &instrument->modulation[i].p2,
        &instrument->modulation[i].p3,
        &instrument->modulation[i].p4);
    }
    consumeLine(file);
  }
  return 0;
}

// Main load function
int instrumentLoadData(FILE* file, Instrument* instrument, Project* p) {
  instrumentClear(instrument);

  // Read common fields first
  while (1) {
    char* line = peekLine(file);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

    if (strncmp(line, "- Name: ", 8) == 0) {
      sscanf(line, "- Name: %[^\n]", instrument->name);
    } else if (strncmp(line, "- Type: ", 8) == 0) {
      sscanf(line, "- Type: %hhd", &instrument->type);
    } else if (strncmp(line, "- Table speed: ", 15) == 0) {
      sscanf(line, "- Table speed: %hhu", &instrument->tableSpeed);
    } else if (strncmp(line, "- Transpose: ", 13) == 0) {
      sscanf(line, "- Transpose: %hhu", &instrument->transposeEnabled);
      consumeLine(file);
      // After reading transpose, check what comes next
      break;
    }
    consumeLine(file);
  }

  // Check version to determine format
  if (projectFileVersion == 1) {
    // Legacy format (version 1.0): no modulation, no "Chip data:" separator
    // Only AY1 instruments existed in version 1.0
    if (instrument->type == instAY1) {
      if (loadInstrumentAY1Legacy(file, instrument)) return 1;
    }
    return 0;
  }

  // New format (version 2.0): read modulation and chip data sections
  char* line = peekLine(file);
  if (line == NULL) return 1;

  if (strncmp(line, "- Modulation:", 13) == 0) {
    consumeLine(file);
    if (loadModulation(file, instrument)) return 1;
    line = peekLine(file);  // Read next line after modulation
    if (line == NULL) return 1;
  }

  if (strncmp(line, "- Chip data:", 12) == 0) {
    consumeLine(file);
    // Load chip-specific data based on instrument type
    switch (instrument->type) {
      case instAY1:
        if (loadInstrumentAY1(file, instrument)) return 1;
        break;
      case instAY2:
        if (loadInstrumentAY2(file, instrument)) return 1;
        break;
      case instAYSample:
        if (loadInstrumentAYSample(file, instrument)) return 1;
        break;
      default:
        break;
    }
  }

  return 0;
}

// Save AY1 instrument data
static int saveInstrumentAY1(FILE* file, Instrument* instrument) {
  // Save volume envelope as ADSR values (for backward compatibility in file format)
  Modulation* ve = &instrument->chip.ay.volumeEnvelope;
  fprintf(file, "- Volume envelope: %hhu,%hhu,%hhu,%hhu\n",
    ve->p1, ve->p2, ve->p3, ve->p4);  // A, D, S, R
  fprintf(file, "- Auto envelope: %hhd,%hhd\n",
    instrument->chip.ay.autoEnvN, instrument->chip.ay.autoEnvD);
  fprintf(file, "- Default mixer: %02X\n", instrument->chip.ay.defaultMixer);
  return 0;
}

// Save AY2 instrument data
static int saveInstrumentAY2(FILE* file, Instrument* instrument) {
  // Tone oscillator
  fprintf(file, "- Tone on: %hhu\n", instrument->chip.ay2.oscTone.isOn);
  fprintf(file, "- Tone pitch flag: %hhu\n", instrument->chip.ay2.oscTone.pitchFlag);
  fprintf(file, "- Tone pitch offset: %hhd\n", instrument->chip.ay2.oscTone.pitchOffset);
  fprintf(file, "- Tone fine tune: %hhd\n", instrument->chip.ay2.oscTone.fineTune);

  // Noise oscillator
  fprintf(file, "- Noise on: %hhu\n", instrument->chip.ay2.oscNoise.isOn);
  fprintf(file, "- Noise period: %hhu\n", instrument->chip.ay2.oscNoise.noisePeriod);

  // Envelope oscillator
  fprintf(file, "- Envelope shape: %hhu\n", instrument->chip.ay2.oscEnvelope.shape);
  fprintf(file, "- Envelope auto N: %hhu\n", instrument->chip.ay2.oscEnvelope.autoEnvN);
  fprintf(file, "- Envelope auto D: %hhu\n", instrument->chip.ay2.oscEnvelope.autoEnvD);
  fprintf(file, "- Envelope pitch flag: %hhu\n", instrument->chip.ay2.oscEnvelope.pitchFlag);
  fprintf(file, "- Envelope pitch offset: %hhd\n", instrument->chip.ay2.oscEnvelope.pitchOffset);
  fprintf(file, "- Envelope fine tune: %hhd\n", instrument->chip.ay2.oscEnvelope.fineTune);

  // Software oscillator
  fprintf(file, "- Software type: %hhu\n", instrument->chip.ay2.oscSoftware.type);
  fprintf(file, "- Software pitch flag: %hhu\n", instrument->chip.ay2.oscSoftware.pitchFlag);
  fprintf(file, "- Software pitch offset: %hhd\n", instrument->chip.ay2.oscSoftware.pitchOffset);
  fprintf(file, "- Software fine tune: %hhd\n", instrument->chip.ay2.oscSoftware.fineTune);
  fprintf(file, "- Pulse Width: %hhu\n", instrument->chip.ay2.oscSoftware.pulseWidth);
  fprintf(file, "- Pulse Low: %hhu\n", instrument->chip.ay2.oscSoftware.pulseLow);
  fprintf(file, "- Wavetable Index: %hhu\n", instrument->chip.ay2.oscSoftware.wavetableIndex);

  return 0;
}

// Save AYSample instrument data
static int saveInstrumentAYSample(FILE* file, Instrument* instrument) {
  // Tone oscillator
  fprintf(file, "- Tone on: %hhu\n", instrument->chip.aySample.oscTone.isOn);
  fprintf(file, "- Tone pitch flag: %hhu\n", instrument->chip.aySample.oscTone.pitchFlag);
  fprintf(file, "- Tone pitch offset: %hhd\n", instrument->chip.aySample.oscTone.pitchOffset);
  fprintf(file, "- Tone fine tune: %hhd\n", instrument->chip.aySample.oscTone.fineTune);

  // Noise oscillator
  fprintf(file, "- Noise on: %hhu\n", instrument->chip.aySample.oscNoise.isOn);
  fprintf(file, "- Noise period: %hhu\n", instrument->chip.aySample.oscNoise.noisePeriod);

  // Sample parameters
  fprintf(file, "- Sample name: %s\n", instrument->chip.aySample.sampleName);
  fprintf(file, "- Sample rate: %hu\n", instrument->chip.aySample.sampleRate);
  fprintf(file, "- Sample start: %04X\n", instrument->chip.aySample.sampleStart);
  fprintf(file, "- Sample length: %04X\n", instrument->chip.aySample.sampleLength);
  fprintf(file, "- Sample loop start: %04X\n", instrument->chip.aySample.sampleLoopStart);
  fprintf(file, "- Sample pitch offset: %hhd\n", instrument->chip.aySample.pitchOffset);
  fprintf(file, "- Sample fine tune: %hhd\n", instrument->chip.aySample.fineTune);

  // Save sample data as a separate #### section
  if (instrument->chip.aySample.fileLength > 0 && instrument->chip.aySample.sampleData != NULL) {
    fprintf(file, "\n#### Sample Data\n\n");
    saveBinaryData(file,
                  instrument->chip.aySample.sampleData,
                  instrument->chip.aySample.fileLength);
  }

  return 0;
}

// Save modulation data
static int saveModulation(FILE* file, Instrument* instrument) {
  fprintf(file, "- Modulation:\n");
  for (int i = 0; i < 4; i++) {
    fprintf(file, "- Mod%d: %hhu,%hhu,%hhd,%hhu,%hhu,%hhu,%hhu\n",
      i + 1,
      instrument->modulation[i].type,
      instrument->modulation[i].destination,
      instrument->modulation[i].amount,
      instrument->modulation[i].p1,
      instrument->modulation[i].p2,
      instrument->modulation[i].p3,
      instrument->modulation[i].p4);
  }
  return 0;
}

// Main save function
int instrumentSaveData(FILE* file, int idx, Instrument* instrument) {
  fprintf(file, "\n### Instrument %X\n\n", idx);
  fprintf(file, "- Name: %s\n", instrument->name);
  fprintf(file, "- Type: %hhd\n", instrument->type);
  fprintf(file, "- Table speed: %hhu\n", instrument->tableSpeed);
  fprintf(file, "- Transpose: %hhu\n", instrument->transposeEnabled);

  // Save modulation data
  saveModulation(file, instrument);

  // Save chip-specific data
  fprintf(file, "- Chip data:\n");
  switch (instrument->type) {
    case instAY1:
      saveInstrumentAY1(file, instrument);
      break;
    case instAY2:
      saveInstrumentAY2(file, instrument);
      break;
    case instAYSample:
      saveInstrumentAYSample(file, instrument);
      break;
    default:
      break;
  }

  return 0;
}
