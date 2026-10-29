#include "project_utils.h"
#include "pitch_table_utils.h"
#include <string.h>

void projectInitAY(chipnomad::Project& p) {
  using namespace chipnomad;
  projectInit(&p);

  p.linearPitch = 1; // Default to linear pitch table
  p.tickRate = 50;
  p.chipType = ChipType::AY;
  p.chipsCount = 1;
  p.chipSetup.ay = {
    .clock = 1773400,
    .isYM = 0,
    .stereoMode = StereoModeAY::ABC,
    .stereoSeparation = 50,
    .pwmFullRange = 0, // Default to hardware-accurate 16 steps
  };

  p.tracksCount = projectGetTotalTracks(&p);

  calculateLinearPitchTable12TET(p); // Linear pitch table
  //calculatePitchTableAY(project); // AY period pitch table
}

// Does chain have notes?
int8_t chainHasNotes(chipnomad::Project& p, int chain) {
  int8_t v = 0;
  for (int c = 0; c < 16; c++) {
    int phrase = p.chains[chain].rows[c].phrase;
    if (phrase != EMPTY_VALUE_16) v = phraseHasNotes(p, phrase);
    if (v == 1) break;
  }

  return v;
}

// Does phrase have notes?
int8_t phraseHasNotes(chipnomad::Project& p, int phrase) {
  int8_t v = 0;
  for (int c = 0; c < 16; c++) {
    if (p.phrases[phrase].rows[c].note < PROJECT_MAX_PITCHES) {
      v = 1;
      break;
    }
  }

  return v;
}

// Instrument name
const char* instrumentName(chipnomad::Project& p, uint8_t instrument) {
  using namespace chipnomad;
  if (p.instruments[instrument].type == InstrumentType::none) return "None";
  if (strlen(p.instruments[instrument].name) == 0) {
    return instrumentTypeName(p.instruments[instrument].type);
  } else {
    return p.instruments[instrument].name;
  }
}

// Instrument type name
const char* instrumentTypeName(chipnomad::InstrumentType type) {
  using namespace chipnomad;
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

// Get first note used with an instrument in the project
uint8_t instrumentFirstNote(chipnomad::Project& p, uint8_t instrument) {
  using namespace chipnomad;
  // Search through all phrases for first use of this instrument
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    if (phraseIsEmpty(&p, c)) continue;
    for (int row = 0; row < 16; row++) {
      if (p.phrases[c].rows[row].instrument == instrument && p.phrases[c].rows[row].note < p.pitchTable.length) {
        return p.phrases[c].rows[row].note;
      }
    }
  }
  // Default to C in 4th octave if not found
  return p.pitchTable.octaveSize * 4;
}

// Swap instrument references in all phrases
static void instrumentSwapReferences(chipnomad::Project& p, uint8_t inst1, uint8_t inst2) {
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    for (int d = 0; d < 16; d++) {
      uint8_t inst = p.phrases[c].rows[d].instrument;
      if (inst == inst1) {
        p.phrases[c].rows[d].instrument = inst2;
      } else if (inst == inst2) {
        p.phrases[c].rows[d].instrument = inst1;
      }
    }
  }
}

// Swap two instruments and their default tables
void instrumentSwap(chipnomad::Project& p, uint8_t inst1, uint8_t inst2) {
  using namespace chipnomad;
  if (inst1 >= PROJECT_MAX_INSTRUMENTS || inst2 >= PROJECT_MAX_INSTRUMENTS || inst1 == inst2) {
    return;
  }

  // Swap instruments
  Instrument temp = p.instruments[inst1];
  p.instruments[inst1] = p.instruments[inst2];
  p.instruments[inst2] = temp;

  // Swap default tables (table number matches instrument number)
  Table tempTable = p.tables[inst1];
  p.tables[inst1] = p.tables[inst2];
  p.tables[inst2] = tempTable;

  // Swap instrument references in phrases
  instrumentSwapReferences(p, inst1, inst2);
}

// Check if chain is used in the song
static int chainIsUsedInSong(chipnomad::Project& p, int chainIdx) {
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      if (p.song[c][d] == chainIdx) return 1;
    }
  }
  return 0;
}

// Check if phrase is used in any chain
static int phraseIsUsedInChains(chipnomad::Project& p, int phraseIdx) {
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    for (int d = 0; d < 16; d++) {
      if (p.chains[c].rows[d].phrase == phraseIdx) return 1;
    }
  }
  return 0;
}

// Check if instrument is used in any phrase
static int instrumentIsUsedInPhrases(chipnomad::Project& p, int instrumentIdx) {
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    for (int d = 0; d < 16; d++) {
      if (p.phrases[c].rows[d].instrument == instrumentIdx) return 1;
    }
  }
  return 0;
}

// Find empty chain slot (empty and not used in project)
int findEmptyChain(chipnomad::Project& p, int start) {
  for (int c = start; c < PROJECT_MAX_CHAINS; c++) {
    if (chipnomad::chainIsEmpty(&p, c) && !chainIsUsedInSong(p, c)) return c;
  }
  return EMPTY_VALUE_16;
}

// Find empty phrase slot (empty and not used in project)
int findEmptyPhrase(chipnomad::Project& p, int start) {
  for (int c = start; c < PROJECT_MAX_PHRASES; c++) {
    if (chipnomad::phraseIsEmpty(&p, c) && !phraseIsUsedInChains(p, c)) return c;
  }
  return EMPTY_VALUE_16;
}

// Find empty instrument slot
int findEmptyInstrument(chipnomad::Project& p, int start) {
  using namespace chipnomad;
  for (int c = start; c < PROJECT_MAX_INSTRUMENTS; c++) {
    if (p.instruments[c].type == InstrumentType::none) return c;
  }
  return EMPTY_VALUE_8;
}

// Cleanup unused phrases - returns count of cleaned phrases
int cleanupUnusedPhrases(chipnomad::Project& p) {
  int cleanedCount = 0;

  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    // Skip if phrase is already empty
    if (phraseIsEmpty(&p, c)) continue;

    // Check if phrase is used in any chain
    if (!phraseIsUsedInChains(p, c)) {
      // Clear the unused phrase properly
      chipnomad::phraseClear(&p.phrases[c]);
      cleanedCount++;
    }
  }

  return cleanedCount;
}

// Cleanup unused chains - returns count of cleaned chains
int cleanupUnusedChains(chipnomad::Project& p) {
  int cleanedCount = 0;

  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    // Skip if chain is already empty
    if (chipnomad::chainIsEmpty(&p, c)) continue;

    // Check if chain is used in the song
    if (!chainIsUsedInSong(p, c)) {
      // Clear the unused chain properly
      chipnomad::chainClear(&p.chains[c]);
      cleanedCount++;
    }
  }

  return cleanedCount;
}

// Cleanup unused instruments - returns count of cleaned instruments
int cleanupUnusedInstruments(chipnomad::Project& p) {
  using namespace chipnomad;
  int cleanedCount = 0;

  for (int c = 0; c < PROJECT_MAX_INSTRUMENTS; c++) {
    // Skip if instrument is already empty
    if (instrumentIsEmpty(&p, c)) continue;

    // Check if instrument is used in any phrase
    if (!instrumentIsUsedInPhrases(p, c)) {
      // Clear the unused instrument and its table properly
      instrumentClear(&p.instruments[c]);
      tableClear(&p.tables[c]);
      cleanedCount++;
    }
  }

  return cleanedCount;
}

// Update chain references in song
static void updateChainReferences(chipnomad::Project& p, int oldChain, int newChain) {
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      if (p.song[c][d] == oldChain) {
        p.song[c][d] = newChain;
      }
    }
  }
}

// Remove duplicate chains - returns count of removed duplicates
int removeDuplicateChains(chipnomad::Project& p) {
  using namespace chipnomad;
  int removedCount = 0;

  for (int c = 0; c < PROJECT_MAX_CHAINS - 1; c++) {
    // Skip if chain is empty
    if (chainIsEmpty(&p, c)) continue;

    for (int d = c + 1; d < PROJECT_MAX_CHAINS; d++) {
      // Skip if chain is empty
      if (chainIsEmpty(&p, d)) continue;

      // Compare chains using memcmp
      if (memcmp(&p.chains[c], &p.chains[d], sizeof(Chain)) == 0) {
        // Update all references to chain j to point to chain i
        updateChainReferences(p, d, c);

        // Clear the duplicate chain
        chainClear(&p.chains[d]);
        removedCount++;
      }
    }
  }

  return removedCount;
}

// Update phrase references in chains
static void updatePhraseReferences(chipnomad::Project& p, int oldPhrase, int newPhrase) {
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    for (int d = 0; d < 16; d++) {
      if (p.chains[c].rows[d].phrase == oldPhrase) {
        p.chains[c].rows[d].phrase = newPhrase;
      }
    }
  }
}

// Remove duplicate phrases - returns count of removed duplicates
int removeDuplicatePhrases(chipnomad::Project& p) {
  using namespace chipnomad;
  int removedCount = 0;

  for (int c = 0; c < PROJECT_MAX_PHRASES - 1; c++) {
    // Skip if phrase is empty
    if (phraseIsEmpty(&p, c)) continue;

    for (int d = c + 1; d < PROJECT_MAX_PHRASES; d++) {
      // Skip if phrase is empty
      if (phraseIsEmpty(&p, d)) continue;

      // Compare phrases using memcmp
      if (memcmp(&p.phrases[c], &p.phrases[d], sizeof(Phrase)) == 0) {
        // Update all references to phrase j to point to phrase i
        updatePhraseReferences(p, d, c);

        // Clear the duplicate phrase
        phraseClear(&p.phrases[d]);
        removedCount++;
      }
    }
  }

  return removedCount;
}

// Check if table is used in TBL/TBX commands
static int tableIsUsedInCommands(chipnomad::Project& p, int tableIdx) {
  using namespace chipnomad;
  // Check phrases for TBL/TBX commands
  for (int c = 0; c < PROJECT_MAX_PHRASES; c++) {
    for (int d = 0; d < 16; d++) {
      for (int fx = 0; fx < 3; fx++) {
        if ((p.phrases[c].rows[d].fx[fx][0] == fxTBL ||
             p.phrases[c].rows[d].fx[fx][0] == fxTBX) &&
            p.phrases[c].rows[d].fx[fx][1] == tableIdx) {
          return 1;
        }
      }
    }
  }

  // Check tables for TBL/TBX commands
  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    for (int d = 0; d < 16; d++) {
      for (int fx = 0; fx < 4; fx++) {
        if ((p.tables[c].rows[d].fx[fx][0] == fxTBL ||
             p.tables[c].rows[d].fx[fx][0] == fxTBX) &&
            p.tables[c].rows[d].fx[fx][1] == tableIdx) {
          return 1;
        }
      }
    }
  }

  return 0;
}

// Cleanup unused tables - returns count of cleaned tables
int cleanupUnusedTables(chipnomad::Project& p) {
  int cleanedCount = 0;

  for (int c = 0; c < PROJECT_MAX_TABLES; c++) {
    // Skip if table is already empty
    if (tableIsEmpty(&p, c)) continue;

    // For 0x00-0x7f range: table is used if corresponding instrument is not None
    if (c < PROJECT_MAX_INSTRUMENTS && !instrumentIsEmpty(&p, c)) {
      continue; // Table is used as instrument table
    }

    // For full range: check if table is used in TBL/TBX commands
    if (!tableIsUsedInCommands(p, c)) {
      // Clear the unused table properly
      tableClear(&p.tables[c]);
      cleanedCount++;
    }
  }

  return cleanedCount;
}

// Combined cleanup functions for UI

// Cleanup phrases and chains (unused + duplicates)
void cleanupPhrasesAndChains(chipnomad::Project& p, int* phrasesFreed, int* chainsFreed) {
  *phrasesFreed = cleanupUnusedPhrases(p) + removeDuplicatePhrases(p);
  *chainsFreed = cleanupUnusedChains(p) + removeDuplicateChains(p);
}

// Cleanup instruments and tables (unused only)
void cleanupInstrumentsAndTables(chipnomad::Project& p, int* instrumentsFreed, int* tablesFreed) {
  *instrumentsFreed = cleanupUnusedInstruments(p);
  *tablesFreed = cleanupUnusedTables(p);
}

// Check if chain is used elsewhere in the song (excluding specific position)
int isChainUsedElsewhere(chipnomad::Project& p, int chainIdx, int excludeTrack, int excludeRow) {
  for (int c = 0; c < PROJECT_MAX_LENGTH; c++) {
    for (int d = 0; d < PROJECT_MAX_TRACKS; d++) {
      if (c == excludeRow && d == excludeTrack) continue;
      if (p.song[c][d] == chainIdx) return 1;
    }
  }
  return 0;
}

// Check if phrase is used elsewhere in chains (excluding specific position)
int isPhraseUsedElsewhere(chipnomad::Project& p, int phraseIdx, int excludeChain, int excludeRow) {
  for (int c = 0; c < PROJECT_MAX_CHAINS; c++) {
    for (int d = 0; d < 16; d++) {
      if (c == excludeChain && d == excludeRow) continue;
      if (p.chains[c].rows[d].phrase == phraseIdx) return 1;
    }
  }
  return 0;
}



// Look up instrument value at the given position or by searching upward through the track
// Parameters: project, song row, chain row, phrase row, track index
// Returns: instrument number (0-255) or EMPTY_VALUE_8 if not found
uint8_t lookupInstrument(chipnomad::Project& p, int songRow, int chainRow, int phraseRow, int track) {
  int sr = songRow;
  int cr = chainRow;
  int pr = phraseRow;

  do {
    // Get current chain
    uint16_t chainIdx = p.song[sr][track];
    if (chainIdx == EMPTY_VALUE_16) {
      // Move to previous song row
      if (--sr < 0) return EMPTY_VALUE_8;
      cr = 15;
      pr = 15;
      continue;
    }

    // Get current phrase
    uint16_t phraseIdx = p.chains[chainIdx].rows[cr].phrase;
    if (phraseIdx == EMPTY_VALUE_16) {
      // Move to previous chain row
      if (--cr < 0) {
        // Move to previous song row
        if (--sr < 0) return EMPTY_VALUE_8;
        cr = 15;
      }
      pr = 15;
      continue;
    }

    // Scan phrase rows from current position upward
    for (int r = pr; r >= 0; r--) {
      if (p.phrases[phraseIdx].rows[r].instrument != EMPTY_VALUE_8) {
        return p.phrases[phraseIdx].rows[r].instrument;
      }
    }

    // Move to previous chain row
    if (--cr < 0) {
      // Move to previous song row
      if (--sr < 0) return EMPTY_VALUE_8;
      cr = 15;
    }
    pr = 15;

  } while (1);
}
