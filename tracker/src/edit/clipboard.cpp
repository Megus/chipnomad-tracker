#include <string.h>
#include <stdlib.h>
#include "clipboard.h"
#include "project_utils.h"

Clipboard::Clipboard() {
  resetClipboard();
}

void Clipboard::resetClipboard(void) {
  grooveLength = 0;
  songRows = 0;
  songCols = 0;
  chainRows = 0;
  phraseRows = 0;
  tableRows = 0;
  instrumentValid = false;
  wavetableValid = false;
}

void Clipboard::copyGroove(chipnomad::Project &p, int grooveIdx, int startRow, int endRow, int isCut) {
  grooveLength = endRow - startRow + 1;
  for (int row = startRow; row <= endRow; row++) {
    groove.speed[row - startRow] = p.grooves[grooveIdx].speed[row];
    if (isCut) p.grooves[grooveIdx].speed[row] = EMPTY_VALUE_8;
  }
}

int Clipboard::pasteGroove(chipnomad::Project &p, int grooveIdx, int startRow) {
  int rowsPasted = 0;
  for (int row = 0; row < grooveLength && startRow + row < 16; row++) {
    rowsPasted++;
    p.grooves[grooveIdx].speed[startRow + row] = groove.speed[row];
  }
  return rowsPasted;
}

void Clipboard::copySong(chipnomad::Project &p, int startCol, int startRow, int endCol, int endRow, int isCut) {
  songRows = endRow - startRow + 1;
  songCols = endCol - startCol + 1;

  for (int row = 0; row < songRows; row++) {
    for (int col = 0; col < songCols; col++) {
      song[row][col] = p.song[startRow + row][startCol + col];
      if (isCut) p.song[startRow + row][startCol + col] = EMPTY_VALUE_16;
    }
  }
}

int Clipboard::pasteSong(chipnomad::Project &p, int startCol, int startRow) {
  // Push down existing data in the paste area
  for (int col = startCol; col < startCol + songCols && col < PROJECT_MAX_TRACKS; col++) {
    for (int row = PROJECT_MAX_LENGTH - 1; row >= startRow + songRows; row--) {
      p.song[row][col] = p.song[row - songRows][col];
    }
  }

  // Paste the buffer data
  int rowsPasted = 0;
  for (int row = 0; row < songRows && startRow + row < PROJECT_MAX_LENGTH; row++) {
    rowsPasted++;
    for (int col = 0; col < songCols && startCol + col < PROJECT_MAX_TRACKS; col++) {
      p.song[startRow + row][startCol + col] = song[row][col];
    }
  }
  return rowsPasted;
}

void Clipboard::shiftSongColumnUp(chipnomad::Project &p, int col, int startRow) {
  for (int row = startRow; row < PROJECT_MAX_LENGTH - 1; row++) {
    p.song[row][col] = p.song[row + 1][col];
  }
  p.song[PROJECT_MAX_LENGTH - 1][col] = EMPTY_VALUE_16;
}

void Clipboard::copyChain(chipnomad::Project &p, int chainIdx, int startCol, int startRow, int endCol, int endRow, int isCut) {
  chainRows = endRow - startRow + 1;
  chainStartCol = startCol;
  chainEndCol = endCol;

  for (int row = 0; row < chainRows; row++) {
    chain.rows[row] = p.chains[chainIdx].rows[startRow + row];

    if (isCut) {
      if (startCol == 0) p.chains[chainIdx].rows[startRow + row].phrase = EMPTY_VALUE_16;
      if (endCol == 1) p.chains[chainIdx].rows[startRow + row].transpose = 0;
    }
  }
}

int Clipboard::pasteChain(chipnomad::Project &p, int chainIdx, int startCol, int startRow) {
  // Only paste if column types match
  if (startCol != chainStartCol) return 0;

  int rowsPasted = 0;
  for (int row = 0; row < chainRows && startRow + row < 16; row++) {
    rowsPasted++;
    if (chainStartCol == 0) {
      p.chains[chainIdx].rows[startRow + row].phrase = chain.rows[row].phrase;
      // If we copied both columns (startCol=0, endCol=1), also paste transpose
      if (chainEndCol == 1) {
        p.chains[chainIdx].rows[startRow + row].transpose = chain.rows[row].transpose;
      }
    } else {
      p.chains[chainIdx].rows[startRow + row].transpose = chain.rows[row].transpose;
    }
  }
  return rowsPasted;
}

void Clipboard::copyPhrase(chipnomad::Project &p, int phraseIdx, int startCol, int startRow, int endCol, int endRow, int isCut) {
  phraseRows = endRow - startRow + 1;
  phraseStartCol = startCol;
  phraseEndCol = endCol;

  for (int row = 0; row < phraseRows; row++) {
    phrase.rows[row] = p.phrases[phraseIdx].rows[startRow + row];

    if (isCut) {
      if (startCol <= 0 && endCol >= 0) p.phrases[phraseIdx].rows[startRow + row].note = EMPTY_VALUE_8;
      if (startCol <= 1 && endCol >= 1) p.phrases[phraseIdx].rows[startRow + row].instrument = EMPTY_VALUE_8;
      if (startCol <= 2 && endCol >= 2) p.phrases[phraseIdx].rows[startRow + row].volume = EMPTY_VALUE_8;
      if (startCol <= 3 && endCol >= 4) {
        p.phrases[phraseIdx].rows[startRow + row].fx[0][0] = EMPTY_VALUE_8;
        p.phrases[phraseIdx].rows[startRow + row].fx[0][1] = 0;
      }
      if (startCol <= 5 && endCol >= 6) {
        p.phrases[phraseIdx].rows[startRow + row].fx[1][0] = EMPTY_VALUE_8;
        p.phrases[phraseIdx].rows[startRow + row].fx[1][1] = 0;
      }
      if (startCol <= 7 && endCol >= 8) {
        p.phrases[phraseIdx].rows[startRow + row].fx[2][0] = EMPTY_VALUE_8;
        p.phrases[phraseIdx].rows[startRow + row].fx[2][1] = 0;
      }
    }
  }
}

int Clipboard::pastePhrase(chipnomad::Project &p, int phraseIdx, int startCol, int startRow) {
  // Check if we can paste - same column or FX to FX
  int canPaste = (startCol == phraseStartCol) ||
  (startCol >= 3 && startCol <= 8 && phraseStartCol >= 3 && phraseStartCol <= 8);

  if (!canPaste) return 0;

  int rowsPasted = 0;
  for (int row = 0; row < phraseRows && startRow + row < 16; row++) {
    rowsPasted++;
    // Handle FX-only paste (can paste to different FX columns)
    if (phraseStartCol >= 3 && phraseStartCol <= 8) {
      int colOffset = startCol - phraseStartCol;
      for (int col = phraseStartCol; col <= phraseEndCol; col++) {
        int dstCol = col + colOffset;
        if (dstCol >= 3 && dstCol <= 8) {
          // Map UI columns to FX array indices
          int srcFx = (col - 3) / 2;
          int dstFx = (dstCol - 3) / 2;
          int srcPart = (col - 3) % 2;  // 0=name, 1=value
          int dstPart = (dstCol - 3) % 2;
          if (srcPart == dstPart) {  // Only paste name to name, value to value
            if (srcPart == 0) {
              p.phrases[phraseIdx].rows[startRow + row].fx[dstFx][0] = phrase.rows[row].fx[srcFx][0];
            } else {
              p.phrases[phraseIdx].rows[startRow + row].fx[dstFx][1] = phrase.rows[row].fx[srcFx][1];
            }
          }
        }
      }
    } else {
      // Handle regular multi-column paste
      for (int col = phraseStartCol; col <= phraseEndCol; col++) {
        if (col == 0) {
          p.phrases[phraseIdx].rows[startRow + row].note = phrase.rows[row].note;
        } else if (col == 1) {
          p.phrases[phraseIdx].rows[startRow + row].instrument = phrase.rows[row].instrument;
        } else if (col == 2) {
          p.phrases[phraseIdx].rows[startRow + row].volume = phrase.rows[row].volume;
        } else if (col >= 3 && col <= 8) {
          int fxIdx = (col - 3) / 2;
          int part = (col - 3) % 2;
          if (part == 0) {
            p.phrases[phraseIdx].rows[startRow + row].fx[fxIdx][0] = phrase.rows[row].fx[fxIdx][0];
          } else {
            p.phrases[phraseIdx].rows[startRow + row].fx[fxIdx][1] = phrase.rows[row].fx[fxIdx][1];
          }
        }
      }
    }
  }
  return rowsPasted;
}

void Clipboard::copyTable(chipnomad::Project &p, int tableIdx, int startCol, int startRow, int endCol, int endRow, int isCut) {
  tableRows = endRow - startRow + 1;
  tableStartCol = startCol;
  tableEndCol = endCol;

  for (int row = 0; row < tableRows; row++) {
    table.rows[row] = p.tables[tableIdx].rows[startRow + row];

    if (isCut) {
      if (startCol <= 0 && endCol >= 0) p.tables[tableIdx].rows[startRow + row].pitchFlag = 0;
      if (startCol <= 1 && endCol >= 1) p.tables[tableIdx].rows[startRow + row].pitchOffset = 0;
      if (startCol <= 2 && endCol >= 2) p.tables[tableIdx].rows[startRow + row].volume = EMPTY_VALUE_8;
      if (startCol <= 3 && endCol >= 4) {
        p.tables[tableIdx].rows[startRow + row].fx[0][0] = EMPTY_VALUE_8;
        p.tables[tableIdx].rows[startRow + row].fx[0][1] = 0;
      }
      if (startCol <= 5 && endCol >= 6) {
        p.tables[tableIdx].rows[startRow + row].fx[1][0] = EMPTY_VALUE_8;
        p.tables[tableIdx].rows[startRow + row].fx[1][1] = 0;
      }
      if (startCol <= 7 && endCol >= 8) {
        p.tables[tableIdx].rows[startRow + row].fx[2][0] = EMPTY_VALUE_8;
        p.tables[tableIdx].rows[startRow + row].fx[2][1] = 0;
      }
      if (startCol <= 9 && endCol >= 10) {
        p.tables[tableIdx].rows[startRow + row].fx[3][0] = EMPTY_VALUE_8;
        p.tables[tableIdx].rows[startRow + row].fx[3][1] = 0;
      }
    }
  }
}

int Clipboard::pasteTable(chipnomad::Project &p, int tableIdx, int startCol, int startRow) {
  // Check if we can paste - same column or FX to FX
  int canPaste = (startCol == tableStartCol) ||
  (startCol >= 3 && startCol <= 10 && tableStartCol >= 3 && tableStartCol <= 10);

  if (!canPaste) return 0;

  int rowsPasted = 0;
  for (int row = 0; row < tableRows && startRow + row < 16; row++) {
    rowsPasted++;
    // Handle FX-only paste (can paste to different FX columns)
    if (tableStartCol >= 3 && tableStartCol <= 10) {
      int colOffset = startCol - tableStartCol;
      for (int col = tableStartCol; col <= tableEndCol; col++) {
        int dstCol = col + colOffset;
        if (dstCol >= 3 && dstCol <= 10) {
          // Map UI columns to FX array indices
          int srcFx = (col - 3) / 2;
          int dstFx = (dstCol - 3) / 2;
          int srcPart = (col - 3) % 2;  // 0=name, 1=value
          int dstPart = (dstCol - 3) % 2;
          if (srcPart == dstPart) {  // Only paste name to name, value to value
            if (srcPart == 0) {
              p.tables[tableIdx].rows[startRow + row].fx[dstFx][0] = table.rows[row].fx[srcFx][0];
            } else {
              p.tables[tableIdx].rows[startRow + row].fx[dstFx][1] = table.rows[row].fx[srcFx][1];
            }
          }
        }
      }
    } else {
      // Handle regular multi-column paste
      for (int col = tableStartCol; col <= tableEndCol; col++) {
        if (col == 0) {
          p.tables[tableIdx].rows[startRow + row].pitchFlag = table.rows[row].pitchFlag;
        } else if (col == 1) {
          p.tables[tableIdx].rows[startRow + row].pitchOffset = table.rows[row].pitchOffset;
        } else if (col == 2) {
          p.tables[tableIdx].rows[startRow + row].volume = table.rows[row].volume;
        } else if (col >= 3 && col <= 10) {
          int fxIdx = (col - 3) / 2;
          int part = (col - 3) % 2;
          if (part == 0) {
            p.tables[tableIdx].rows[startRow + row].fx[fxIdx][0] = table.rows[row].fx[fxIdx][0];
          } else {
            p.tables[tableIdx].rows[startRow + row].fx[fxIdx][1] = table.rows[row].fx[fxIdx][1];
          }
        }
      }
    }
  }
  return rowsPasted;
}


// Clone chain (shallow)
int Clipboard::cloneChain(chipnomad::Project &p, int srcIdx, int dstIdx) {
  if (srcIdx >= PROJECT_MAX_CHAINS || dstIdx >= PROJECT_MAX_CHAINS) return 0;
  p.chains[dstIdx] = p.chains[srcIdx];
  return 1;
}

// Clone phrase
int Clipboard::clonePhrase(chipnomad::Project &p, int srcIdx, int dstIdx) {
  if (srcIdx >= PROJECT_MAX_PHRASES || dstIdx >= PROJECT_MAX_PHRASES) return 0;
  p.phrases[dstIdx] = p.phrases[srcIdx];
  return 1;
}

// Clone instrument
int Clipboard::cloneInstrument(chipnomad::Project &p, int srcIdx, int dstIdx) {
  using namespace chipnomad;
  if (srcIdx >= PROJECT_MAX_INSTRUMENTS || dstIdx >= PROJECT_MAX_INSTRUMENTS) return 0;
  p.instruments[dstIdx] = p.instruments[srcIdx];
  p.tables[dstIdx] = p.tables[srcIdx];

  // Deep copy sample data for AYSample instruments
  Instrument* src = &p.instruments[srcIdx];
  if (src->type == InstrumentType::AYSample && src->chip.aySample.sampleData != NULL) {
    uint16_t len = src->chip.aySample.fileLength;
    uint8_t* copy = (uint8_t*)malloc(len);
    if (copy) {
      memcpy(copy, src->chip.aySample.sampleData, len);
    }
    p.instruments[dstIdx].chip.aySample.sampleData = copy;
  }

  return 1;
}

// Deep clone phrases within an existing chain
int Clipboard::deepCloneChain(chipnomad::Project &p, int chainIdx) {
  if (chainIdx >= PROJECT_MAX_CHAINS) return 0;

  // Find all distinct phrases used in the chain
  uint16_t usedPhrases[16];
  uint16_t phraseMapping[16];
  int distinctCount = 0;

  for (int row = 0; row < 16; row++) {
    uint16_t phraseIdx = p.chains[chainIdx].rows[row].phrase;
    if (phraseIdx == EMPTY_VALUE_16) {
      phraseMapping[row] = EMPTY_VALUE_16;
      continue;
    }

    // Check if phrase already processed
    int found = -1;
    for (int i = 0; i < distinctCount; i++) {
      if (usedPhrases[i] == phraseIdx) {
        found = i;
        break;
      }
    }

    if (found == -1) {
      // New phrase, find empty slot
      int newPhraseIdx = findEmptyPhrase(&p, 0);
      if (newPhraseIdx == EMPTY_VALUE_16) return 0;

      // Clone the phrase
      clonePhrase(p, phraseIdx, newPhraseIdx);

      usedPhrases[distinctCount] = phraseIdx;
      phraseMapping[row] = newPhraseIdx;
      distinctCount++;
    } else {
      // Reuse existing mapping
      for (int i = 0; i < row; i++) {
        if (p.chains[chainIdx].rows[i].phrase == phraseIdx) {
          phraseMapping[row] = phraseMapping[i];
          break;
        }
      }
    }
  }

  // Update chain with new phrase references
  for (int row = 0; row < 16; row++) {
    p.chains[chainIdx].rows[row].phrase = phraseMapping[row];
  }

  return 1;
}

// Consolidated cloning functions
int Clipboard::cloneChainToNext(chipnomad::Project &p, int srcIdx) {
  int nextEmpty = findEmptyChain(&p, srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_16) return EMPTY_VALUE_16;
  cloneChain(p, srcIdx, nextEmpty);
  return nextEmpty;
}

int Clipboard::clonePhraseToNext(chipnomad::Project &p, int srcIdx) {
  int nextEmpty = findEmptyPhrase(&p, srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_16) return EMPTY_VALUE_16;
  clonePhrase(p, srcIdx, nextEmpty);
  return nextEmpty;
}

int Clipboard::cloneInstrumentToNext(chipnomad::Project &p, int srcIdx) {
  int nextEmpty = findEmptyInstrument(&p, srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_8) return EMPTY_VALUE_8;
  cloneInstrument(p, srcIdx, nextEmpty);
  return nextEmpty;
}

int Clipboard::deepCloneChainToNext(chipnomad::Project &p, int srcIdx) {
  int nextEmpty = findEmptyChain(&p, srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_16) return EMPTY_VALUE_16;
  cloneChain(p, srcIdx, nextEmpty);
  if (!deepCloneChain(p, nextEmpty)) return EMPTY_VALUE_16;
  return nextEmpty;
}

int Clipboard::cloneInstrumentsInPhrase(chipnomad::Project &p, int phraseIdx, int startRow, int endRow) {
  uint8_t usedInstruments[16];
  uint8_t instrumentMapping[16];
  int distinctCount = 0;

  // Find all distinct instruments in selection
  for (int r = startRow; r <= endRow; r++) {
    uint8_t instIdx = p.phrases[phraseIdx].rows[r].instrument;
    if (instIdx == EMPTY_VALUE_8) {
      instrumentMapping[r - startRow] = EMPTY_VALUE_8;
      continue;
    }

    // Check if instrument already processed
    int found = -1;
    for (int i = 0; i < distinctCount; i++) {
      if (usedInstruments[i] == instIdx) {
        found = i;
        break;
      }
    }

    if (found == -1) {
      // New instrument, clone it
      int newInstIdx = cloneInstrumentToNext(p, instIdx);
      if (newInstIdx == EMPTY_VALUE_8) return 0;

      usedInstruments[distinctCount] = instIdx;
      instrumentMapping[r - startRow] = newInstIdx;
      distinctCount++;
    } else {
      // Reuse existing mapping
      for (int i = 0; i < r - startRow; i++) {
        if (p.phrases[phraseIdx].rows[startRow + i].instrument == instIdx) {
          instrumentMapping[r - startRow] = instrumentMapping[i];
          break;
        }
      }
    }
  }

  // Update phrase with new instrument references
  for (int r = startRow; r <= endRow; r++) {
    p.phrases[phraseIdx].rows[r].instrument = instrumentMapping[r - startRow];
  }

  return distinctCount;
}

void Clipboard::copyInstrument(chipnomad::Project &p, int instrumentIdx) {
  instrument = p.instruments[instrumentIdx];
  instrumentTable = p.tables[instrumentIdx];
  instrumentValid = 1;
}

void Clipboard::pasteInstrument(chipnomad::Project &p, int instrumentIdx) {
  using namespace chipnomad;
  if (!instrumentValid) return;
  p.instruments[instrumentIdx] = instrument;
  p.tables[instrumentIdx] = instrumentTable;

  // Deep copy sample data for AYSample instruments
  if (instrument.type == InstrumentType::AYSample && instrument.chip.aySample.sampleData != NULL) {
    uint16_t len = instrument.chip.aySample.fileLength;
    uint8_t* copy = (uint8_t*)malloc(len);
    if (copy) {
      memcpy(copy, instrument.chip.aySample.sampleData, len);
    }
    p.instruments[instrumentIdx].chip.aySample.sampleData = copy;
  }
}

void Clipboard::copyAYWavetable(chipnomad::Project &p, int wavetableIdx) {
  memcpy(ayWavetable, p.ayWavetables[wavetableIdx], 32);
  wavetableValid = 1;
}

void Clipboard::pasteAYWavetable(chipnomad::Project &p, int wavetableIdx) {
  if (!wavetableValid) return;
  memcpy(p.ayWavetables[wavetableIdx], ayWavetable, 32);
}
