#ifndef __PROJECT_H__
#define __PROJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "project_instruments.h"
#include "project_constants.h"

// Song data structures

// FX

enum FX {
  // Sequencer FX
  fxARP, // Arpeggio
  fxARC, // Arpeggio config
  fxPVB, // Pitch vibrato
  fxPBN, // Pitch bend
  fxPSL, // Pitch slide (portamento)
  fxPIT, // Pitch - steps/semitones (relative)
  fxFIN, // Pitch fine (relative)
  fxPRD, // Period (relative)
  fxVOL, // Volume (relative)
  fxRET, // Retrigger
  fxDEL, // Delay
  fxOFF, // Off
  fxKIL, // Kill note
  fxTIC, // Table speed
  fxTBL, // Set instrument table
  fxTBX, // Set aux table
  fxTHO, // Table hop
  fxTXH, // Aux table hop
  fxGRV, // Track groove
  fxGGR, // Global groove
  fxHOP, // Hop
  fxSNG, // Song hop

  // Modulation FX
  fxM1A, // Modulation 1 amount (relative)
  fxM11, // Modulation 1, parameter 1 (relative)
  fxM12, // Modulation 1, parameter 2 (relative)
  fxM13, // Modulation 1, parameter 3 (relative)
  fxM14, // Modulation 1, parameter 4 (relative)
  fxM2A, // Modulation 2 amount (relative)
  fxM21, // Modulation 2, parameter 1 (relative)
  fxM22, // Modulation 2, parameter 2 (relative)
  fxM23, // Modulation 2, parameter 3 (relative)
  fxM24, // Modulation 2, parameter 4 (relative)
  fxM3A, // Modulation 3 amount (relative)
  fxM31, // Modulation 3, parameter 1 (relative)
  fxM32, // Modulation 3, parameter 2 (relative)
  fxM33, // Modulation 3, parameter 3 (relative)
  fxM34, // Modulation 3, parameter 4 (relative)
  fxM4A, // Modulation 4 amount (relative)
  fxM41, // Modulation 4, parameter 1 (relative)
  fxM42, // Modulation 4, parameter 2 (relative)
  fxM43, // Modulation 4, parameter 3 (relative)
  fxM44, // Modulation 4, parameter 4 (relative)

  // AY FX
  // Common AY FX (used in 1+ AY instrument types):
  fxAYM, // AY Mixer settting
  fxERT, // Envelope phase retrigger
  fxNOI, // Noise (relative)
  fxNOA, // Noise (absolute)
  fxEAU, // Auto-env setting
  fxTNN, // Tone specific note
  fxTNP, // Tone pitch (relative)
  fxTNF, // Tone fine (relative)
  fxTRT, // Tone phase retrigger
  fxENN, // Envelope specific note
  fxENP, // Envelope pitch (relative)
  fxENF, // Envelope fine (relative)
  // AY Classic instrument (AY1):
  fxEVB, // Envelope vibrato
  fxEBN, // Envelope bend
  fxESL, // Envelope slide (portamento)
  fxENT, // Envelope note
  fxEPT, // Envelope period (relative)
  fxEPL, // Envelope period L
  fxEPH, // Envelope period H
  // AY Plus instrument (AY2):
  fxSFT, // Software oscillator type
  fxSFN, // Software oscillator specific note
  fxSFP, // Software oscillator pitch (relative)
  fxSFF, // Software oscillator fine (relative)
  fxSRT, // Software oscillator phase retrigger
  fxSFM, // Software oscillator FM depth (relative)
  fxPWM, // Software oscillator pulse width (relative)
  fxSPL, // Software oscillator pulse low
  fxSWT, // Software oscillator wavetable index (relative)
  // AY Sample instrument (AYSample):
  fxSMS, // Sample start position
  // TODO: FM FX

  // TODO: SID FX

  // Total count - must be last
  fxTotalCount
};

typedef struct FXName {
  enum FX fx;
  char name[4];
} FXName;

typedef struct FXGroup {
  const char* name;              // Group name (e.g., "Sequencer FX")
  FXName* fxList;                // Array of FX in this group
  int count;                     // Number of FX in this group
  int columns;                   // Number of columns for grid layout (default 8)
  enum InstrumentType instType;  // instNone for non-instrument groups
} FXGroup;

extern FXName fxNames[256]; // All names
extern FXGroup fxGroups[]; // FX groups
extern int fxGroupCount; // Number of FX groups

// Chips

enum ChipType {
  chipAY = 0,
  chipTotalCount,
};

enum StereoModeAY {
  ayStereoABC,
  ayStereoACB,
  ayStereoBAC,
};

typedef struct ChipSetupAY {
  int clock;
  uint8_t isYM;
  enum StereoModeAY stereoMode;
  uint8_t stereoSeparation;
  uint8_t pwmFullRange; // 0 = 16 steps (hardware accurate), 1 = 256 steps (extended precision)
} ChipSetupAY;

typedef union ChipSetup {
  ChipSetupAY ay;
} ChipSetup;

// Tables

typedef struct TableRow {
  uint8_t pitchFlag;
  uint8_t pitchOffset;
  uint8_t volume;
  uint8_t fx[4][2];
} TableRow;

typedef struct Table {
  TableRow rows[16];
} Table;

// Grooves

typedef struct Groove {
  uint8_t speed[16];
} Groove;

// Phrases

typedef struct PhraseRow {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t fx[3][2];
} PhraseRow;

typedef struct Phrase {
  PhraseRow rows[16];
} Phrase;

// Chains

typedef struct ChainRow {
  uint16_t phrase;
  uint8_t transpose;
} ChainRow;

typedef struct Chain {
  ChainRow rows[16];
} Chain;

// Project

typedef struct PitchTable {
  char name[PROJECT_PITCH_TABLE_TITLE_LENGTH + 1];
  uint16_t octaveSize;
  uint16_t length;
  uint16_t values[PROJECT_MAX_PITCHES];
  char noteNames[PROJECT_MAX_PITCHES][4];
} PitchTable;

typedef struct Project {
  char title[PROJECT_TITLE_LENGTH + 1];
  char author[PROJECT_TITLE_LENGTH + 1];

  float tickRate;
  enum ChipType chipType;
  ChipSetup chipSetup;
  int chipsCount;
  uint8_t linearPitch;

  int tracksCount;

  PitchTable pitchTable;

  // Main project data
  uint16_t song[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
  uint8_t songHighlight[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
  Chain chains[PROJECT_MAX_CHAINS];
  Phrase phrases[PROJECT_MAX_PHRASES];
  Groove grooves[PROJECT_MAX_GROOVES];
  Instrument instruments[PROJECT_MAX_INSTRUMENTS];
  Table tables[PROJECT_MAX_TABLES];

  // Additional data for different chips
  uint8_t ayWavetables[256][32]; // 256 wavetables with 32 4-bit values each (AY chip)
} Project;

extern char projectFileError[41];

// Fill FX names (call this first before loading any projects)
void fillFXNames();
// Initialize an empty project
void projectInit(Project* p);
// Load project from a file
int projectLoad(Project* p, const char* path);
// Save project to a file
int projectSave(Project* p, const char* path);
// Save instrument to a file
int instrumentSave(Project* p, const char* path, int instrumentIdx);
// Load instrument from a file
int instrumentLoad(Project* p, const char* path, int instrumentIdx);

// Is chain empty?
int8_t chainIsEmpty(Project* p, int chain);
// Is phrase empty?
int8_t phraseIsEmpty(Project* p, int phrase);
// Is instrument empty?
int8_t instrumentIsEmpty(Project* p, int instrument);
// Is table empty?
int8_t tableIsEmpty(Project* p, int table);
// Is groove empty?
int8_t grooveIsEmpty(Project* p, int groove);
// Is wavetable empty?
int8_t wavetableIsEmpty(Project* p, int wavetable);
// Note name in phrase
const char* noteName(Project* p, uint8_t note);
// Get number of tracks for a chip at index
int projectGetChipTracks(Project* p, int chipIndex);
// Get total number of tracks for the project
int projectGetTotalTracks(Project* p);
// Clear a single phrase with proper initialization
void phraseClear(Phrase* phrase);
// Clear a single chain with proper initialization
void chainClear(Chain* chain);
// Clear a single instrument with proper initialization
void instrumentClear(Instrument* instrument);
// Clear a single table with proper initialization
void tableClear(Table* table);

#ifdef __cplusplus
}
#endif

#endif