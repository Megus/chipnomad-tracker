#ifndef __COPY_PASTE_H__
#define __COPY_PASTE_H__

#include "chipnomad_lib.h"
#include "gfx.h"

class Clipboard {
  public:
    Clipboard();
    ~Clipboard() = default;

    void resetClipboard();

    void copyGroove(chipnomad::Project &p, int grooveIdx, int startRow, int endRow, int isCut);
    int pasteGroove(chipnomad::Project &p, int grooveIdx, int startRow);

    void copySong(chipnomad::Project &p, Rect range, bool isCut);
    int pasteSong(chipnomad::Project &p, int startCol, int startRow);

    void copyChain(chipnomad::Project &p, int chainIdx, Rect range, bool isCut);
    int pasteChain(chipnomad::Project &p, int chainIdx, int startCol, int startRow);

    void copyPhrase(chipnomad::Project &p, int phraseIdx, Rect range, bool isCut);
    int pastePhrase(chipnomad::Project &p, int phraseIdx, int startCol, int startRow);

    void copyTable(chipnomad::Project &p, int tableIdx, Rect range, bool isCut);
    int pasteTable(chipnomad::Project &p, int tableIdx, int startCol, int startRow);

    void copyInstrument(chipnomad::Project &p, int instrumentIdx);
    void pasteInstrument(chipnomad::Project &p, int instrumentIdx);

    void copyAYWavetable(chipnomad::Project &p, int wavetableIdx);
    void pasteAYWavetable(chipnomad::Project &p, int wavetableIdx);

    int cloneChain(chipnomad::Project &p, int srcIdx, int dstIdx);
    int clonePhrase(chipnomad::Project &p, int srcIdx, int dstIdx);
    int cloneInstrument(chipnomad::Project &p, int srcIdx, int dstIdx);
    int deepCloneChain(chipnomad::Project &p, int chainIdx);
    int cloneChainToNext(chipnomad::Project &p, int srcIdx);
    int clonePhraseToNext(chipnomad::Project &p, int srcIdx);
    int cloneInstrumentToNext(chipnomad::Project &p, int srcIdx);
    int deepCloneChainToNext(chipnomad::Project &p, int srcIdx);
    int cloneInstrumentsInPhrase(chipnomad::Project &p, int phraseIdx, int startRow, int endRow);

  protected:
    uint16_t song[PROJECT_MAX_LENGTH][PROJECT_MAX_TRACKS];
    chipnomad::Chain chain;
    chipnomad::Phrase phrase;
    chipnomad::Table table;
    chipnomad::Groove groove;
    chipnomad::Instrument instrument;
    chipnomad::Table instrumentTable;
    uint8_t ayWavetable[32];

    int grooveLength;
    int songRows;
    int songCols;
    int chainRows;
    int chainStartCol;
    int chainEndCol;
    int phraseRows;
    int phraseStartCol;
    int phraseEndCol;
    int tableRows;
    int tableStartCol;
    int tableEndCol;
    bool instrumentValid;
    bool wavetableValid;
};

#endif // __COPY_PASTE_H__
