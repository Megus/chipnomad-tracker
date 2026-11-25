#ifndef __SCREEN_CHAIN_H__
#define __SCREEN_CHAIN_H__

#include "screen.h"
#include "clipboard.h"
#include "audio_manager.h"

class ScreenChain : public Screen, EditCellInterface {
  public:
    ScreenChain(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx, AudioManager& audio, Clipboard& clipboard): Screen(state, dispatcher, gfx), audio(audio), clipboard(clipboard) {};

    void setup(const ScreenData& input) override;
    void resetState() override;
    bool supportsSelectMode() override { return true; };
    int getRowCount() override { return 16; };
    int getColumnCount(int row) override { return 2; };
    ScreenPlaybackLevel getPlaybackLevel() override { return ScreenPlaybackLevel::chain; };
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

  protected:
    int chain;

    AudioManager& audio;
    Clipboard& clipboard;

    uint16_t lastPhraseValue = 0;
    uint8_t lastTransposeValue = 0;
};

#endif // __SCREEN_CHAIN_H__