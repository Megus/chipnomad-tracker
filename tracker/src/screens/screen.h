#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "input_utils.h"
#include "gfx.h"
#include "chipnomad_lib.h"
#include "tracker_state.h"
#include "events.h"
#include "edit_common.h"

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


enum class ScreenMode : int {
  edit,
  select,
};

// Base Screen class. Instances of main screens are created once on app launch.
class Screen : public EventHandler {
  ///////////////////////////////////////////////////////////////////////////
  // Methods to override in sub-classes
  public:
    friend class ScreenPartial;

    Screen(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx): state(state), dispatcher(dispatcher), gfx(gfx) {}
    virtual ~Screen() = default;

    virtual void setup(const ScreenData& input) {}; // Setup screen before it's displayed

    virtual bool supportsSelectMode() { return false; }; // Does the screen support selection?
    virtual int getRowCount() { return 0; }; // How many rows on this screen?
    virtual int getColumnCount(int row) { return 0; }; // How many columns in this row?
    virtual ScreenPlaybackLevel getPlaybackLevel() { return ScreenPlaybackLevel::none; }; // Screen playback level. Used with the global playback input handler

    virtual bool isValidCell(int col, int row) { return true; }; // Is this cell valid?
    virtual Rect getSelectionRect(const Rect& range) { return {0, 0, 0, 0}; }; // Get screen rectangle of the selection
    virtual chipnomad::LoopRange getLoopRange() { return {}; }; // Get playback loop range

    virtual void draw() {}; // Called every frame to display highly dynamic content
    virtual void drawStatic() {}; // Draw static content
    virtual void drawCursor(int col, int row) {}; // Draw cursor
    virtual void drawRowHeader(int row, CellState cellState) {}; // Draw row header
    virtual void drawColHeader(int col, CellState cellState) {}; // Draw column header
    virtual void drawCell(int col, int row, CellState cellState) {}; // Draw a cell
    virtual bool isRowVisible(int row) { return true; }; // Is this row visible on the screen?
    virtual bool adjustVerticalScroll(bool pageJump) { return false; }; // Adjust vertical scroll, return true if the screen needs to be scrolled

    virtual bool onEdit(int col, int row, EditCellAction action) { return false; }; // Handle edit action
    virtual bool onNavigationInput(const InputEvent& event) { return false; }; // Navigation input handler. Convenience function

    virtual bool onEvent(const Event& event) override { return false; }; // Handle events from the EventDispatcher

  protected:

  ///////////////////////////////////////////////////////////////////////////
  // Common screen properties and methods
  public:
    void fullRedraw(); // Redraw the whole screen: static, headers, cells, overlay

  protected:
    TrackerState& state;
    EventDispatcher& dispatcher;
    Gfx& gfx;

    ScreenMode mode = ScreenMode::edit; // Edit / Select mode
    int cursorRow = 0; // Cursor row
    int cursorCol = 0; // Cursor column
    int selectStartRow = 0; // Selection start row
    int selectStartCol = 0; // Selection start column
    int selectAnchorRow = 0; // Where selection mode was entered
    int selectAnchorCol = 0; // Where selection mode was entered

    int optPressed = 0; // Used for input handling. TODO: Can it be removed/simplified?
    int shallowClonePressed = 0; // Used for input handling. TODO: Can it be removed/simplified?

    inline ColorTheme& theme() { return state.settings.colorTheme; }; // Convenience function to get color theme from TrackerState

    Rect getSelectionRange(); // Get selection range between cursor and selection start cell
    void drawSelectionBorder(const Rect& range, bool draw); // Draw selection border
    bool isSingleColumnSelection(); // Is the selection a single column?

    void validateCursorPosition(); // Ensure that cursor is in a valid cell
    void setCellColor(CellState state, int isEmpty, int hasContent); // Set cell color based on state and content

    int isSingleCellSelection(const Rect& range);
    int isFullColumnSelection(const Rect& range, int maxRow);
    int isFullRowSelection(const Rect& range, int maxCol);
    void selectColumn(int maxRow);
    void selectRow(int maxCol);
    void selectAll(int maxCol, int maxRow);

    void moveCursorToSelectionStart(); // Move cursor to the start of the selection (top-left corner)
    void moveCursorBelowSelection(); // Move cursor below the selection (or to the last row if selection is at the bottom)
    void drawSelectedCells(); // Draw all selected cells

    bool commonEventHandler(const Event& event); // Common event handler to call from onEvent()
    bool inputEditMode(int keys, int tapCount); // Handles common input in edit mode
    bool inputSelectMode(int keys, int tapCount);
    void inputCursor(int keys, bool* handled, bool* needsRedraw); // Handles standard cursor movement (left, right, up, down)

    // Common popups:
    void requestConfirmation(const char* title, int confirmValue, int cancelValue); // Show confirmation popup
    bool editCharacter(EditCellAction action, char* str, int idx, int maxLen); // Delete a character or open characted edit popup
    void selectFile(const char* title, const char* startPath, const char* extensions, FilePreviewer* previewer); // File browser: select a file
    void selectFolder(const char* title, const char* startPath, const char* extensions, const char* filename); // File browser: select a folder
};

#endif // __SCREEN_H__
