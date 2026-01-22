#ifndef COPY_PASTE_H
#define COPY_PASTE_H
#include "screens/screens.h"

// Groove copy/paste
void copyGroove(int grooveIdx, int startRow, int endRow, int isCut);
int pasteGroove(int grooveIdx, int startRow);

// Song copy/paste
void copySong(int startCol, int startRow, int endCol, int endRow, int isCut);
int pasteSong(int startCol, int startRow);
void shiftSongColumnUp(int col, int startRow);

// Chain copy/paste
void copyChain(int chainIdx, int startCol, int startRow, int endCol, int endRow, int isCut);
int pasteChain(int chainIdx, int startCol, int startRow);

// Phrase copy/paste
void copyPhrase(int phraseIdx, int startCol, int startRow, int endCol, int endRow, int isCut);
int pastePhrase(int phraseIdx, int startCol, int startRow);

// Table copy/paste
void copyTable(int tableIdx, int startCol, int startRow, int endCol, int endRow, int isCut);
int pasteTable(int tableIdx, int startCol, int startRow);

// Instrument copy/paste
void copyInstrument(int instrumentIdx);
void pasteInstrument(int instrumentIdx);
// Clone functionality
int cloneChain(int srcIdx, int dstIdx);
int clonePhrase(int srcIdx, int dstIdx);
int cloneInstrument(int srcIdx, int dstIdx);
int deepCloneChain(int chainIdx);
int cloneChainToNext(int srcIdx);
int clonePhraseToNext(int srcIdx);
int cloneInstrumentToNext(int srcIdx);
int deepCloneChainToNext(int srcIdx);
int cloneInstrumentsInPhrase(int phraseIdx, int startRow, int endRow);

// Selection mode switching
int switchPhraseSelectionMode(ScreenData* screen);
int switchTableSelectionMode(ScreenData* screen);
int switchChainSelectionMode(ScreenData* screen);
int switchGrooveSelectionMode(ScreenData* screen);
int switchSongSelectionMode(ScreenData* screen);

// Copy buffer management
void resetCopyBuffers(void);
#endif // COPY_PASTE_H
