#include <stdio.h>
#include <string.h>
#include "project.h"
#include "project_instruments.h"
#include "utils.h"
#include "playback.h"

namespace chipnomad {

  // Note names for 12TET pitch table
  static char noteStrings[12][4] = { "C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0" };

  FXName fxNames[256];

  // FX Names organized by groups (in the order as they appear in FX select screen)

  // Sequencer FX
  FXName fxNamesSequencer[] = {
    {fxARP, "ARP"}, {fxARC, "ARC"}, {fxPVB, "PVB"}, {fxPBN, "PBN"}, {fxPSL, "PSL"},
    {fxPIT, "PIT"}, {fxFIN, "FIN"}, {fxPRD, "PRD"}, {fxVOL, "VOL"}, {fxVSL, "VSL"},
    {fxRET, "RET"}, {fxDEL, "DEL"}, {fxOFF, "OFF"}, {fxKIL, "KIL"}, {fxTIC, "TIC"},
    {fxTBL, "TBL"}, {fxTBX, "TBX"}, {fxTHO, "THO"}, {fxTXH, "TXH"}, {fxGRV, "GRV"},
    {fxGGR, "GGR"}, {fxHOP, "HOP"}, {fxSNG, "SNG"}
  };
  int fxSequencerCount = sizeof(fxNamesSequencer) / sizeof(FXName);

  // Modulation FX
  FXName fxNamesModulation[] = {
    {fxM1A, "M1A"}, {fxM11, "M11"}, {fxM12, "M12"}, {fxM13, "M13"}, {fxM14, "M14"},
    {fxM2A, "M2A"}, {fxM21, "M21"}, {fxM22, "M22"}, {fxM23, "M23"}, {fxM24, "M24"},
    {fxM3A, "M3A"}, {fxM31, "M31"}, {fxM32, "M32"}, {fxM33, "M33"}, {fxM34, "M34"},
    {fxM4A, "M4A"}, {fxM41, "M41"}, {fxM42, "M42"}, {fxM43, "M43"}, {fxM44, "M44"}
  };
  int fxModulationCount = sizeof(fxNamesModulation) / sizeof(FXName);

  // AY1 FX
  FXName fxNamesAY1[] = {
    {fxAYM, "AYM"}, {fxNOI, "NOI"}, {fxNOA, "NOA"}, {fxERT, "ERT"}, {fxEAU, "EAU"},
    {fxEVB, "EVB"}, {fxEBN, "EBN"}, {fxESL, "ESL"}, {fxENT, "ENT"}, {fxEPT, "EPT"},
    {fxEPL, "EPL"}, {fxEPH, "EPH"},
  };
  int fxAY1Count = sizeof(fxNamesAY1) / sizeof(FXName);

  // AY2 FX
  FXName fxNamesAY2[] = {
    // Mixer and noise
    {fxAYM, "AYM"}, {fxNOI, "NOI"}, {fxNOA, "NOA"},
    // Tone
    {fxTNN, "TNN"}, {fxTNP, "TNP"}, {fxTNF, "TNF"}, {fxTRT, "TRT"},
    // Envelope
    {fxEAU, "EAU"}, {fxENN, "ENN"}, {fxENP, "ENP"}, {fxENF, "ENF"}, {fxERT, "ERT"},
    // Software osc
    {fxSFT, "SFT"}, {fxSFN, "SFN"}, {fxSFP, "SFP"}, {fxSFF, "SFF"}, {fxSRT, "SRT"},
    {fxSFM, "SFM"}, {fxPWM, "PWM"}, {fxSPL, "SPL"}, {fxSWT, "SWT"},
  };
  int fxAY2Count = sizeof(fxNamesAY2) / sizeof(FXName);

  // AYSample FX
  FXName fxNamesAYSample[] = {
    // Mixer and noise
    {fxAYM, "AYM"}, {fxNOI, "NOI"}, {fxNOA, "NOA"},
    // Tone
    {fxTNN, "TNN"}, {fxTNP, "TNP"}, {fxTNF, "TNF"}, {fxTRT, "TRT"},
    // Software osc (unified with AY2)
    {fxSFN, "SFN"}, {fxSFP, "SFP"}, {fxSFF, "SFF"},
    // Sample-specific
    {fxSMS, "SMS"},
  };
  int fxAYSampleCount = sizeof(fxNamesAYSample) / sizeof(FXName);

  // FX Groups array. FX counts are filled in fillFXNames()
  FXGroup fxGroups[] = {
    {"Sequencer FX", fxNamesSequencer, 0, 8, InstrumentType::none},
    {"Modulation FX", fxNamesModulation, 0, 5, InstrumentType::none},
    {"AY Classic FX", fxNamesAY1, 0, 8, InstrumentType::AY1},
    {"AY Plus FX", fxNamesAY2, 0, 8, InstrumentType::AY2},
    {"AYSample FX", fxNamesAYSample, 0, 8, InstrumentType::AYSample},
  };
  int fxGroupCount = sizeof(fxGroups) / sizeof(FXGroup);

  // Fill FX names
  //
  // Idempotent: safe to call multiple times and from multiple entry points
  // (Engine construction, projectLoad, tests). The first call populates the
  // global fxNames[] lookup and the per-group counts in fxGroups[]; subsequent
  // calls are no-ops. This decouples the FX-name table from the Engine lifecycle
  // so project loading always has valid FX names/counts regardless of ordering.
  void fillFXNames() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    // Initialize all FX names to "---"
    for (int c = 0; c < 256; c++) {
      strcpy(fxNames[c].name, "---");
      fxNames[c].fx = (enum FX)c;
    }

    // Fill counts in fxGroups array
    fxGroups[0].count = fxSequencerCount;
    fxGroups[1].count = fxModulationCount;
    fxGroups[2].count = fxAY1Count;
    fxGroups[3].count = fxAY2Count;
    fxGroups[4].count = fxAYSampleCount;

    // Fill FX names from all groups
    for (int g = 0; g < fxGroupCount; g++) {
      for (int i = 0; i < fxGroups[g].count; i++) {
        enum FX fx = fxGroups[g].fxList[i].fx;
        strcpy(fxNames[fx].name, fxGroups[g].fxList[i].name);
        fxNames[fx].fx = fx;
      }
    }
  }

  // Initialize project
  Project::Project() {
    title[0] = 0;
    author[0] = 0;
    linearPitch = 1;

    // Init song structure
    for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
      for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
        song[c][d] = EMPTY_VALUE_16;
        songHighlight[c][d] = 0;
      }
    }

    // Init chains
    for (int c = 0; c < PROJECT_MAX_CHAINS; c++) clearChain(c);

    // Init grooves
    for (int c = 0; c < PROJECT_MAX_GROOVES; c++) {
      for (int d = 0; d < 16; d++) grooves[c].speed[d] = EMPTY_VALUE_8;
    }

    // Default groove
    grooves[0].speed[0] = 6;
    grooves[0].speed[1] = 6;

    // Init phrases, instruments, and tables
    for (int c = 0; c < PROJECT_MAX_PHRASES; c++) clearPhrase(c);
    for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) clearInstrument(c);
    for (int c = 0; c < PROJECT_MAX_TABLES; c++) clearTable(c);

    // Init AY wavetables
    memset(ayWavetables, 0, sizeof(ayWavetables));
  }

  Project::~Project() {
    // Because instruments can have dynamically allocated sample data, we need to clear them
    for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) clearInstrument(c);
  }

  void Project::initAYproject() {
    linearPitch = 1; // Default to linear pitch table
    tickRate = 50;
    chipType = ChipType::AY;
    chipsCount = 1;
    chipSetup.ay = {
      .clock = 1773400,
      .isYM = 0,
      .stereoMode = StereoModeAY::ABC,
      .stereoSeparation = 50,
      .pwmFullRange = 0, // Default to hardware-accurate 16 steps
    };

    tracksCount = getTotalTracksCount();

    calculatePitchTable();
  }

  // Create 12TET scale
  void Project::calculatePitchTableAY() {
    float clock = (float)(chipSetup.ay.clock);
    int octaves = 9;
    int startMidiNote = 12; // Start from C-0 (MIDI 12)

    snprintf(pitchTable.name, sizeof(pitchTable.name), "12TET %dHz", chipSetup.ay.clock);
    pitchTable.length = octaves * 12;
    pitchTable.octaveSize = 12;

    for (int o = 0; o < octaves; o++) {
      for (int c = 0; c < 12; c++) {
        noteStrings[c][2] = 48 + o;

        int midiNote = startMidiNote + o * 12 + c;
        float freq = centsToFrequency(midiNote * 100);
        int period = frequencyToAYPeriod(freq, (int)clock);

        pitchTable.values[o * 12 + c] = period;
        strcpy(pitchTable.noteNames[o * 12 + c], noteStrings[c]);
      }
    }
  }

  // Create 12TET linear pitch table (values in cents)
  void Project::calculateLinearPitchTable12TET() {
    strcpy(pitchTable.name, "12TET Linear");
    int octaves = 9;
    pitchTable.length = octaves * 12;
    pitchTable.octaveSize = 12;

    int startMidiNote = 12; // Start from C-0 (MIDI 12)

    for (int o = 0; o < octaves; o++) {
      for (int c = 0; c < 12; c++) {
        noteStrings[c][2] = 48 + o; // ASCII '0' + octave number

        int midiNote = startMidiNote + o * 12 + c;
        pitchTable.values[o * 12 + c] = midiNote * 100; // Value in cents
        strcpy(pitchTable.noteNames[o * 12 + c], noteStrings[c]);
      }
    }
  }

  // Reinitialize pitch table based on linear pitch setting
  void Project::calculatePitchTable() {
    if (linearPitch) {
      calculateLinearPitchTable12TET();
    } else {
      calculatePitchTableAY();
    }
  }

  // Is chain empty?
  bool Project::isChainEmpty(int chain) {
    for (int c = 0; c < 16; c++) {
      if (chains[chain].rows[c].phrase != EMPTY_VALUE_16) return false;
    }
    return true;
  }

  // Is phrase empty?
  bool Project::isPhraseEmpty(int phrase) {
    for (int c = 0; c < 16; c++) {
      if (phrases[phrase].rows[c].note != EMPTY_VALUE_8) return false;
      if (phrases[phrase].rows[c].instrument != EMPTY_VALUE_8) return false;
      if (phrases[phrase].rows[c].volume != EMPTY_VALUE_8) return false;
      for (int d = 0; d < 3; d++) {
        if (phrases[phrase].rows[c].fx[d][0] != EMPTY_VALUE_8) return false;
        if (phrases[phrase].rows[c].fx[d][1] != 0) return false;
      }
    }

    return true;
  }

  // Is instrument empty?
  bool Project::isInstrumentEmpty(int instrument) {
    return instruments[instrument].type == InstrumentType::none;
  }

  // Is table empty?
  bool Project::isTableEmpty(int table) {
    for (int c = 0; c < 16; c++) {
      if (tables[table].rows[c].pitchFlag != 0) return false;
      if (tables[table].rows[c].pitchOffset != 0) return false;
      if (tables[table].rows[c].volume != EMPTY_VALUE_8) return false;
      for (int d = 0; d < 4; d++) {
        if (tables[table].rows[c].fx[d][0] != EMPTY_VALUE_8) return false;
        if (tables[table].rows[c].fx[d][1] != 0) return false;
      }
    }

    return true;
  }

  // Is groove empty?
  bool Project::isGrooveEmpty(int groove) {
    for (int c = 0; c < 16; c++) {
      if (grooves[groove].speed[c] != EMPTY_VALUE_8) return false;
    }
    return true;
  }

  // Is wavetable empty?
  bool Project::isWavetableEmpty(int wavetable) {
    for (int c = 0; c < 32; c++) {
      if (ayWavetables[wavetable][c] != 0) return false;
    }
    return true;
  }

  // Note name in phrase
  const char* Project::noteName(uint8_t note) {
    if (note == NOTE_OFF) {
      return "OFF";
    } else if (note < pitchTable.length) {
      return pitchTable.noteNames[note];
    } else {
      return "---";
    }
  }

  // Get number of tracks for a chip at index
  int Project::getChipTracksCount(int chipIndex) {
    // TODO: Hardcoded for AY chips (3 channels) for now
    return 3;
  }

  // Get total number of tracks for the project
  int Project::getTotalTracksCount() {
    int totalTracks = 0;
    for (int i = 0; i < chipsCount; i++) {
      totalTracks += getChipTracksCount(i);
    }
    return totalTracks;
  }

  // Clear a single phrase with proper initialization
  void Project::clearPhrase(int phrase) {
    for (int d = 0; d < 16; d++) {
      phrases[phrase].rows[d].note = EMPTY_VALUE_8;
      phrases[phrase].rows[d].instrument = EMPTY_VALUE_8;
      phrases[phrase].rows[d].volume = EMPTY_VALUE_8;
      for (int e = 0; e < 3; e++) {
        phrases[phrase].rows[d].fx[e][0] = EMPTY_VALUE_8;
        phrases[phrase].rows[d].fx[e][1] = 0;
      }
    }
  }

  // Clear a single chain with proper initialization
  void Project::clearChain(int chain) {
    for (int d = 0; d < 16; d++) {
      chains[chain].rows[d].phrase = EMPTY_VALUE_16;
      chains[chain].rows[d].transpose = 0;
    }
  }

  // Clear a single instrument with proper initialization
  void Project::clearInstrument(int instrument) {
    getInstrumentFunctions(instruments[instrument].type).free(&instruments[instrument]);
    getInstrumentFunctions(InstrumentType::none).init(&instruments[instrument]);
  }

  // Clear a single table with proper initialization
  void Project::clearTable(int table) {
    for (int d = 0; d < 16; d++) {
      tables[table].rows[d].pitchFlag = 0;
      tables[table].rows[d].pitchOffset = 0;
      tables[table].rows[d].volume = EMPTY_VALUE_8;
      for (int e = 0; e < 4; e++) {
        tables[table].rows[d].fx[e][0] = EMPTY_VALUE_8;
        tables[table].rows[d].fx[e][1] = 0;
      }
    }
  }

  // Does chain have notes?
  bool Project::chainHasNotes(int chain) {
    bool v = false;
    for (int c = 0; c < 16; c++) {
      int phrase = chains[chain].rows[c].phrase;
      if (phrase != EMPTY_VALUE_16) v = phraseHasNotes(phrase);
      if (v) break;
    }
    return v;
  }

  // Does phrase have notes?
  bool Project::phraseHasNotes(int phrase) {
    bool v = false;
    for (int c = 0; c < 16; c++) {
      if (phrases[phrase].rows[c].note < PROJECT_MAX_PITCHES) {
        v = true;
        break;
      }
    }
    return v;
  }

  // Instrument name
  const char* Project::instrumentName(uint8_t instrument) {
    if (instruments[instrument].type == InstrumentType::none) return "None";
    if (strlen(instruments[instrument].name) == 0) {
      return instrumentTypeName(instruments[instrument].type);
    } else {
      return instruments[instrument].name;
    }
  }

  // Instrument type name
  const char* Project::instrumentTypeName(InstrumentType type) {
    switch (type) {
      case InstrumentType::AY1:
        return "AY Classic";
      case InstrumentType::AY2:
        return "AY Plus";
      case InstrumentType::AYSample:
        return "AY Sample";
      case InstrumentType::none:
        return "None";
      default:
        return "";
    }
  }
};
