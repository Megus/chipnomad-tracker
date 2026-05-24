#include "project.h"
#include "project_io_common.h"
#include "corelib/corelib_file.h"
#include <stdio.h>
#include <string.h>

// Load AY1 instrument data (legacy format - version 1.0)
static int loadInstrumentAY1Legacy(int fileId, Instrument* instrument) {
  while (1) {
    char* line = peekLine(fileId);
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
    consumeLine(fileId);
  }
}

// Load AY1 instrument data (new format - version 2.0)
static int loadInstrumentAY1(int fileId, Instrument* instrument) {
  while (1) {
    char* line = peekLine(fileId);
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
    consumeLine(fileId);
  }
}

// Load AY2 instrument data
static int loadInstrumentAY2(int fileId, Instrument* instrument) {
  while (1) {
    char* line = peekLine(fileId);
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
    } else if (strncmp(line, "- Software aux parameter: ", 26) == 0) {
      sscanf(line, "- Software aux parameter: %hhu", &instrument->chip.ay2.oscSoftware.auxParameter);
    }
    consumeLine(fileId);
  }
}

// Load AYSample instrument data
static int loadInstrumentAYSample(int fileId, Instrument* instrument) {
  while (1) {
    char* line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

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
    } else if (strncmp(line, "- Sample loop end: ", 19) == 0) {
      sscanf(line, "- Sample loop end: %hX", &instrument->chip.aySample.sampleLoopEnd);
    } else if (strncmp(line, "- Sample pitch offset: ", 23) == 0) {
      sscanf(line, "- Sample pitch offset: %hhd", &instrument->chip.aySample.pitchOffset);
    } else if (strncmp(line, "- Sample fine tune: ", 20) == 0) {
      sscanf(line, "- Sample fine tune: %hhd", &instrument->chip.aySample.fineTune);
    }
    // TODO: Sample data loading (binary data)
    consumeLine(fileId);
  }
}

// Load AYWavetable instrument data
static int loadInstrumentAYWavetable(int fileId, Instrument* instrument) {
  while (1) {
    char* line = peekLine(fileId);
    if (line == NULL) return 1;
    if (line[0] == '#') return 0;

    // Tone oscillator
    if (strncmp(line, "- Tone on: ", 11) == 0) {
      sscanf(line, "- Tone on: %hhu", &instrument->chip.ayWavetable.oscTone.isOn);
    } else if (strncmp(line, "- Tone pitch flag: ", 19) == 0) {
      sscanf(line, "- Tone pitch flag: %hhu", &instrument->chip.ayWavetable.oscTone.pitchFlag);
    } else if (strncmp(line, "- Tone pitch offset: ", 21) == 0) {
      sscanf(line, "- Tone pitch offset: %hhd", &instrument->chip.ayWavetable.oscTone.pitchOffset);
    } else if (strncmp(line, "- Tone fine tune: ", 18) == 0) {
      sscanf(line, "- Tone fine tune: %hhd", &instrument->chip.ayWavetable.oscTone.fineTune);
    }
    // Noise oscillator
    else if (strncmp(line, "- Noise on: ", 12) == 0) {
      sscanf(line, "- Noise on: %hhu", &instrument->chip.ayWavetable.oscNoise.isOn);
    } else if (strncmp(line, "- Noise period: ", 16) == 0) {
      sscanf(line, "- Noise period: %hhu", &instrument->chip.ayWavetable.oscNoise.noisePeriod);
    }
    // Wavetable parameters
    else if (strncmp(line, "- Wave index: ", 14) == 0) {
      sscanf(line, "- Wave index: %hhu", &instrument->chip.ayWavetable.waveIndex);
    } else if (strncmp(line, "- Wave pitch offset: ", 21) == 0) {
      sscanf(line, "- Wave pitch offset: %hhd", &instrument->chip.ayWavetable.pitchOffset);
    } else if (strncmp(line, "- Wave fine tune: ", 18) == 0) {
      sscanf(line, "- Wave fine tune: %hhd", &instrument->chip.ayWavetable.fineTune);
    }
    consumeLine(fileId);
  }
}

// Load modulation data
static int loadModulation(int fileId, Instrument* instrument) {
  for (int i = 0; i < 4; i++) {
    char* line = peekLine(fileId);
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
    consumeLine(fileId);
  }
  return 0;
}

// Main load function
int instrumentLoadData(int fileId, Instrument* instrument, Project* p) {
  instrumentClear(instrument);

  // Read common fields first
  while (1) {
    char* line = peekLine(fileId);
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
      consumeLine(fileId);
      // After reading transpose, check what comes next
      break;
    }
    consumeLine(fileId);
  }

  // Check version to determine format
  if (projectFileVersion == 1) {
    // Legacy format (version 1.0): no modulation, no "Chip data:" separator
    // Only AY1 instruments existed in version 1.0
    if (instrument->type == instAY1) {
      if (loadInstrumentAY1Legacy(fileId, instrument)) return 1;
    }
    return 0;
  }

  // New format (version 2.0): read modulation and chip data sections
  char* line = peekLine(fileId);
  if (line == NULL) return 1;

  if (strncmp(line, "- Modulation:", 13) == 0) {
    consumeLine(fileId);
    if (loadModulation(fileId, instrument)) return 1;
    line = peekLine(fileId);  // Read next line after modulation
    if (line == NULL) return 1;
  }

  if (strncmp(line, "- Chip data:", 12) == 0) {
    consumeLine(fileId);
    // Load chip-specific data based on instrument type
    switch (instrument->type) {
      case instAY1:
        if (loadInstrumentAY1(fileId, instrument)) return 1;
        break;
      case instAY2:
        if (loadInstrumentAY2(fileId, instrument)) return 1;
        break;
      case instAYSample:
        if (loadInstrumentAYSample(fileId, instrument)) return 1;
        break;
      case instAYWavetable:
        if (loadInstrumentAYWavetable(fileId, instrument)) return 1;
        break;
      default:
        break;
    }
  }

  return 0;
}

// Save AY1 instrument data
static int saveInstrumentAY1(int fileId, Instrument* instrument) {
  // Save volume envelope as ADSR values (for backward compatibility in file format)
  Modulation* ve = &instrument->chip.ay.volumeEnvelope;
  filePrintf(fileId, "- Volume envelope: %hhu,%hhu,%hhu,%hhu\n",
    ve->p1, ve->p2, ve->p3, ve->p4);  // A, D, S, R
  filePrintf(fileId, "- Auto envelope: %hhd,%hhd\n",
    instrument->chip.ay.autoEnvN, instrument->chip.ay.autoEnvD);
  filePrintf(fileId, "- Default mixer: %02X\n", instrument->chip.ay.defaultMixer);
  return 0;
}

// Save AY2 instrument data
static int saveInstrumentAY2(int fileId, Instrument* instrument) {
  // Tone oscillator
  filePrintf(fileId, "- Tone on: %hhu\n", instrument->chip.ay2.oscTone.isOn);
  filePrintf(fileId, "- Tone pitch flag: %hhu\n", instrument->chip.ay2.oscTone.pitchFlag);
  filePrintf(fileId, "- Tone pitch offset: %hhd\n", instrument->chip.ay2.oscTone.pitchOffset);
  filePrintf(fileId, "- Tone fine tune: %hhd\n", instrument->chip.ay2.oscTone.fineTune);

  // Noise oscillator
  filePrintf(fileId, "- Noise on: %hhu\n", instrument->chip.ay2.oscNoise.isOn);
  filePrintf(fileId, "- Noise period: %hhu\n", instrument->chip.ay2.oscNoise.noisePeriod);

  // Envelope oscillator
  filePrintf(fileId, "- Envelope shape: %hhu\n", instrument->chip.ay2.oscEnvelope.shape);
  filePrintf(fileId, "- Envelope auto N: %hhu\n", instrument->chip.ay2.oscEnvelope.autoEnvN);
  filePrintf(fileId, "- Envelope auto D: %hhu\n", instrument->chip.ay2.oscEnvelope.autoEnvD);
  filePrintf(fileId, "- Envelope pitch flag: %hhu\n", instrument->chip.ay2.oscEnvelope.pitchFlag);
  filePrintf(fileId, "- Envelope pitch offset: %hhd\n", instrument->chip.ay2.oscEnvelope.pitchOffset);
  filePrintf(fileId, "- Envelope fine tune: %hhd\n", instrument->chip.ay2.oscEnvelope.fineTune);

  // Software oscillator
  filePrintf(fileId, "- Software type: %hhu\n", instrument->chip.ay2.oscSoftware.type);
  filePrintf(fileId, "- Software pitch flag: %hhu\n", instrument->chip.ay2.oscSoftware.pitchFlag);
  filePrintf(fileId, "- Software pitch offset: %hhd\n", instrument->chip.ay2.oscSoftware.pitchOffset);
  filePrintf(fileId, "- Software fine tune: %hhd\n", instrument->chip.ay2.oscSoftware.fineTune);
  filePrintf(fileId, "- Software aux parameter: %hhu\n", instrument->chip.ay2.oscSoftware.auxParameter);

  return 0;
}

// Save AYSample instrument data
static int saveInstrumentAYSample(int fileId, Instrument* instrument) {
  // Tone oscillator
  filePrintf(fileId, "- Tone on: %hhu\n", instrument->chip.aySample.oscTone.isOn);
  filePrintf(fileId, "- Tone pitch flag: %hhu\n", instrument->chip.aySample.oscTone.pitchFlag);
  filePrintf(fileId, "- Tone pitch offset: %hhd\n", instrument->chip.aySample.oscTone.pitchOffset);
  filePrintf(fileId, "- Tone fine tune: %hhd\n", instrument->chip.aySample.oscTone.fineTune);

  // Noise oscillator
  filePrintf(fileId, "- Noise on: %hhu\n", instrument->chip.aySample.oscNoise.isOn);
  filePrintf(fileId, "- Noise period: %hhu\n", instrument->chip.aySample.oscNoise.noisePeriod);

  // Sample parameters
  filePrintf(fileId, "- Sample name: %s\n", instrument->chip.aySample.sampleName);
  filePrintf(fileId, "- Sample rate: %hu\n", instrument->chip.aySample.sampleRate);
  filePrintf(fileId, "- Sample start: %04X\n", instrument->chip.aySample.sampleStart);
  filePrintf(fileId, "- Sample length: %04X\n", instrument->chip.aySample.sampleLength);
  filePrintf(fileId, "- Sample loop start: %04X\n", instrument->chip.aySample.sampleLoopStart);
  filePrintf(fileId, "- Sample loop end: %04X\n", instrument->chip.aySample.sampleLoopEnd);
  filePrintf(fileId, "- Sample pitch offset: %hhd\n", instrument->chip.aySample.pitchOffset);
  filePrintf(fileId, "- Sample fine tune: %hhd\n", instrument->chip.aySample.fineTune);

  // TODO: Sample data saving (binary data)

  return 0;
}

// Save AYWavetable instrument data
static int saveInstrumentAYWavetable(int fileId, Instrument* instrument) {
  // Tone oscillator
  filePrintf(fileId, "- Tone on: %hhu\n", instrument->chip.ayWavetable.oscTone.isOn);
  filePrintf(fileId, "- Tone pitch flag: %hhu\n", instrument->chip.ayWavetable.oscTone.pitchFlag);
  filePrintf(fileId, "- Tone pitch offset: %hhd\n", instrument->chip.ayWavetable.oscTone.pitchOffset);
  filePrintf(fileId, "- Tone fine tune: %hhd\n", instrument->chip.ayWavetable.oscTone.fineTune);

  // Noise oscillator
  filePrintf(fileId, "- Noise on: %hhu\n", instrument->chip.ayWavetable.oscNoise.isOn);
  filePrintf(fileId, "- Noise period: %hhu\n", instrument->chip.ayWavetable.oscNoise.noisePeriod);

  // Wavetable parameters
  filePrintf(fileId, "- Wave index: %hhu\n", instrument->chip.ayWavetable.waveIndex);
  filePrintf(fileId, "- Wave pitch offset: %hhd\n", instrument->chip.ayWavetable.pitchOffset);
  filePrintf(fileId, "- Wave fine tune: %hhd\n", instrument->chip.ayWavetable.fineTune);

  return 0;
}

// Save modulation data
static int saveModulation(int fileId, Instrument* instrument) {
  filePrintf(fileId, "- Modulation:\n");
  for (int i = 0; i < 4; i++) {
    filePrintf(fileId, "- Mod%d: %hhu,%hhu,%hhd,%hhu,%hhu,%hhu,%hhu\n",
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
int instrumentSaveData(int fileId, int idx, Instrument* instrument) {
  filePrintf(fileId, "\n### Instrument %X\n\n", idx);
  filePrintf(fileId, "- Name: %s\n", instrument->name);
  filePrintf(fileId, "- Type: %hhd\n", instrument->type);
  filePrintf(fileId, "- Table speed: %hhu\n", instrument->tableSpeed);
  filePrintf(fileId, "- Transpose: %hhu\n", instrument->transposeEnabled);

  // Save modulation data
  saveModulation(fileId, instrument);

  // Save chip-specific data
  filePrintf(fileId, "- Chip data:\n");
  switch (instrument->type) {
    case instAY1:
      saveInstrumentAY1(fileId, instrument);
      break;
    case instAY2:
      saveInstrumentAY2(fileId, instrument);
      break;
    case instAYSample:
      saveInstrumentAYSample(fileId, instrument);
      break;
    case instAYWavetable:
      saveInstrumentAYWavetable(fileId, instrument);
      break;
    default:
      break;
  }

  return 0;
}
