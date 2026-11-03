#ifndef __SCREEN_PROJECT_H__
#define __SCREEN_PROJECT_H__

#include "screen.h"
#include "screen_project_ay.h"

class ScreenProject : public Screen {
  public:
    ScreenProject(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx);
    ~ScreenProject();

    void setup(const ScreenData& input) override;
    int getRowCount() override;
    int getColumnCount(int row) override;
    ScreenPlaybackLevel getPlaybackLevel() override { return ScreenPlaybackLevel::song; };

    void drawStatic() override;
    void drawCursor(int col, int row) override;
    void drawCell(int col, int row, CellState state) override;

    bool onEdit(int col, int row, EditCellAction action) override;
    bool onNavigationInput(const InputEvent& e) override;
    bool onEvent(const Event& e) override;

  protected:
    static constexpr int commonRowCount = 8; // Number of common rows on the Project screen (before partial)

    int tickRateI = 0; // Integer part of the tick rate
    uint16_t tickRateF = 0; // Fractional part of the tick rate
    char* editingString = nullptr; // The string being edited (TODO: might drop it and use row number instead)
    ScreenPartial* partial; // Current partial (chip-specific project settings)

    ScreenPartialProjectAY* partialAY;
};

#endif // __SCREEN_PROJECT_H__