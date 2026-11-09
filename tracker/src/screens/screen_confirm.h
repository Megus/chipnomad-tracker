#ifndef __SCREEN_CONFIRM_H__
#define __SCREEN_CONFIRM_H__

#include "screen.h"

class ScreenConfirm : public Screen {
  public:
    ScreenConfirm(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx): Screen(state, dispatcher, gfx) {};

    void setup(const ScreenData& input) override;
    int getRowCount() override { return 1; };
    int getColumnCount(int row) override { return 2; };

    void drawStatic() override;
    void drawCell(int col, int row, CellState state) override;

    bool onEdit(int col, int row, EditCellAction action) override;
    bool onNavigationInput(const Event& e) override;

  protected:
    char message[41];
    int confirmValue;
    int cancelValue;
};

#endif // __SCREEN_CONFIRM_H__