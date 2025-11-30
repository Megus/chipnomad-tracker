#include "project_utils.h"
#include "pitch_table_utils.h"
#include <string.h>

void projectInitAY(Project* project) {
  projectInit(project);

  project->tickRate = 50;
  project->chipType = chipAY;
  project->chipsCount = 1;
  project->chipSetup.ay = (struct ChipSetupAY){
    .clock = 1750000,
    .isYM = 0,
    .stereoMode = ayStereoABC,
    .stereoSeparation = 50,
  };

  project->tracksCount = projectGetTotalTracks(project);

  calculatePitchTableAY(project);
}

// Does chain have notes?
int8_t chainHasNotes(Project* project, int chain) {
  int8_t v = 0;
  for (int c = 0; c < 16; c++) {
    int phrase = project->chains[chain].rows[c].phrase;
    if (phrase != EMPTY_VALUE_16) v = phraseHasNotes(project, phrase);
    if (v == 1) break;
  }

  return v;
}

// Does phrase have notes?
int8_t phraseHasNotes(Project* project, int phrase) {
  int8_t v = 0;
  for (int c = 0; c < 16; c++) {
    if (project->phrases[phrase].rows[c].note < PROJECT_MAX_PITCHES) {
      v = 1;
      break;
    }
  }

  return v;
}

// Instrument name
char* instrumentName(Project* project, uint8_t instrument) {
  if (project->instruments[instrument].type == instNone) return "None";
  if (strlen(project->instruments[instrument].name) == 0) {
    return instrumentTypeName(project->instruments[instrument].type);
  } else {
    return project->instruments[instrument].name;
  }
}

// Instrument type name
char* instrumentTypeName(uint8_t type) {
  switch (type) {
    case instAY:
    return "AY";
    break;
    case instNone:
    return "None";
    break;
    default:
    return "";
    break;
  }
}

// Get first note used with an instrument in the project
uint8_t instrumentFirstNote(Project* project, uint8_t instrument) {
  // Search through all phrases for first use of this instrument
  for (int p = 0; p < PROJECT_MAX_PHRASES; p++) {
    if (phraseIsEmpty(project, p)) continue;
    for (int row = 0; row < 16; row++) {
      if (project->phrases[p].rows[row].instrument == instrument && project->phrases[p].rows[row].note < project->pitchTable.length) {
        return project->phrases[p].rows[row].note;
      }
    }
  }
  // Default to C in 4th octave if not found
  return project->pitchTable.octaveSize * 4;
}

// Swap instrument references in all phrases
static void instrumentSwapReferences(Project* project, uint8_t inst1, uint8_t inst2) {
  for (int p = 0; p < PROJECT_MAX_PHRASES; p++) {
    for (int row = 0; row < 16; row++) {
      uint8_t inst = project->phrases[p].rows[row].instrument;
      if (inst == inst1) {
        project->phrases[p].rows[row].instrument = inst2;
      } else if (inst == inst2) {
        project->phrases[p].rows[row].instrument = inst1;
      }
    }
  }
}

// Swap two instruments and their default tables
void instrumentSwap(Project* project, uint8_t inst1, uint8_t inst2) {
  if (inst1 >= PROJECT_MAX_INSTRUMENTS || inst2 >= PROJECT_MAX_INSTRUMENTS || inst1 == inst2) {
    return;
  }

  // Swap instruments
  Instrument temp = project->instruments[inst1];
  project->instruments[inst1] = project->instruments[inst2];
  project->instruments[inst2] = temp;

  // Swap default tables (table number matches instrument number)
  Table tempTable = project->tables[inst1];
  project->tables[inst1] = project->tables[inst2];
  project->tables[inst2] = tempTable;

  // Swap instrument references in phrases
  instrumentSwapReferences(project, inst1, inst2);
}

// Check if chain is used in the song
static int chainIsUsedInSong(Project* project, int chainIdx) {
  for (int row = 0; row < PROJECT_MAX_LENGTH; row++) {
    for (int col = 0; col < PROJECT_MAX_TRACKS; col++) {
      if (project->song[row][col] == chainIdx) return 1;
    }
  }
  return 0;
}

// Check if phrase is used in any chain
static int phraseIsUsedInChains(Project* project, int phraseIdx) {
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    for (int row = 0; row < 16; row++) {
      if (project->chains[c].rows[row].phrase == phraseIdx) return 1;
    }
  }
  return 0;
}

// Find empty chain slot (empty and not used in project)
int findEmptyChain(Project* project, int start) {
  for (int i = start; i < PROJECT_MAX_CHAINS; i++) {
    if (chainIsEmpty(project, i) && !chainIsUsedInSong(project, i)) return i;
  }
  return EMPTY_VALUE_16;
}

// Find empty phrase slot (empty and not used in project)
int findEmptyPhrase(Project* project, int start) {
  for (int i = start; i < PROJECT_MAX_PHRASES; i++) {
    if (phraseIsEmpty(project, i) && !phraseIsUsedInChains(project, i)) return i;
  }
  return EMPTY_VALUE_16;
}

// Find empty instrument slot
int findEmptyInstrument(Project* project, int start) {
  for (int i = start; i < PROJECT_MAX_INSTRUMENTS; i++) {
    if (project->instruments[i].type == instNone) return i;
  }
  return EMPTY_VALUE_8;
}
