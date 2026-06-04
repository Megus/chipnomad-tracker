#include <stdio.h>
#include <string.h>
#include "project.h"
#include "project_instruments.h"
#include "utils.h"

FXName fxNames[256];

// FX Names organized by groups (in the order as they appear in FX select screen)

// Sequencer FX (renamed from fxNamesCommon)
FXName fxNamesSequencer[] = {
  {fxARP, "ARP"}, {fxARC, "ARC"}, {fxPVB, "PVB"}, {fxPBN, "PBN"}, {fxPSL, "PSL"},
  {fxPIT, "PIT"}, {fxFIN, "FIN"}, {fxPRD, "PRD"}, {fxVOL, "VOL"}, {fxRET, "RET"},
  {fxDEL, "DEL"}, {fxOFF, "OFF"}, {fxKIL, "KIL"}, {fxTIC, "TIC"}, {fxTBL, "TBL"},
  {fxTBX, "TBX"}, {fxTHO, "THO"}, {fxTXH, "TXH"}, {fxGRV, "GRV"}, {fxGGR, "GGR"},
  {fxHOP, "HOP"}, {fxSNG, "SNG"}
};
int fxSequencerCount = sizeof(fxNamesSequencer) / sizeof(FXName);

// Modulation FX (new group)
FXName fxNamesModulation[] = {
  {fxM1A, "M1A"}, {fxM11, "M11"}, {fxM12, "M12"}, {fxM13, "M13"}, {fxM14, "M14"},
  {fxM2A, "M2A"}, {fxM21, "M21"}, {fxM22, "M22"}, {fxM23, "M23"}, {fxM24, "M24"},
  {fxM3A, "M3A"}, {fxM31, "M31"}, {fxM32, "M32"}, {fxM33, "M33"}, {fxM34, "M34"},
  {fxM4A, "M4A"}, {fxM41, "M41"}, {fxM42, "M42"}, {fxM43, "M43"}, {fxM44, "M44"}
};
int fxModulationCount = sizeof(fxNamesModulation) / sizeof(FXName);

// AY1 FX
FXName fxNamesAY1[] = {
  {fxAYM, "AYM"}, {fxNOI, "NOI"}, {fxNOA, "NOA"}, {fxERT, "ERT"},{fxEAU, "EAU"},
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
  {fxSFT, "SFT"}, {fxSFV, "SFV"}, {fxSFN, "SFN"}, {fxSFP, "SFP"}, {fxSFF, "SFF"},
  {fxSRT, "SRT"},
};
int fxAY2Count = sizeof(fxNamesAY2) / sizeof(FXName);

// AYSample FX
FXName fxNamesAYSample[] = {
  // Mixer and noise
  {fxAYM, "AYM"}, {fxNOI, "NOI"}, {fxNOA, "NOA"},
  // Tone
  {fxTNN, "TNN"}, {fxTNP, "TNP"}, {fxTNF, "TNF"}, {fxTRT, "TRT"},
  // Software osc (unified with AY2)
  {fxSFN, "SFN"}, {fxSFP, "SFP"}, {fxSFF, "SFF"}, {fxSRT, "SRT"},
  // Sample-specific
  {fxSMS, "SMS"},
};
int fxAYSampleCount = sizeof(fxNamesAYSample) / sizeof(FXName);

// FX Groups array. FX counts are filled in fillFXNames()
FXGroup fxGroups[] = {
  {"Sequencer FX", fxNamesSequencer, 0, 8, instNone},
  {"Modulation FX", fxNamesModulation, 0, 5, instNone},
  {"AY Classic FX", fxNamesAY1, 0, 8, instAY1},
  {"AY Plus FX", fxNamesAY2, 0, 8, instAY2},
  {"AYSample FX", fxNamesAYSample, 0, 8, instAYSample},
};
int fxGroupCount = sizeof(fxGroups) / sizeof(FXGroup);

// Fill FX names
void fillFXNames() {
  // Initialize all FX names to "---"
  for (int c = 0; c < 256; c++) {
    strcpy(fxNames[c].name, "---");
    fxNames[c].fx = c;
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
void projectInit(Project* p) {
  // Title
  strcpy(p->title, "");
  strcpy(p->author, "");
  p->linearPitch = 0;

  // Clean song structure
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      p->song[c][d] = EMPTY_VALUE_16;
      p->songHighlight[c][d] = 0;
    }
  }

  // Clean chains
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    chainClear(&p->chains[c]);
  }

  // Clean grooves
  for (int c = 0; c < PROJECT_MAX_GROOVES; c++) {
    for (int d = 0; d < 16; d++) {
      p->grooves[c].speed[d] = EMPTY_VALUE_8;
    }
  }

  // Default groove
  p->grooves[0].speed[0] = 6;
  p->grooves[0].speed[1] = 6;

  // Clean phrases
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    phraseClear(&p->phrases[c]);
  }

  // Clean instruments
  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    instrumentClear(&p->instruments[c]);
  }

  // Clean tables
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    tableClear(&p->tables[c]);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Convenience functions

// Is chain empty?
int8_t chainIsEmpty(Project* project, int chain) {
  for (int c = 0; c < 16; c++) {
    if (project->chains[chain].rows[c].phrase != EMPTY_VALUE_16) return 0;
  }
  return 1;
}

// Is phrase empty?
int8_t phraseIsEmpty(Project* project, int phrase) {
  for (int c = 0; c < 16; c++) {
    if (project->phrases[phrase].rows[c].note != EMPTY_VALUE_8) return 0;
    if (project->phrases[phrase].rows[c].instrument != EMPTY_VALUE_8) return 0;
    if (project->phrases[phrase].rows[c].volume != EMPTY_VALUE_8) return 0;
    for (int d = 0; d < 3; d++) {
      if (project->phrases[phrase].rows[c].fx[d][0] != EMPTY_VALUE_8) return 0;
      if (project->phrases[phrase].rows[c].fx[d][1] != 0) return 0;
    }
  }

  return 1;
}

// Is instrument empty?
int8_t instrumentIsEmpty(Project* project, int instrument) {
  return project->instruments[instrument].type == instNone;
}

// Is table empty?
int8_t tableIsEmpty(Project* project, int table) {
  for (int c = 0; c < 16; c++) {
    if (project->tables[table].rows[c].pitchFlag != 0) return 0;
    if (project->tables[table].rows[c].pitchOffset != 0) return 0;
    if (project->tables[table].rows[c].volume != EMPTY_VALUE_8) return 0;
    for (int d = 0; d < 4; d++) {
      if (project->tables[table].rows[c].fx[d][0] != EMPTY_VALUE_8) return 0;
      if (project->tables[table].rows[c].fx[d][1] != 0) return 0;
    }
  }

  return 1;
}

// Is groove empty?
int8_t grooveIsEmpty(Project* project, int groove) {
  for (int c = 0; c < 16; c++) {
    if (project->grooves[groove].speed[c] != EMPTY_VALUE_8) return 0;
  }
  return 1;
}

// Note name in phrase
char* noteName(Project* project, uint8_t note) {
  if (note == NOTE_OFF) {
    return "OFF";
  } else if (note < project->pitchTable.length) {
    return project->pitchTable.noteNames[note];
  } else {
    return "---";
  }
}

// Get number of tracks for a chip at index
int projectGetChipTracks(Project* p, int chipIndex) {
  // Hardcoded for AY chips (3 channels) for now
  return 3;
}

// Get total number of tracks for the project
int projectGetTotalTracks(Project* p) {
  int totalTracks = 0;
  for (int i = 0; i < p->chipsCount; i++) {
    totalTracks += projectGetChipTracks(p, i);
  }
  return totalTracks;
}

// Clear a single phrase with proper initialization
void phraseClear(Phrase* phrase) {
  for (int d = 0; d < 16; d++) {
    phrase->rows[d].note = EMPTY_VALUE_8;
    phrase->rows[d].instrument = EMPTY_VALUE_8;
    phrase->rows[d].volume = EMPTY_VALUE_8;
    for (int e = 0; e < 3; e++) {
      phrase->rows[d].fx[e][0] = EMPTY_VALUE_8;
      phrase->rows[d].fx[e][1] = 0;
    }
  }
}

// Clear a single chain with proper initialization
void chainClear(Chain* chain) {
  for (int d = 0; d < 16; d++) {
    chain->rows[d].phrase = EMPTY_VALUE_16;
    chain->rows[d].transpose = 0;
  }
}

// Clear a single instrument with proper initialization
void instrumentClear(Instrument* instrument) {
  getInstrumentFunctions(instrument->type).free(instrument);
  getInstrumentFunctions(instNone).init(instrument);
}

// Clear a single table with proper initialization
void tableClear(Table* table) {
  for (int d = 0; d < 16; d++) {
    table->rows[d].pitchFlag = 0;
    table->rows[d].pitchOffset = 0;
    table->rows[d].volume = EMPTY_VALUE_8;
    for (int e = 0; e < 4; e++) {
      table->rows[d].fx[e][0] = EMPTY_VALUE_8;
      table->rows[d].fx[e][1] = 0;
    }
  }
}
