#ifndef __SCREEN_ENTER_STRING_H__
#define __SCREEN_ENTER_STRING_H__

#include "screen.h"

class ScreenEnterString : public Screen {
  public:
    ScreenEnterString(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx): Screen(state, dispatcher, gfx) {};

    void setup(const ScreenData& input) override;
    int getRowCount() override { return 2; };
    int getColumnCount(int row) override;

    void drawStatic() override;
    void drawCell(int col, int row, CellState state) override;
    void drawCursor(int col, int row) override;

    bool onEdit(int col, int row, EditCellAction action) override;
    bool onEvent(const Event& e) override;

  protected:
    char title[41]; // Screen title
    char prompt[41]; // Screen prompt
    char detail[81]; // Additional detail (up to 2 rows), e.g. folder path when entering folder name
    int maxLength;
    char* string; // String to edit, passed by caller
};

#endif // __SCREEN_ENTER_STRING_H__