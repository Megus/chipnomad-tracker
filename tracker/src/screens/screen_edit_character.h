#ifndef __SCREEN_EDIT_CHARACTER_H__
#define __SCREEN_EDIT_CHARACTER_H__

#include "screen.h"

class ScreenEditCharacter : public Screen {
  public:
    ScreenEditCharacter(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx): Screen(state, dispatcher, gfx) {};

    void setup(const ScreenData& input) override;
    int getRowCount() override { return 4; };
    int getColumnCount(int row) override { return 10; };

    void drawStatic() override;
    void drawCursor(int col, int row) override;
    void drawCell(int col, int row, CellState state) override;

    bool onEvent(const Event& e) override;

  protected:
    bool isUppercase = true;
    bool shiftFlag = false;
    char* str = nullptr;
    int charIdx = 0;
    int maxLength = 0;

    inline const char** keyboardRows();
};

#endif // __SCREEN_EDIT_CHARACTER_H__