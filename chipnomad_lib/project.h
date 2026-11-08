#ifndef __CHIPNOMAD_LIB__PROJECT_H__
#define __CHIPNOMAD_LIB__PROJECT_H__

#include <stdio.h>
#include <stdint.h>
#include "project_instruments.h"
#include "chipnomad_constants.h"

namespace chipnomad {

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
  fxVSL, // Volume slide
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

struct FXName {
  enum FX fx;
  char name[4];
};

struct FXGroup {
  const char* name;              // Group name (e.g., "Sequencer FX")
  FXName* fxList;                // Array of FX in this group
  int count;                     // Number of FX in this group
  int columns;                   // Number of columns for grid layout (default 8)
  InstrumentType instType;  // InstrumentType::none for non-instrument groups
};

extern FXName fxNames[256]; // All names
extern FXGroup fxGroups[]; // FX groups
extern int fxGroupCount; // Number of FX groups

// Chips

enum class ChipType {
  AY = 0,
  totalCount,
};

enum class StereoModeAY : uint8_t {
  ABC,
  ACB,
  BAC,
};

struct ChipSetupAY {
  int clock;
  uint8_t isYM;
  StereoModeAY stereoMode;
  uint8_t stereoSeparation;
  uint8_t pwmFullRange; // 0 = 16 steps (hardware accurate), 1 = 256 steps (extended precision)
};

union ChipSetup {
  ChipSetupAY ay;
};

// Tables

struct TableRow {
  uint8_t pitchFlag;
  uint8_t pitchOffset;
  uint8_t volume;
  uint8_t fx[4][2];
};

struct Table {
  TableRow rows[16];
};

// Grooves

struct Groove {
  uint8_t speed[16];
};

// Phrases

struct PhraseRow {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t fx[3][2];
};

struct Phrase {
  PhraseRow rows[16];
};

// Chains

struct ChainRow {
  uint16_t phrase;
  uint8_t transpose;
};

struct Chain {
  ChainRow rows[16];
};

// Project

struct PitchTable {
  char name[PROJECT_PITCH_TABLE_TITLE_LENGTH + 1];
  uint16_t octaveSize;
  uint16_t length;
  uint16_t values[PROJECT_MAX_PITCHES];
  char noteNames[PROJECT_MAX_PITCHES][4];
};

class Project {
  public:
    Project();
    ~Project();

    char title[PROJECT_TITLE_LENGTH + 1]; // Project title
    char author[PROJECT_TITLE_LENGTH + 1]; // Project author

    float tickRate; // Project tick rate
    ChipType chipType; // Project chip type
    ChipSetup chipSetup; // Chip configuration
    int chipsCount; // Number of chips in the project
    uint8_t linearPitch; // Linear pitch flag

    int tracksCount; // Number of tracks

    PitchTable pitchTable; // Pitch table (cents for linear pitch, period values otherwise)

    // Main project data
    uint16_t song[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
    uint8_t songHighlight[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
    Chain chains[PROJECT_MAX_CHAINS];
    Phrase phrases[PROJECT_MAX_PHRASES];
    Groove grooves[PROJECT_MAX_GROOVES];
    Instrument instruments[PROJECT_MAX_INSTRUMENTS + 1]; // +1 for instrument preview slot
    Table tables[PROJECT_MAX_TABLES];

    // Additional data for different chips
    uint8_t ayWavetables[256][32]; // 256 AY wavetables with 32 4-bit values each

    // Project functions

    void initAYproject();

    void calculatePitchTable(); // Calculate pitch table based on linear pitch setting and chip

    const char* noteName(uint8_t note); // Note name in phrase
    int getChipTracksCount(int chipIndex); // Get number of tracks for a chip at index
    int getTotalTracksCount(); // Get total number of tracks for the project

    bool isChainEmpty(int chain); // Is chain empty?
    bool isPhraseEmpty(int phrase); // Is phrase empty?
    bool isInstrumentEmpty(int instrument); // Is instrument empty?
    bool isTableEmpty(int table); // Is table empty?
    bool isGrooveEmpty(int groove); // Is groove empty?
    bool isWavetableEmpty(int wavetable); // Is wavetable empty?

    void clearPhrase(int phrase); // Clear a phrase
    void clearChain(int chain); // Clear a chain
    void clearInstrument(int instrument); // Clear an instrument
    void clearTable(int table); // Clear a table

    bool chainHasNotes(int chain); // Does chain have notes?
    bool phraseHasNotes(int phrase); // Does phrase have notes?
    const char* instrumentName(uint8_t instrument); // Instrument name
    const char* instrumentTypeName(InstrumentType type); // Instrument type name

  protected:
    void calculatePitchTableAY(); // Calculate 12TET pitch table for AY chip
    void calculateLinearPitchTable12TET(); // Calculate 12TET linear pitch table (values in cents, C-4 = 60)
};

// Fill FX names (call this first before loading any projects)
void fillFXNames();

// Load project from a file
Project* loadProject(const char* path);
// Save project to a file
bool saveProject(Project* p, const char* path);
// Save instrument to a file
bool saveInstrument(Project* p, const char* path, int instrumentIdx);
// Load instrument from a file
bool loadInstrument(Project* p, const char* path, int instrumentIdx);

}; // namespace chipnomad

#endif // __CHIPNOMAD_LIB__PROJECT_H__