#ifndef __SCREEN_PROJECT_AY_H__
#define __SCREEN_PROJECT_AY_H__

#include "screen_partial.h"

class ScreenProject;

class ScreenPartialProjectAY : public ScreenPartial<ScreenProject> {
  public:
    ScreenPartialProjectAY(ScreenProject& screen, int startRow): ScreenPartial<ScreenProject>(screen, startRow) {};
    ~ScreenPartialProjectAY() = default;

    int getRowCount() override { return 6; }
    int getColumnCount(int row) override { return 1; };

    void drawStatic() override;
    void drawCursor(int col, int row) override;
    void drawCell(int col, int row, CellState state) override;

    bool onEdit(int col, int row, EditCellAction action) override;

  protected:
    char stereoModes[3][5] = {
      "ABC",
      "ACB",
      "BAC",
    };

    int clockPresets[5] = {
      750000,    // 0.75 MHz
      1000000,   // 1 MHz
      1750000,   // 1.75 MHz
      1773400,   // 1.7734 MHz
      2000000    // 2 MHz
    };

    char clockNames[5][11] = {
      "0.75 MHz",
      "1 MHz",
      "1.75 MHz",
      "1.7734 MHz",
      "2 MHz"
    };

    int getClockPresetIndex(int clock);
};

#endif // __SCREEN_PROJECT_AY_H__