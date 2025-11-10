#include <copy_paste.h>
#include <screens.h>
#include <string.h>

uint16_t cpBufSong[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
struct Chain cpBufChain;
struct Phrase cpBufPhrase;
struct Table cpBufTable;
struct Groove cpBufGroove;

static int cpBufGrooveLength;
static int cpBufSongRows;
static int cpBufSongCols;
static int cpBufChainRows;
static int cpBufChainStartCol;
static int cpBufChainEndCol;
static int cpBufPhraseRows;
static int cpBufPhraseStartCol;
static int cpBufPhraseEndCol;
static int cpBufTableRows;
static int cpBufTableStartCol;
static int cpBufTableEndCol;

void copyGroove(int grooveIdx, int startRow, int endRow, int isCut) {
  cpBufGrooveLength = endRow - startRow + 1;
  for (int row = startRow; row <= endRow; row++) {
    cpBufGroove.speed[row - startRow] = project.grooves[grooveIdx].speed[row];
    if (isCut) {
      project.grooves[grooveIdx].speed[row] = 0;
    }
  }
}

void pasteGroove(int grooveIdx, int startRow) {
  for (int row = 0; row < cpBufGrooveLength && startRow + row < 16; row++) {
    project.grooves[grooveIdx].speed[startRow + row] = cpBufGroove.speed[row];
  }
}

void copySong(int startCol, int startRow, int endCol, int endRow, int isCut) {
  cpBufSongRows = endRow - startRow + 1;
  cpBufSongCols = endCol - startCol + 1;

  for (int row = 0; row < cpBufSongRows; row++) {
    for (int col = 0; col < cpBufSongCols; col++) {
      cpBufSong[row][col] = project.song[startRow + row][startCol + col];
      if (isCut) {
        project.song[startRow + row][startCol + col] = EMPTY_VALUE_16;
      }
    }
  }
}

void pasteSong(int startCol, int startRow) {
  // Push down existing data in the paste area
  for (int col = startCol; col < startCol + cpBufSongCols && col < PROJECT_MAX_TRACKS; col++) {
    for (int row = PROJECT_MAX_LENGTH - 1; row >= startRow + cpBufSongRows; row--) {
      project.song[row][col] = project.song[row - cpBufSongRows][col];
    }
  }

  // Paste the buffer data
  for (int row = 0; row < cpBufSongRows && startRow + row < PROJECT_MAX_LENGTH; row++) {
    for (int col = 0; col < cpBufSongCols && startCol + col < PROJECT_MAX_TRACKS; col++) {
      project.song[startRow + row][startCol + col] = cpBufSong[row][col];
    }
  }
}

void copyChain(int chainIdx, int startCol, int startRow, int endCol, int endRow, int isCut) {
  cpBufChainRows = endRow - startRow + 1;
  cpBufChainStartCol = startCol;
  cpBufChainEndCol = endCol;

  for (int row = 0; row < cpBufChainRows; row++) {
    cpBufChain.rows[row] = project.chains[chainIdx].rows[startRow + row];

    if (isCut) {
      if (startCol == 0) project.chains[chainIdx].rows[startRow + row].phrase = EMPTY_VALUE_16;
      if (endCol == 1) project.chains[chainIdx].rows[startRow + row].transpose = 0;
    }
  }
}

void pasteChain(int chainIdx, int startCol, int startRow) {
  // Only paste if column types match
  if (startCol != cpBufChainStartCol) return;

  for (int row = 0; row < cpBufChainRows && startRow + row < 16; row++) {
    if (cpBufChainStartCol == 0) {
      project.chains[chainIdx].rows[startRow + row].phrase = cpBufChain.rows[row].phrase;
      // If we copied both columns (startCol=0, endCol=1), also paste transpose
      if (cpBufChainEndCol == 1) {
        project.chains[chainIdx].rows[startRow + row].transpose = cpBufChain.rows[row].transpose;
      }
    } else {
      project.chains[chainIdx].rows[startRow + row].transpose = cpBufChain.rows[row].transpose;
    }
  }
}

void copyPhrase(int phraseIdx, int startCol, int startRow, int endCol, int endRow, int isCut) {
  cpBufPhraseRows = endRow - startRow + 1;
  cpBufPhraseStartCol = startCol;
  cpBufPhraseEndCol = endCol;

  for (int row = 0; row < cpBufPhraseRows; row++) {
    cpBufPhrase.rows[row] = project.phrases[phraseIdx].rows[startRow + row];

    if (isCut) {
      if (startCol <= 0 && endCol >= 0) project.phrases[phraseIdx].rows[startRow + row].note = EMPTY_VALUE_8;
      if (startCol <= 1 && endCol >= 1) project.phrases[phraseIdx].rows[startRow + row].instrument = EMPTY_VALUE_8;
      if (startCol <= 2 && endCol >= 2) project.phrases[phraseIdx].rows[startRow + row].volume = EMPTY_VALUE_8;
      if (startCol <= 3 && endCol >= 3) {
        project.phrases[phraseIdx].rows[startRow + row].fx[0][0] = EMPTY_VALUE_8;
        project.phrases[phraseIdx].rows[startRow + row].fx[0][1] = 0;
      }
      if (startCol <= 4 && endCol >= 4) {
        project.phrases[phraseIdx].rows[startRow + row].fx[1][0] = EMPTY_VALUE_8;
        project.phrases[phraseIdx].rows[startRow + row].fx[1][1] = 0;
      }
      if (startCol <= 5 && endCol >= 5) {
        project.phrases[phraseIdx].rows[startRow + row].fx[2][0] = EMPTY_VALUE_8;
        project.phrases[phraseIdx].rows[startRow + row].fx[2][1] = 0;
      }
    }
  }
}

void pastePhrase(int phraseIdx, int startCol, int startRow) {
  // Check if we can paste - same column or FX to FX
  int canPaste = (startCol == cpBufPhraseStartCol) ||
                 (startCol >= 3 && startCol <= 8 && cpBufPhraseStartCol >= 3 && cpBufPhraseStartCol <= 8);

  if (!canPaste) return;

  for (int row = 0; row < cpBufPhraseRows && startRow + row < 16; row++) {
    // Handle FX-only paste (can paste to different FX columns)
    if (cpBufPhraseStartCol >= 3 && cpBufPhraseStartCol <= 8) {
      int colOffset = startCol - cpBufPhraseStartCol;
      for (int col = cpBufPhraseStartCol; col <= cpBufPhraseEndCol; col++) {
        int dstCol = col + colOffset;
        if (dstCol >= 3 && dstCol <= 8) {
          // Map UI columns to FX array indices
          int srcFx = (col - 3) / 2;
          int dstFx = (dstCol - 3) / 2;
          int srcPart = (col - 3) % 2;  // 0=name, 1=value
          int dstPart = (dstCol - 3) % 2;
          if (srcPart == dstPart) {  // Only paste name to name, value to value
            if (srcPart == 0) {
              project.phrases[phraseIdx].rows[startRow + row].fx[dstFx][0] = cpBufPhrase.rows[row].fx[srcFx][0];
            } else {
              project.phrases[phraseIdx].rows[startRow + row].fx[dstFx][1] = cpBufPhrase.rows[row].fx[srcFx][1];
            }
          }
        }
      }
    } else {
      // Handle regular multi-column paste
      for (int col = cpBufPhraseStartCol; col <= cpBufPhraseEndCol; col++) {
        if (col == 0) {
          project.phrases[phraseIdx].rows[startRow + row].note = cpBufPhrase.rows[row].note;
        } else if (col == 1) {
          project.phrases[phraseIdx].rows[startRow + row].instrument = cpBufPhrase.rows[row].instrument;
        } else if (col == 2) {
          project.phrases[phraseIdx].rows[startRow + row].volume = cpBufPhrase.rows[row].volume;
        } else if (col >= 3 && col <= 8) {
          int fxIdx = (col - 3) / 2;
          int part = (col - 3) % 2;
          if (part == 0) {
            project.phrases[phraseIdx].rows[startRow + row].fx[fxIdx][0] = cpBufPhrase.rows[row].fx[fxIdx][0];
          } else {
            project.phrases[phraseIdx].rows[startRow + row].fx[fxIdx][1] = cpBufPhrase.rows[row].fx[fxIdx][1];
          }
        }
      }
    }
  }
}

void copyTable(int tableIdx, int startCol, int startRow, int endCol, int endRow, int isCut) {
  cpBufTableRows = endRow - startRow + 1;
  cpBufTableStartCol = startCol;
  cpBufTableEndCol = endCol;

  for (int row = 0; row < cpBufTableRows; row++) {
    cpBufTable.rows[row] = project.tables[tableIdx].rows[startRow + row];

    if (isCut) {
      if (startCol <= 0 && endCol >= 0) project.tables[tableIdx].rows[startRow + row].pitchFlag = 0;
      if (startCol <= 1 && endCol >= 1) project.tables[tableIdx].rows[startRow + row].pitchOffset = 0;
      if (startCol <= 2 && endCol >= 2) project.tables[tableIdx].rows[startRow + row].volume = EMPTY_VALUE_8;
      if (startCol <= 3 && endCol >= 3) {
        project.tables[tableIdx].rows[startRow + row].fx[0][0] = EMPTY_VALUE_8;
        project.tables[tableIdx].rows[startRow + row].fx[0][1] = 0;
      }
      if (startCol <= 4 && endCol >= 4) {
        project.tables[tableIdx].rows[startRow + row].fx[1][0] = EMPTY_VALUE_8;
        project.tables[tableIdx].rows[startRow + row].fx[1][1] = 0;
      }
      if (startCol <= 5 && endCol >= 5) {
        project.tables[tableIdx].rows[startRow + row].fx[2][0] = EMPTY_VALUE_8;
        project.tables[tableIdx].rows[startRow + row].fx[2][1] = 0;
      }
      if (startCol <= 6 && endCol >= 6) {
        project.tables[tableIdx].rows[startRow + row].fx[3][0] = EMPTY_VALUE_8;
        project.tables[tableIdx].rows[startRow + row].fx[3][1] = 0;
      }
    }
  }
}

void pasteTable(int tableIdx, int startCol, int startRow) {
  // Check if we can paste - same column or FX to FX
  int canPaste = (startCol == cpBufTableStartCol) ||
                 (startCol >= 3 && startCol <= 10 && cpBufTableStartCol >= 3 && cpBufTableStartCol <= 10);

  if (!canPaste) return;

  for (int row = 0; row < cpBufTableRows && startRow + row < 16; row++) {
    // Handle FX-only paste (can paste to different FX columns)
    if (cpBufTableStartCol >= 3 && cpBufTableStartCol <= 10) {
      int colOffset = startCol - cpBufTableStartCol;
      for (int col = cpBufTableStartCol; col <= cpBufTableEndCol; col++) {
        int dstCol = col + colOffset;
        if (dstCol >= 3 && dstCol <= 10) {
          // Map UI columns to FX array indices
          int srcFx = (col - 3) / 2;
          int dstFx = (dstCol - 3) / 2;
          int srcPart = (col - 3) % 2;  // 0=name, 1=value
          int dstPart = (dstCol - 3) % 2;
          if (srcPart == dstPart) {  // Only paste name to name, value to value
            if (srcPart == 0) {
              project.tables[tableIdx].rows[startRow + row].fx[dstFx][0] = cpBufTable.rows[row].fx[srcFx][0];
            } else {
              project.tables[tableIdx].rows[startRow + row].fx[dstFx][1] = cpBufTable.rows[row].fx[srcFx][1];
            }
          }
        }
      }
    } else {
      // Handle regular multi-column paste
      for (int col = cpBufTableStartCol; col <= cpBufTableEndCol; col++) {
        if (col == 0) {
          project.tables[tableIdx].rows[startRow + row].pitchFlag = cpBufTable.rows[row].pitchFlag;
        } else if (col == 1) {
          project.tables[tableIdx].rows[startRow + row].pitchOffset = cpBufTable.rows[row].pitchOffset;
        } else if (col == 2) {
          project.tables[tableIdx].rows[startRow + row].volume = cpBufTable.rows[row].volume;
        } else if (col >= 3 && col <= 10) {
          int fxIdx = (col - 3) / 2;
          int part = (col - 3) % 2;
          if (part == 0) {
            project.tables[tableIdx].rows[startRow + row].fx[fxIdx][0] = cpBufTable.rows[row].fx[fxIdx][0];
          } else {
            project.tables[tableIdx].rows[startRow + row].fx[fxIdx][1] = cpBufTable.rows[row].fx[fxIdx][1];
          }
        }
      }
    }
  }
}

// Selection mode switching helpers
static int isSingleCell(int startCol, int startRow, int endCol, int endRow) {
  return startCol == endCol && startRow == endRow;
}

static int isFullColumn(int startCol, int startRow, int endCol, int endRow) {
  return startCol == endCol && startRow == 0 && endRow == 15;
}

static int isFullRow(int startRow, int endRow, int startCol, int endCol, int maxCol) {
  return startRow == endRow && startCol == 0 && endCol == maxCol;
}

static void selectColumn(struct ScreenData* screen) {
  screen->selectStartRow = 0;
  screen->selectStartCol = screen->selectAnchorCol;
  screen->cursorRow = 15;
  screen->cursorCol = screen->selectAnchorCol;
}

static void selectRow(struct ScreenData* screen, int maxCol) {
  screen->selectStartRow = screen->selectAnchorRow;
  screen->selectStartCol = 0;
  screen->cursorRow = screen->selectAnchorRow;
  screen->cursorCol = maxCol;
}

static void selectAll(struct ScreenData* screen, int maxCol) {
  screen->selectStartRow = 0;
  screen->selectStartCol = 0;
  screen->cursorRow = 15;
  screen->cursorCol = maxCol;
}

// Selection mode switching
int switchPhraseSelectionMode(struct ScreenData* screen) {
  int startCol, startRow, endCol, endRow;
  getSelectionBounds(screen, &startCol, &startRow, &endCol, &endRow);
  
  if (isSingleCell(startCol, startRow, endCol, endRow)) {
    selectColumn(screen);
    return 0;
  }
  if (isFullColumn(startCol, startRow, endCol, endRow)) {
    selectRow(screen, 8);
    return 0;
  }
  if (isFullRow(startRow, endRow, startCol, endCol, 8)) {
    selectAll(screen, 8);
    return 0;
  }
  return 1;
}

int switchTableSelectionMode(struct ScreenData* screen) {
  int startCol, startRow, endCol, endRow;
  getSelectionBounds(screen, &startCol, &startRow, &endCol, &endRow);
  
  if (isSingleCell(startCol, startRow, endCol, endRow)) {
    selectColumn(screen);
    return 0;
  }
  if (isFullColumn(startCol, startRow, endCol, endRow)) {
    selectRow(screen, 10);
    return 0;
  }
  if (isFullRow(startRow, endRow, startCol, endCol, 10)) {
    selectAll(screen, 10);
    return 0;
  }
  return 1;
}

int switchChainSelectionMode(struct ScreenData* screen) {
  int startCol, startRow, endCol, endRow;
  getSelectionBounds(screen, &startCol, &startRow, &endCol, &endRow);
  
  if (isSingleCell(startCol, startRow, endCol, endRow)) {
    selectColumn(screen);
    return 0;
  }
  if (isFullColumn(startCol, startRow, endCol, endRow)) {
    selectRow(screen, 1);
    return 0;
  }
  if (isFullRow(startRow, endRow, startCol, endCol, 1)) {
    selectAll(screen, 1);
    return 0;
  }
  return 1;
}

int switchGrooveSelectionMode(struct ScreenData* screen) {
  int startCol, startRow, endCol, endRow;
  getSelectionBounds(screen, &startCol, &startRow, &endCol, &endRow);
  
  if (isSingleCell(startCol, startRow, endCol, endRow)) {
    selectAll(screen, 0);
    return 0;
  }
  return 1;
}

int switchSongSelectionMode(struct ScreenData* screen) {
  int startCol, startRow, endCol, endRow;
  getSelectionBounds(screen, &startCol, &startRow, &endCol, &endRow);
  
  if (isSingleCell(startCol, startRow, endCol, endRow)) {
    selectRow(screen, screen->getColumnCount(screen->selectAnchorRow) - 1);
    return 0;
  }
  return 1;
}

// Find empty chain slot
int findEmptyChain(int start) {
  for (int i = start; i < PROJECT_MAX_CHAINS; i++) {
    if (chainIsEmpty(i)) return i;
  }
  return EMPTY_VALUE_16;
}

// Find empty phrase slot
int findEmptyPhrase(int start) {
  for (int i = start; i < PROJECT_MAX_PHRASES; i++) {
    if (phraseIsEmpty(i)) return i;
  }
  return EMPTY_VALUE_16;
}

// Clone chain (shallow)
int cloneChain(int srcIdx, int dstIdx) {
  if (srcIdx >= PROJECT_MAX_CHAINS || dstIdx >= PROJECT_MAX_CHAINS) return 0;
  project.chains[dstIdx] = project.chains[srcIdx];
  return 1;
}

// Clone phrase
int clonePhrase(int srcIdx, int dstIdx) {
  if (srcIdx >= PROJECT_MAX_PHRASES || dstIdx >= PROJECT_MAX_PHRASES) return 0;
  project.phrases[dstIdx] = project.phrases[srcIdx];
  return 1;
}

// Deep clone phrases within an existing chain
int deepCloneChain(int chainIdx) {
  if (chainIdx >= PROJECT_MAX_CHAINS) return 0;
  
  // Find all distinct phrases used in the chain
  uint16_t usedPhrases[16];
  uint16_t phraseMapping[16];
  int distinctCount = 0;
  
  for (int row = 0; row < 16; row++) {
    uint16_t phraseIdx = project.chains[chainIdx].rows[row].phrase;
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
      int newPhraseIdx = findEmptyPhrase(0);
      if (newPhraseIdx == EMPTY_VALUE_16) return 0;
      
      // Clone the phrase
      clonePhrase(phraseIdx, newPhraseIdx);
      
      usedPhrases[distinctCount] = phraseIdx;
      phraseMapping[row] = newPhraseIdx;
      distinctCount++;
    } else {
      // Reuse existing mapping
      for (int i = 0; i < row; i++) {
        if (project.chains[chainIdx].rows[i].phrase == phraseIdx) {
          phraseMapping[row] = phraseMapping[i];
          break;
        }
      }
    }
  }
  
  // Update chain with new phrase references
  for (int row = 0; row < 16; row++) {
    project.chains[chainIdx].rows[row].phrase = phraseMapping[row];
  }
  
  return 1;
}

// Consolidated cloning functions
int cloneChainToNext(int srcIdx) {
  int nextEmpty = findEmptyChain(srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_16) return EMPTY_VALUE_16;
  cloneChain(srcIdx, nextEmpty);
  return nextEmpty;
}

int clonePhraseToNext(int srcIdx) {
  int nextEmpty = findEmptyPhrase(srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_16) return EMPTY_VALUE_16;
  clonePhrase(srcIdx, nextEmpty);
  return nextEmpty;
}

int deepCloneChainToNext(int srcIdx) {
  int nextEmpty = findEmptyChain(srcIdx + 1);
  if (nextEmpty == EMPTY_VALUE_16) return EMPTY_VALUE_16;
  cloneChain(srcIdx, nextEmpty);
  if (!deepCloneChain(nextEmpty)) return EMPTY_VALUE_16;
  return nextEmpty;
}

