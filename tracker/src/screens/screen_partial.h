#ifndef __SCREEN_PARTIAL_H__
#define __SCREEN_PARTIAL_H__

#include "screen.h"

template <class T>
class ScreenPartial {
  public:
    ScreenPartial(T& screen, int startRow): screen(screen), startRow(startRow) {};
    virtual ~ScreenPartial() = default;

    virtual int getRowCount() { return 0; }; // How many rows on this partial?
    virtual int getColumnCount(int row) { return 0; }; // How many columns in this row?

    virtual bool isValidCell(int col, int row) { return true; }; // Is this cell valid?

    virtual void draw() {}; // Called every frame to display highly dynamic content
    virtual void drawStatic() {}; // Draw static content
    virtual void drawCursor(int col, int row) {}; // Draw cursor
    virtual void drawRowHeader(int row, CellState state) {}; // Draw row header
    virtual void drawColHeader(int col, CellState state) {}; // Draw column header
    virtual void drawCell(int col, int row, CellState state) {}; // Draw a cell

    virtual bool onEdit(int col, int row, EditCellAction action) { return false; }; // Handle edit action

    virtual bool onEvent(const Event& e) { return false; }; // Handle events from the EventDispatcher

  protected:
    T& screen;
    int startRow;
};

#endif // __SCREEN_PARTIAL_H__