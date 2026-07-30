#ifndef __CHIPNOMAD_LIB__PROJECT_INSTRUMENTS_H__
#define __CHIPNOMAD_LIB__PROJECT_INSTRUMENTS_H__

#include <stdlib.h>
#include <stdint.h>
#include "project_constants.h"

// Forward declarations
struct Project;

// Instruments

enum class InstrumentType : uint8_t {
  none = 0,
  AY1 = 1,
  AY2 = 2,
  AYSample = 3,
};

enum class ModulationType : uint8_t {
  ADSR = 0,
  AHD = 1,
  LFO = 2,
  totalCount,
};

enum class LFOShape : uint8_t {
  tri = 0,
  sin = 1,
  uniTri = 2,
  uniSin = 3,
  rampDown = 4,
  rampUp = 5,
  expDown = 6,
  expUp = 7,
  square = 8,
  random = 9,
  totalCount,
};

enum class LFOTrigger : uint8_t {
  free = 0,
  retrig = 1,
  hold = 2,
  once = 3,
  totalCount,
};

struct Modulation {
  ModulationType type;
  uint8_t destination;
  int8_t amount;
  uint8_t p1; // ADSR: A, AHD: A, LFO: Shape
  uint8_t p2; // ADSR: D, AHD: H, LFO: Trig
  uint8_t p3; // ADSR: S, AHD: D, LFO: Period
  uint8_t p4; // ADSR: R, AHD: -, LFO: -
};

// AY Instruments

struct InstrumentAY1 {
  Modulation volumeEnvelope;  // ADSR envelope as modulation
  uint8_t autoEnvN; // 0 - no auto-env
  uint8_t autoEnvD;
  uint8_t defaultMixer; // Low nibble: mixer, high nibble: envelope shape
};

struct InstrumentAYOscTone {
  uint8_t isOn;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
};

struct InstrumentAYOscNoise {
  uint8_t isOn;
  uint8_t noisePeriod;
};

struct InstrumentAYOscEnvelope {
  uint8_t shape;
  uint8_t autoEnvN;
  uint8_t autoEnvD;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
};

enum class AYSoftwareOscType : uint8_t {
  none = 0,
  pulse = 1,
  syncTone = 2,
  syncEnvelope = 3,
  wavetable = 4,
  toneFM = 5,
  envFM = 6,
  sample = 7, // Needs to be last for various conditions for AY2 instrument
  totalCount,
};

struct InstrumentAYOscSoftware {
  AYSoftwareOscType type;
  uint8_t pitchFlag;
  int8_t pitchOffset;
  int8_t fineTune;
  uint8_t pulseWidth;
  uint8_t pulseLow;
  uint8_t wavetableIndex;
  uint8_t fmDepth;
  uint8_t envShapePair; // For SyncEnv: high nibble = shape 1, low nibble = shape 2, default 0x00
};

struct InstrumentAY2 {
  InstrumentAYOscTone oscTone;
  InstrumentAYOscNoise oscNoise;
  InstrumentAYOscEnvelope oscEnvelope;
  InstrumentAYOscSoftware oscSoftware;
};

struct InstrumentAYSample {
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
};

union InstrumentChipData {
  InstrumentAY1 ay;
  InstrumentAY2 ay2;
  InstrumentAYSample aySample;
};

struct Instrument {
  InstrumentType type;
  char name[PROJECT_INSTRUMENT_NAME_LENGTH + 1];
  uint8_t tableSpeed;
  uint8_t transposeEnabled;
  Modulation modulation[4];
  InstrumentChipData chip;
};

struct InstrumentFunctions {
  int modDestinationsCount;
  const char* (*modName)(int modIndex);
  int (*init)(Instrument* instrument);
  int (*free)(Instrument* instrument);
};

InstrumentFunctions getInstrumentFunctions(InstrumentType type);

#endif // __CHIPNOMAD_LIB__PROJECT_INSTRUMENTS_H__