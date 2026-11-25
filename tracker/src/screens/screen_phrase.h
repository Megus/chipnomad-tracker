#ifndef __SCREEN_PHRASE_H__
#define __SCREEN_PHRASE_H__

#include "screen.h"
#include "clipboard.h"
#include "audio_manager.h"

class ScreenPhrase : public Screen, EditCellInterface {
  public:
    ScreenPhrase(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx, AudioManager& audio, Clipboard& clipboard): Screen(state, dispatcher, gfx), audio(audio), clipboard(clipboard) {};

    void setup(const ScreenData& input) override;
    void resetState() override;
    bool supportsSelectMode() override { return true; };
    int getRowCount() override { return 16; };
    int getColumnCount(int row) override { return 9; };
    ScreenPlaybackLevel getPlaybackLevel() override { return ScreenPlaybackLevel::phrase; };
    Rect getSelectionRect(const Rect& range) override;
    bool switchSelectionRange() override;
    chipnomad::LoopRange getLoopRange() override;

    void draw() override;
    void drawStatic() override;
    void drawCursor(int col, int row) override;
    void drawRowHeader(int row, CellState state) override;
    void drawColHeader(int col, CellState state) override;
    void drawCell(int col, int row, CellState state) override;

    bool onEdit(int col, int row, EditCellAction action) override;
    bool editCell(int col, int row, EditCellAction action) override;
    bool onNavigationInput(const Event& e) override;
    bool onEvent(const Event& e);

  protected:
    static constexpr int columnX[] = {3, 7, 10, 13, 16, 19, 22, 25, 28, 31};

    int phrase;
    int phraseIdx = 0;
    chipnomad::PhraseRow *phraseRows = nullptr;

    uint8_t lastNote = 48;
    uint8_t lastInstrument = 0;

    uint8_t lastVolume = 15;
    uint8_t lastFX[2] = {0, 0};

    AudioManager& audio;
    Clipboard& clipboard;
};

#endif // __SCREEN_CHAIN_H__