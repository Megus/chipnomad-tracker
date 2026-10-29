#ifndef __SCREEN_SONG_H__
#define __SCREEN_SONG_H__

#include "screen.h"
#include "clipboard.h"
#include "audio_manager.h"

class ScreenSong : public Screen, EditCellInterface {
  public:
    ScreenSong(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx, AudioManager& audio, Clipboard& clipboard):
      Screen(state, dispatcher, gfx), audio(audio), clipboard(clipboard) {};

    void setup(int input) override;
    bool supportsSelectMode() override { return true; };
    int getRowCount() override { return PROJECT_MAX_LENGTH; };
    int getColumnCount(int row) override { return state.project.tracksCount; };
    ScreenPlaybackLevel getPlaybackLevel() override { return ScreenPlaybackLevel::song; };
    Rect getSelectionRect(Rect range) override;
    chipnomad::LoopRange getLoopRange() override;

    void draw() override;
    void drawStatic() override;
    void drawCursor(int col, int row) override;
    void drawRowHeader(int row, CellState state) override;
    void drawColHeader(int col, CellState state) override;
    void drawCell(int col, int row, CellState state) override;
    bool isRowVisible(int row) override;
    bool adjustVerticalScroll(bool pageJump) override;

    bool onEdit(int col, int row, EditCellAction action) override;
    bool editCell(int col, int row, EditCellAction action) override;
    bool onNavigationInput(InputEventData e) override;
    bool onEvent(Event e) override;

  protected:
    enum class MuteSoloState : int {
      empty,
      optPressed,
      mute,
      solo,
      tempLeft,
      tempRight
    };

    AudioManager& audio;
    Clipboard& clipboard;

    int topRow = 0;
    uint16_t lastChainValue = 0;
    MuteSoloState muteSoloState = MuteSoloState::empty;

    bool switchSelectionMode();
};

#endif // __SCREEN_SONG_H__