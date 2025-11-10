#include <project.h>

// Forward declaration
struct ScreenData;

// Groove copy/paste
void copyGroove(int grooveIdx, int startRow, int endRow, int isCut);
void pasteGroove(int grooveIdx, int startRow);

// Song copy/paste
void copySong(int startCol, int startRow, int endCol, int endRow, int isCut);
void pasteSong(int startCol, int startRow);

// Chain copy/paste
void copyChain(int chainIdx, int startCol, int startRow, int endCol, int endRow, int isCut);
void pasteChain(int chainIdx, int startCol, int startRow);

// Phrase copy/paste
void copyPhrase(int phraseIdx, int startCol, int startRow, int endCol, int endRow, int isCut);
void pastePhrase(int phraseIdx, int startCol, int startRow);

// Table copy/paste
void copyTable(int tableIdx, int startCol, int startRow, int endCol, int endRow, int isCut);
void pasteTable(int tableIdx, int startCol, int startRow);
// Selection mode switching
int switchPhraseSelectionMode(struct ScreenData* screen);
int switchTableSelectionMode(struct ScreenData* screen);
int switchChainSelectionMode(struct ScreenData* screen);
int switchGrooveSelectionMode(struct ScreenData* screen);
int switchSongSelectionMode(struct ScreenData* screen);