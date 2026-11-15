#ifndef __FILE_BROWSER_H__
#define __FILE_BROWSER_H__

#include <vector>
#include "screen.h"
#include "file_system.h"

class ScreenFileBrowser : public Screen {
  public:
    ScreenFileBrowser(TrackerState& state, EventDispatcher& dispatcher, Gfx& gfx, FileSystem& file): Screen(state, dispatcher, gfx), file(file) {};

    void setup(const ScreenData& input) override;
    int getRowCount() override;
    int getColumnCount(int row) override { return 1; };
    void draw() override;
    void drawStatic() override;
    void drawCell(int col, int row, CellState state) override;
    bool isRowVisible(int row) override;
    bool adjustVerticalScroll(bool pageJump) override;
    bool onEvent(const Event& e) override;

  protected:
    static constexpr int kScrollDelayFrames = 120;
    static constexpr int kScrollSpeedFrames = 8;
    static constexpr int kBoundaryWaitFrames = kScrollSpeedFrames * 2;
    static constexpr int kMaxLenDir = 31; // Maximal string length for directory name
    static constexpr int kMaxLenFile = 33; // Maximal string length for file name

    enum class State {
      normal,
      editPresed,
      previewing
    };

    enum class ScrollState {
      none,
      initialWait,
      scrollLeft,
      waitEnd,
      scrollRight,
      waitStart,
    };

    FileSystem& file;

    char title[41];
    char extensions[65];
    char saveFilename[FileSystem::kFilenameMaxLength + 1];
    char createFolderName[FileSystem::kFilenameMaxLength + 1];
    bool isFolderMode = false;
    FilePreviewer* previewer;

    char currentPath[FileSystem::kPathMaxLength + 1];
    char outputPath[FileSystem::kPathMaxLength + 1];
    std::vector<FileEntry> entries;
    State state = State::normal;
    int topRow = 0;

    ScrollState scrollState = ScrollState::none;
    int scrollTimer;
    int scrollPosition;
    int scrollItemLength;
    int scrollIdemDisplayLength;

    void loadFolder(const char* selectName);
    int getEntryIndexForRow(int itemIndex);
    int getMaxDisplayForEntry(int entryIdx);

    void startPreview();
    void stopPreview();
    void onEntrySelected();
};

#endif // __FILE_BROWSER_H__
