#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "input_utils.h"
#include "gfx.h"
#include "chipnomad_lib.h"
#include "tracker_state.h"
#include "events.h"

enum class ScreenPlaybackLevel : int {
  none,
  song,
  chain,
  phrase,
};

enum class CellState : int {
  normal = 0,
  focus = 1,
  selected = 2,
};

enum class CellEditAction : int {
  clear,
  tap,
  doubleTap,
  increase,
  decrease,
  increaseBig,
  decreaseBig,
  shallowClone,
  deepClone,
  copy,
  cut,
  paste,
  switchSelection,
  multiIncrease,
  multiDecrease,
  multiIncreaseBig,
  multiDecreaseBig
};

enum class ScreenMode : int {
  edit,
  select,
};

// Base Screen class. Instances of main screens are created once on app launch.
class Screen : EventHandler {
  ///////////////////////////////////////////////////////////////////////////
  // Methods to override in sub-classes
  public:
    Screen(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx):
      state(state), events(dispatcher), gfx(gfx), optPressed(0), shallowClonePressed(0) {};
    virtual ~Screen() = default;

    virtual void setup(int input) {}; // Setup screen before it's displayed

    virtual bool supportsSelectMode() { return false; }; // Does the screen support selection?
    virtual int getRowCount() { return 0; }; // How many rows on this screen?
    virtual int getColumnCount(int row) { return 0; }; // How many columns in this row?
    virtual ScreenPlaybackLevel getPlaybackLevel() { return ScreenPlaybackLevel::none; }; // Screen playback level. Used with the global playback input handler

    virtual bool isValidCell(int col, int row) { return false; }; // Is this cell valid?
    virtual Rect getSelectionRect(Rect range) { return {0, 0, 0, 0}; }; // Get screen rectangle of the selection
    virtual chipnomad::LoopRange getLoopRange() { return {}; }; // Get playback loop range

    virtual void draw() {}; // Called every frame to display highly dynamic content
    virtual void drawStatic() {}; // Draw static content
    virtual void drawCursor(int col, int row) {}; // Draw cursor
    virtual void drawRowHeader(int row, CellState state) {}; // Draw row header
    virtual void drawColHeader(int col, CellState state) {}; // Draw column header
    virtual void drawCell(int col, int row, CellState state) {}; // Draw a cell
    virtual bool isRowVisible(int row) { return true; }; // Is this row visible on the screen?
    virtual bool adjustVerticalScroll(bool pageJump) { return false; }; // Adjust vertical scroll, return true if the screen needs to be scrolled

    virtual bool onEdit(int col, int row, CellEditAction action) {}; // Handle edit action
    virtual bool onInput(int isKeyDown, int keys, int tapCount) { return false; }; // Screen input handler
    virtual bool onNavigationInput(int isKeyDown, int keys, int tapCount) { return false; }; // Navigation input handler. Convenience function
    virtual bool onRawInput(int isKeyDown, InputCode keyCode) { return false; }; // Raw input handler (used at Key Mapping screen)

    virtual bool onEvent(Event event) override { return false; }; // Handle events from the EventDispatcher

  protected:

  ///////////////////////////////////////////////////////////////////////////
  // Common screen properties and methods
  public:
    void fullRedraw(); // Redraw the whole screen: static, headers, cells, overlay

  protected:
    TrackerState& state;
    EventDispatcher& events;
    Gfx& gfx;

    ScreenMode mode; // Edit / Select mode
    int cursorRow; // Cursor row
    int cursorCol; // Cursor column
    int selectStartRow; // Selection start row
    int selectStartCol; // Selection start column
    int selectAnchorRow; // Where selection mode was entered
    int selectAnchorCol; // Where selection mode was entered

    int optPressed; // Used for input handling. TODO: Can it be removed/simplified?
    int shallowClonePressed; // Used for input handling. TODO: Can it be removed/simplified?

    inline ColorTheme& theme(); // Convenience function to get color theme from TrackerState

    Rect getSelectionRange(); // Get selection range between cursor and selection start cell
    void drawSelectionBorder(Rect range, bool draw); // Draw selection border
    bool isSingleColumnSelection(); // Is the selection a single column?

    void validateCursorPosition(); // Ensure that cursor is in a valid cell
    void setCellColor(CellState state, int isEmpty, int hasContent); // Set cell color based on state and content

    bool commonInputHandler(int isKeyDown, int keys, int tapCount); // Common input handler to call from onInput()

    void moveCursorToSelectionStart(); // Move cursor to the start of the selection (top-left corner)
    void moveCursorBelowSelection(); // Move cursor below the selection (or to the last row if selection is at the bottom)
    void drawSelectedCells(); // Draw all selected cells

    bool inputEditMode(int keys, int tapCount); // Handles common input in edit mode
    bool inputSelectMode(int keys, int tapCount);
    void inputCursor(int keys, bool& handled, bool& needsRedraw); // Handles standard cursor movement (left, right, up, down)

    void showMessage(bool timed, const char* format, ...); // Show screen message (dispatches event)
};

#endif // __SCREEN_H__
