#ifndef __SCREEN_PROJECT_H__
#define __SCREEN_PROJECT_H__

#include "screen.h"
#include "screen_project_ay.h"
#include "audio_manager.h"

class ScreenProject : public Screen {
  public:
    ScreenProject(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx, AudioManager& audio);
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

    friend class ScreenPartialProjectAY;

  protected:
    enum class ConfirmValue : int {
      cancel,
      newProject,
      loadProject,
    };

    AudioManager& audio;

    static constexpr int commonRowCount = 8; // Number of common rows on the Project screen (before partial)

    int tickRateI = 0; // Integer part of the tick rate
    uint16_t tickRateF = 0; // Fractional part of the tick rate
    ScreenPartial<ScreenProject>* partial; // Current partial (chip-specific project settings)

    ScreenPartialProjectAY* partialAY;

    void newProject();
    void loadProject();
};

#endif // __SCREEN_PROJECT_H__