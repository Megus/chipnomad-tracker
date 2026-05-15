#include <stdio.h>
#include <string.h>
#include "project.h"
#include "corelib/corelib_file.h"
#include "utils.h"

FXName fxNames[256];

// FX Names (in the order as they appear in FX select screen)
FXName fxNamesCommon[] = {
  {fxARP, "ARP"}, {fxARC, "ARC"}, {fxPVB, "PVB"}, {fxPBN, "PBN"}, {fxPSL, "PSL"},
  {fxPIT, "PIT"}, {fxFIN, "FIN"}, {fxPRD, "PRD"}, {fxVOL, "VOL"}, {fxRET, "RET"},
  {fxDEL, "DEL"}, {fxOFF, "OFF"}, {fxKIL, "KIL"}, {fxTIC, "TIC"}, {fxTBL, "TBL"},
  {fxTBX, "TBX"}, {fxTHO, "THO"}, {fxTXH, "TXH"}, {fxGRV, "GRV"}, {fxGGR, "GGR"},
  {fxHOP, "HOP"}, {fxSNG, "SNG"}
};
int fxCommonCount = sizeof(fxNamesCommon) / sizeof(FXName);

FXName fxNamesAY[] = {
  {fxAYM, "AYM"}, {fxERT, "ERT"}, {fxNOI, "NOI"}, {fxNOA, "NOA"}, {fxEAU, "EAU"},
  {fxEVB, "EVB"}, {fxEBN, "EBN"}, {fxESL, "ESL"}, {fxENT, "ENT"}, {fxEPT, "EPT"},
  {fxEPL, "EPL"}, {fxEPH, "EPH"},
};
int fxAYCount = sizeof(fxNamesAY) / sizeof(FXName);

// Fill FX names
void fillFXNames() {
  for (int c = 0; c < 256; c++) {
    strcpy(fxNames[c].name, "---");
    fxNames[c].fx = c;
  }

  for (int c = 0; c < fxCommonCount; c++) {
    strcpy(fxNames[fxNamesCommon[c].fx].name, fxNamesCommon[c].name);
    fxNames[fxNamesCommon[c].fx].fx = fxNamesCommon[c].fx;
  }

  for (int c = 0; c < fxAYCount; c++) {
    strcpy(fxNames[fxNamesAY[c].fx].name, fxNamesAY[c].name);
    fxNames[fxNamesAY[c].fx].fx = fxNamesAY[c].fx;
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
  instrument->type = instNone;
  instrument->name[0] = 0;
  instrument->chip.ay.defaultMixer = 0x01; // Default to tone only
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
