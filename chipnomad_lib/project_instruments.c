#include <string.h>
#include "project_instruments.h"
#include "project.h"

// Convention: the first modulation destination should be volume

static void initCommon(Instrument* instrument) {
  memset(instrument, 0, sizeof(Instrument));
  instrument->tableSpeed = 1;
  instrument->transposeEnabled = 1;
  instrument->modulation[0].type = modADSR;
  instrument->modulation[1].type = modAHD;
  instrument->modulation[2].type = modLFO;
  instrument->modulation[3].type = modLFO;
}

static void freeCommon(Instrument* instrument) {
  memset(instrument, 0, sizeof(Instrument));
}

// Instrument type: None
static char* modNameNone(int modIndex) {
  return "Off";
}

static int initNoneInstrument(Instrument* instrument) {
  initCommon(instrument);
  instrument->type = instNone;
  return 0;
}

static int freeNoneInstrument(Instrument* instrument) {
  freeCommon(instrument);
  return 0;
}

// Instrument type: AY1
static char* modNameAY1(int modIndex) {
  static char *names[] = {"Off", "Volume", "Pitch", "Noise", "EnvPrd"};
  return names[modIndex];
}

static int initAY1Instrument(Instrument* instrument) {
  initCommon(instrument);
  instrument->type = instAY1;
  instrument->chip.ay.defaultMixer = 0x01; // Tone on, noise off, envelope shape 0
  instrument->chip.ay.volumeEnvelope = (Modulation){.type = modADSR, .destination = 1, .amount = 127, .p1 = 0, .p2 = 0, .p3 = 15, .p4 = 0 };
  return 0;
}

static int freeAY1Instrument(Instrument* instrument) {
  freeCommon(instrument);
  return 0;
}

// Instrument type: AY2
static char* modNameAY2(int modIndex) {
  static char *names[] = {"Off", "Volume", "Pitch", "TonePit", "Noise", "EnvPit", "SoftPit", "PulseW", "PulseL", "WavIdx"};
  return names[modIndex];
}

static int initAY2Instrument(Instrument* instrument) {
  initCommon(instrument);
  instrument->type = instAY2;
  instrument->chip.ay2.oscTone.isOn = 1;
  instrument->chip.ay2.oscEnvelope.pitchOffset = 48; // +4 octaves because envelope is lower
  instrument->chip.ay2.oscSoftware.pulseWidth = 0x80; // 50% duty cycle
  return 0;
}

static int freeAY2Instrument(Instrument* instrument) {
  freeCommon(instrument);
  return 0;
}

// Instrument type: AY Sample
static char* modNameAYSample(int modIndex) {
  static char *names[] = {"Off", "Volume", "Pitch", "SmplPit", "TonePit", "Noise"};
  return names[modIndex];
}

static int initAYSampleInstrument(Instrument* instrument) {
  initCommon(instrument);
  instrument->type = instAYSample;

  return 0;
}

static int freeAYSampleInstrument(Instrument* instrument) {
  if (instrument->chip.aySample.sampleData != NULL) {
    free(instrument->chip.aySample.sampleData);
  }
  freeCommon(instrument);
  return 0;
}

// Get function pointers for instrument type
InstrumentFunctions getInstrumentFunctions(enum InstrumentType type) {
  switch (type) {
    case instAY1:
      return (InstrumentFunctions){
        .modDestinationsCount = 4,
        .modName = modNameAY1,
        .init = initAY1Instrument,
        .free = freeAY1Instrument
      };
    case instAY2:
      return (InstrumentFunctions){
        .modDestinationsCount = 9,
        .modName = modNameAY2,
        .init = initAY2Instrument,
        .free = freeAY2Instrument
      };
    case instAYSample:
      return (InstrumentFunctions){
        .modDestinationsCount = 5,
        .modName = modNameAYSample,
        .init = initAYSampleInstrument,
        .free = freeAYSampleInstrument
      };
    default:
      return (InstrumentFunctions){
        .modDestinationsCount = 0,
        .modName = modNameNone,
        .init = initNoneInstrument,
        .free = freeNoneInstrument
      };
  }
}

