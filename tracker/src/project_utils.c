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

// Check if instrument is used in any phrase
static int instrumentIsUsedInPhrases(Project* project, int instrumentIdx) {
  for (int p = 0; p < PROJECT_MAX_PHRASES; p++) {
    for (int row = 0; row < 16; row++) {
      if (project->phrases[p].rows[row].instrument == instrumentIdx) return 1;
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

// Cleanup unused phrases - returns count of cleaned phrases
int cleanupUnusedPhrases(Project* project) {
  int cleanedCount = 0;
  
  for (int i = 0; i < PROJECT_MAX_PHRASES; i++) {
    // Skip if phrase is already empty
    if (phraseIsEmpty(project, i)) continue;
    
    // Check if phrase is used in any chain
    if (!phraseIsUsedInChains(project, i)) {
      // Clear the unused phrase properly
      phraseClear(&project->phrases[i]);
      cleanedCount++;
    }
  }
  
  return cleanedCount;
}

// Cleanup unused chains - returns count of cleaned chains
int cleanupUnusedChains(Project* project) {
  int cleanedCount = 0;
  
  for (int i = 0; i < PROJECT_MAX_CHAINS; i++) {
    // Skip if chain is already empty
    if (chainIsEmpty(project, i)) continue;
    
    // Check if chain is used in the song
    if (!chainIsUsedInSong(project, i)) {
      // Clear the unused chain properly
      chainClear(&project->chains[i]);
      cleanedCount++;
    }
  }
  
  return cleanedCount;
}

// Cleanup unused instruments - returns count of cleaned instruments
int cleanupUnusedInstruments(Project* project) {
  int cleanedCount = 0;
  
  for (int i = 0; i < PROJECT_MAX_INSTRUMENTS; i++) {
    // Skip if instrument is already empty
    if (instrumentIsEmpty(project, i)) continue;
    
    // Check if instrument is used in any phrase
    if (!instrumentIsUsedInPhrases(project, i)) {
      // Clear the unused instrument and its table properly
      instrumentClear(&project->instruments[i]);
      tableClear(&project->tables[i]);
      cleanedCount++;
    }
  }
  
  return cleanedCount;
}

// Update chain references in song
static void updateChainReferences(Project* project, int oldChain, int newChain) {
  for (int row = 0; row < PROJECT_MAX_LENGTH; row++) {
    for (int col = 0; col < PROJECT_MAX_TRACKS; col++) {
      if (project->song[row][col] == oldChain) {
        project->song[row][col] = newChain;
      }
    }
  }
}

// Remove duplicate chains - returns count of removed duplicates
int removeDuplicateChains(Project* project) {
  int removedCount = 0;
  
  for (int i = 0; i < PROJECT_MAX_CHAINS - 1; i++) {
    // Skip if chain is empty
    if (chainIsEmpty(project, i)) continue;
    
    for (int j = i + 1; j < PROJECT_MAX_CHAINS; j++) {
      // Skip if chain is empty
      if (chainIsEmpty(project, j)) continue;
      
      // Compare chains using memcmp
      if (memcmp(&project->chains[i], &project->chains[j], sizeof(Chain)) == 0) {
        // Update all references to chain j to point to chain i
        updateChainReferences(project, j, i);
        
        // Clear the duplicate chain
        chainClear(&project->chains[j]);
        removedCount++;
      }
    }
  }
  
  return removedCount;
}

// Update phrase references in chains
static void updatePhraseReferences(Project* project, int oldPhrase, int newPhrase) {
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    for (int row = 0; row < 16; row++) {
      if (project->chains[c].rows[row].phrase == oldPhrase) {
        project->chains[c].rows[row].phrase = newPhrase;
      }
    }
  }
}

// Remove duplicate phrases - returns count of removed duplicates
int removeDuplicatePhrases(Project* project) {
  int removedCount = 0;
  
  for (int i = 0; i < PROJECT_MAX_PHRASES - 1; i++) {
    // Skip if phrase is empty
    if (phraseIsEmpty(project, i)) continue;
    
    for (int j = i + 1; j < PROJECT_MAX_PHRASES; j++) {
      // Skip if phrase is empty
      if (phraseIsEmpty(project, j)) continue;
      
      // Compare phrases using memcmp
      if (memcmp(&project->phrases[i], &project->phrases[j], sizeof(Phrase)) == 0) {
        // Update all references to phrase j to point to phrase i
        updatePhraseReferences(project, j, i);
        
        // Clear the duplicate phrase
        phraseClear(&project->phrases[j]);
        removedCount++;
      }
    }
  }
  
  return removedCount;
}

// Check if table is used in TBL/TBX commands
static int tableIsUsedInCommands(Project* project, int tableIdx) {
  // Check phrases for TBL/TBX commands
  for (int p = 0; p < PROJECT_MAX_PHRASES; p++) {
    for (int row = 0; row < 16; row++) {
      for (int fx = 0; fx < 3; fx++) {
        if ((project->phrases[p].rows[row].fx[fx][0] == fxTBL || 
             project->phrases[p].rows[row].fx[fx][0] == fxTBX) &&
            project->phrases[p].rows[row].fx[fx][1] == tableIdx) {
          return 1;
        }
      }
    }
  }
  
  // Check tables for TBL/TBX commands
  for (int t = 0; t < PROJECT_MAX_TABLES; t++) {
    for (int row = 0; row < 16; row++) {
      for (int fx = 0; fx < 4; fx++) {
        if ((project->tables[t].rows[row].fx[fx][0] == fxTBL || 
             project->tables[t].rows[row].fx[fx][0] == fxTBX) &&
            project->tables[t].rows[row].fx[fx][1] == tableIdx) {
          return 1;
        }
      }
    }
  }
  
  return 0;
}

// Cleanup unused tables - returns count of cleaned tables
int cleanupUnusedTables(Project* project) {
  int cleanedCount = 0;
  
  for (int i = 0; i < PROJECT_MAX_TABLES; i++) {
    // Skip if table is already empty
    if (tableIsEmpty(project, i)) continue;
    
    // For 0x00-0x7f range: table is used if corresponding instrument is not None
    if (i < PROJECT_MAX_INSTRUMENTS && !instrumentIsEmpty(project, i)) {
      continue; // Table is used as instrument table
    }
    
    // For full range: check if table is used in TBL/TBX commands
    if (!tableIsUsedInCommands(project, i)) {
      // Clear the unused table properly
      tableClear(&project->tables[i]);
      cleanedCount++;
    }
  }
  
  return cleanedCount;
}

// Combined cleanup functions for UI

// Cleanup phrases and chains (unused + duplicates)
void cleanupPhrasesAndChains(Project* project, int* phrasesFreed, int* chainsFreed) {
  *phrasesFreed = cleanupUnusedPhrases(project) + removeDuplicatePhrases(project);
  *chainsFreed = cleanupUnusedChains(project) + removeDuplicateChains(project);
}

// Cleanup instruments and tables (unused only)
void cleanupInstrumentsAndTables(Project* project, int* instrumentsFreed, int* tablesFreed) {
  *instrumentsFreed = cleanupUnusedInstruments(project);
  *tablesFreed = cleanupUnusedTables(project);
}

// Check if chain is used elsewhere in the song (excluding specific position)
int isChainUsedElsewhere(Project* project, int chainIdx, int excludeTrack, int excludeRow) {
  for (int row = 0; row < PROJECT_MAX_LENGTH; row++) {
    for (int col = 0; col < PROJECT_MAX_TRACKS; col++) {
      if (row == excludeRow && col == excludeTrack) continue;
      if (project->song[row][col] == chainIdx) return 1;
    }
  }
  return 0;
}

// Check if phrase is used elsewhere in chains (excluding specific position)
int isPhraseUsedElsewhere(Project* project, int phraseIdx, int excludeChain, int excludeRow) {
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    for (int row = 0; row < 16; row++) {
      if (c == excludeChain && row == excludeRow) continue;
      if (project->chains[c].rows[row].phrase == phraseIdx) return 1;
    }
  }
  return 0;
}


