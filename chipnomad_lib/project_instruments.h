#ifndef __PROJECT_INSTRUMENTS_H__
#define __PROJECT_INSTRUMENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include "project_constants.h"

// Forward declarations
struct Project;

// Instruments

enum InstrumentType {
  instNone = 0,
  instAY1 = 1,
  instAY2 = 2,
  instAYSample = 3,
};

enum ModulationType {
  modADSR = 0,
  modAHD = 1,
  modLFO = 2,
  modTotalCount,
};

enum LFOShape {
  lfoShapeTri = 0,
  lfoShapeSin = 1,
  lfoShapeRampDown = 2,
  lfoShapeRampUp = 3,
  lfoShapeExpDown = 4,
  lfoShapeExpUp = 5,
  lfoShapeSquare = 6,
  lfoShapeRandom = 7,
  lfoShapeTotalCount,
};

enum LFOTrigger {
  lfoTrigFree = 0,
  lfoTrigRetrig = 1,
  lfoTrigHold = 2,
  lfoTrigOnce = 3,
  lfoTrigTotalCount,
};

typedef struct Modulation {
  enum ModulationType type;
  uint8_t destination;
  int8_t amount;
  uint8_t p1; // ADSR: A, AHD: A, LFO: Shape
  uint8_t p2; // ADSR: D, AHD: H, LFO: Trig
  uint8_t p3; // ADSR: S, AHD: D, LFO: Period
  uint8_t p4; // ADSR: R, AHD: -, LFO: -
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
  aySoftwareOscPulse = 1,
  aySoftwareOscSyncTone = 2,
  aySoftwareOscSyncEnvelope = 3,
  aySoftwareOscWavetable = 4,
  aySoftwareOscToneFM = 5,
  aySoftwareOscEnvFM = 6,
  aySoftwareOscSample = 7, // Needs to be last for various conditions for AY2 instrument
  aySoftwareOscTotalCount,
};

typedef struct InstrumentAYOscSoftware {
  enum AYSoftwareOscType type;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
  uint8_t pulseWidth;
  uint8_t pulseLow;
  uint8_t wavetableIndex;
  uint8_t fmDepth;
  uint8_t envShapePair; // For SyncEnv: high nibble = shape 1, low nibble = shape 2, default 0x00
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
  uint8_t *sampleData;  // 8-bit unsigned PCM data
  int8_t pitchOffset;
  int8_t fineTune;
} InstrumentAYSample;

typedef union InstrumentChipData {
  InstrumentAY1 ay;
  InstrumentAY2 ay2;
  InstrumentAYSample aySample;
} InstrumentChipData;

typedef struct Instrument {
  uint8_t type;
  char name[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
  Modulation modulation[4];
  InstrumentChipData chip;
} Instrument;

typedef struct InstrumentFunctions {
  int modDestinationsCount;
  const char* (*modName)(int modIndex);
  int (*init)(Instrument* instrument);
  int (*free)(Instrument* instrument);
} InstrumentFunctions;

InstrumentFunctions getInstrumentFunctions(enum InstrumentType type);

#ifdef __cplusplus
}
#endif

#endif