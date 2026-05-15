#ifndef __PROJECT_INSTRUMENTS_H__
#define __PROJECT_INSTRUMENTS_H__

#include <stdlib.h>
#include "project_constants.h"

// Instruments

enum InstrumentType {
  instNone = 0,
  instAY1 = 1,
  instAY2 = 2,
  instAYSample = 3,
  instAYWavetable = 4,
};

enum ModulationType {
  modNone = 0,
  modAHD = 1,
  modADSR = 2,
  modLFO = 3,
  modTotalCount,
};

enum LFOShape {
  lfoTri = 0,
  lfoSin = 1,
  lfoRampDown = 2,
  lfoRampUp = 3,
  lfoExpDown = 4,
  lfoExpUp = 5,
  lfoSquare = 6,
  lfoRandom = 7,
  lfoShapeTotalCount,
};

enum LFOTrigger {
  lfoFree = 0,
  lfoRetrig = 1,
  lfoHold = 2,
  lfoOnce = 3,
  lfoTriggerTotalCount,
};

typedef struct Modulation {
  enum ModulationType type;
  uint8_t destination;
  int8_t amount;
  uint8_t p1;
  uint8_t p2;
  uint8_t p3;
  uint8_t p4;
} Modulation;

// AY Instruments

typedef struct InstrumentAY1 {
  Modulation volumeEnvelope;  // ADSR envelope as modulation
  uint8_t autoEnvN; // 0 - no auto-env
  uint8_t autoEnvD;
  uint8_t defaultMixer; // Low nibble: mixer, high nibble: envelope shape
} InstrumentAY1;

typedef struct InstrumentAYOscTone {
  uint8_t isOn;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
} InstrumentAYOscTone;

typedef struct InstrumentAYOscNoise {
  uint8_t isOn;
  uint8_t noisePeriod;
} InstrumentAYOscNoise;

typedef struct InstrumentAYOscEnvelope {
  uint8_t shape;
  uint8_t autoEnvN;
  uint8_t autoEnvD;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
} InstrumentAYOscEnvelope;

enum AYSoftwareOscType {
  aySoftwareOscNone = 0,
  aySoftwareOscRingMod = 1,
  aySoftwareOscSyncTone = 2,
  aySoftwareOscSyncEnvelope = 3,
  aySoftwareOscNoiseWavetable = 4,
  aysoftwareOscTotalCount,
};

typedef struct InstrumentAYOscSoftware {
  enum AYSoftwareOscType type;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
} InstrumentAYOscSoftware;

typedef struct InstrumentAY2 {
  InstrumentAYOscTone oscTone;
  InstrumentAYOscNoise oscNoise;
  InstrumentAYOscEnvelope oscEnvelope;
  InstrumentAYOscSoftware oscSoftware;
} InstrumentAY2;

typedef struct InstrumentAYSample {
  InstrumentAYOscTone oscTone;
  InstrumentAYOscNoise oscNoise;
  char sampleName[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
  uint16_t fileLength;
  uint16_t sampleRate;
  uint16_t sampleStart;
  uint16_t sampleLength;
  uint16_t sampleLoopStart;
  uint16_t sampleLoopEnd;
  uint8_t *sampleData;  // 8-bit unsigned PCM data
  int8_t pitchOffset;
  int8_t fineTune;
} InstrumentAYSample;

typedef struct InstrumentAYWavetable {
  InstrumentAYOscTone oscTone;
  InstrumentAYOscNoise oscNoise;
  uint8_t waveIndex;
  int8_t pitchOffset;
  int8_t fineTune;
} InstrumentAYWavetable;

typedef union InstrumentChipData {
  InstrumentAY1 ay;
  InstrumentAY2 ay2;
  InstrumentAYSample aySample;
  InstrumentAYWavetable ayWavetable;
} InstrumentChipData;

typedef struct Instrument {
  uint8_t type; // enum InstrumentType
  char name[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
  Modulation modulation[4];
  InstrumentChipData chip;
} Instrument;

#endif