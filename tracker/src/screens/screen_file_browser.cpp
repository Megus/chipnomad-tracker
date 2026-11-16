#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "screen_file_browser.h"
#include "file_system.h"
#include "misc.h"

void ScreenFileBrowser::setup(const ScreenData& input) {
  // TODO: Remember position if the folder is the same as the last one

  isFolderMode = input.isFolder;
  previewer = isFolderMode ? nullptr : input.previewer;
  copyString(title, input.title, sizeof(title) - 1);
  copyString(extensions, input.extensions, sizeof(extensions) - 1);

  if (input.folderPath && strlen(input.folderPath) > 0 && file.directoryExists(input.folderPath)) {
    copyString(currentPath, input.folderPath, sizeof(currentPath) - 1);
  } else {
    file.getDefaultDirectory(currentPath, sizeof(currentPath));
  }

  if (isFolderMode) {
    copyString(saveFilename, input.filename ? input.filename : "", sizeof(saveFilename) - 1);
    trimString(saveFilename);
  }

  state = State::normal;
  loadFolder(nullptr);
}

int ScreenFileBrowser::getRowCount() {
  return entries.size() + (isFolderMode ? 2 : 0);
}

void ScreenFileBrowser::draw() {
  if (scrollState == ScrollState::none) return;
  if (scrollUpdate()) {
    drawCell(0, cursorRow, CellState::normal);
  }
}

void ScreenFileBrowser::drawStatic() {
  gfx.setFgColor(theme().textTitles);
  gfx.print(0, 0, title);

  gfx.setFgColor(theme().textInfo);
  if (strlen(currentPath) <= 80) {
    gfx.print(0, 1, currentPath);
  } else {
    gfx.printf(0, 1, "...%s", stringTail(currentPath, 77));
  }
}

void ScreenFileBrowser::drawCell(int col, int row, CellState state) {
  int y = 3 + row - topRow;
  gfx.clearRect(0, y, 35, 1);

  // Draw cursor and set item text color
  if (row == cursorRow) {
    gfx.setFgColor(theme().textValue);
    gfx.print(0, y, ">");
  } else {
    gfx.setFgColor(theme().textDefault);
    gfx.print(0, y, " ");
  }

  if (isFolderMode && row == 0) {
    // Save to [folder name]
    static char folderName[FileSystem::kFilenameMaxLength + 1];

    // Extract the last folder name from the current path
    char* lastSep = strrchr(currentPath, FileSystem::kPathSeparatorChar);
    if (lastSep) {
      copyString(folderName, lastSep + 1, sizeof(folderName) - 1);
      if (folderName[0] == 0) strcpy(folderName, FileSystem::kPathSeparatorStr);
    } else {
      copyString(folderName, currentPath, sizeof(folderName) - 1);
    }

    // Trim folder name, if needed
    if (strlen(folderName) <= 23) {
      gfx.printf(2, y, "Save to [%s]", folderName);
    } else {
      gfx.printf(2, y, "Save to [...%.20s]", stringTail(folderName, 20));
    }
  } else if (isFolderMode && row == 1) {
    // Create folder
    gfx.print(2, y, "Create Folder");
  } else {
    // File/Folder entry
    int idx = getEntryIndexForRow(row);

    if (isFolderMode) {
      gfx.setFgColor(entries[idx].isDirectory ? theme().textDefault : theme().textInfo);
    } else {
      gfx.setFgColor(theme().textDefault);
    }

    const char* name = entries[idx].name;
    int maxDisplay = getMaxDisplayForEntry(idx);
    int isScrolling = (row == cursorRow) && scrollState != ScrollState::none;
    int isDirectory = entries[idx].isDirectory;
    int displayLen = maxDisplay;
    const char* displayName = isScrolling ? name + scrollPosition : name;

    if (isDirectory) {
      gfx.printf(2, y, "[%.*s]", displayLen, displayName);
    } else {
      gfx.printf(2, y, "%.*s", displayLen, displayName);
    }
  }
}

bool ScreenFileBrowser::isRowVisible(int row) {
  return row >= topRow && row < (topRow + 16);
}

bool ScreenFileBrowser::adjustVerticalScroll(bool pageJump) {
  bool needsRedraw = false;

  if (pageJump) {
    if (cursorRow < topRow) {
      topRow -= 16;
      if (topRow < 0) topRow = 0;
      needsRedraw = true;
    } else if (topRow + 16 <= cursorRow) {
      topRow += 16;
      if (topRow + 16 >= PROJECT_MAX_LENGTH) topRow = PROJECT_MAX_LENGTH - 16;
      needsRedraw = true;
    }
  } else {
    if (cursorRow < topRow) {
      topRow = cursorRow;
      needsRedraw = true;
    } else if (cursorRow >= topRow + 16) {
      topRow = cursorRow - 15;
      needsRedraw = true;
    }
  }

  return needsRedraw;
}

bool ScreenFileBrowser::onEvent(const Event& e) {
  if (e.type == EventType::input) {
    // Handle key-up: release state
    if (!e.isKeyDown && e.keys == 0) {
      if (state == State::editPresed) {
        // EDIT was pressed and released alone — perform file select action
        state = State::normal;
        onEntrySelected();
        return true;
      } else if (state == State::previewing) {
        // Preview key combo released — stop preview
        stopPreview();
        return true;
      }
      return false;
    }
    if (!e.isKeyDown) return false; // Ignore other key up events

    int maxIndex = entries.size() - 1;
    if (isFolderMode) maxIndex += 2;

    if (e.keys == keyUp || e.keys == keyDown || e.keys == keyLeft || e.keys == keyRight) {
      // Move cursor
      int oldCursorRow = cursorRow;
      int oldTopRow = topRow;

      if (e.keys == keyUp) {
        if (cursorRow > 0) cursorRow--;
        else { cursorRow = maxIndex; }
        if (cursorRow < topRow) topRow = cursorRow;
        else if (cursorRow >= topRow + 16) topRow = cursorRow - 16 + 1;
      } else if (e.keys == keyDown) {
        if (cursorRow < maxIndex) cursorRow++;
        else { cursorRow = 0; topRow = 0; }
        if (cursorRow >= topRow + 16) topRow = cursorRow - 16 + 1;
      } else if (e.keys == keyLeft) {
        cursorRow -= 16;
        if (cursorRow < 0) cursorRow = 0;
        if (cursorRow < topRow) topRow = cursorRow;
      } else if (e.keys == keyRight) {
        cursorRow += 16;
        if (cursorRow > maxIndex) cursorRow = maxIndex;
        if (cursorRow >= topRow + 16) topRow = cursorRow - 16 + 1;
      }

      resetScrollState();

      if (topRow != oldTopRow) {
        events.dispatch(Event::redrawScreen());
      } else {
        drawCell(0, oldCursorRow, CellState::normal);
        drawCell(0, cursorRow, CellState::focus);
      }
      return true;
    } else if (e.keys == (keyEdit | keyPlay)) {
      // Preview file under cursor
      startPreview();
      return true;
    } else if (e.keys == keyEdit) {
      // Edit is pressed: it can become a preview start or item selection
      if (state == State::normal) state = State::editPresed;
      return true;
    } else if (e.keys == keyOpt) {
      // Exit file browser
      stopPreview();
      events.dispatch(Event::popScreen(ScreenEnum::fileBrowser, ScreenData::fileBrowserOutput(false, nullptr, currentPath)));
      return true;
    }
    return false;
  } else if (e.type == EventType::screenOutput) {
    // Handle output from popups
    if (e.screen == ScreenEnum::confirm && e.screenData.intValue == 1) {
      // Overwrite file
      events.dispatch(Event::popScreen(ScreenEnum::fileBrowser, ScreenData::fileBrowserOutput(true, outputPath, currentPath)));
    } else if (e.screen == ScreenEnum::enterString && e.screenData.str != nullptr) {
      // Create folder
      if (e.screenData.intValue) {
        snprintf(outputPath, sizeof(outputPath), "%s%s%s", currentPath, FileSystem::kPathSeparatorStr, createFolderName);
        if (file.createDirectory(outputPath)) {
          loadFolder(nullptr);
          events.dispatch(Event::redrawScreen());
        }
      }
    }
    return true;
  }
  return false;
}

// Perform the EDIT action (file select / directory enter / save)
void ScreenFileBrowser::onEntrySelected() {
  // Save to...
  if (isFolderMode && cursorRow == 0) {
    // Construct full save path
    snprintf(outputPath, sizeof(outputPath), "%s%s%s%s", currentPath, FileSystem::kPathSeparatorStr, saveFilename, extensions);
    if (file.fileExists(outputPath)) {
      // File exists, ask for overwrite confirmation
      events.dispatch(Event::pushScreen(ScreenEnum::confirm, ScreenData::confirm("Overwrite existing file?", 1, 0)));
    } else {
      // File doesn't exist, good to save
      events.dispatch(Event::popScreen(ScreenEnum::fileBrowser, ScreenData::fileBrowserOutput(true, outputPath, currentPath)));
    }
  } else if (isFolderMode && cursorRow == 1) {
    // Create folder
    createFolderName[0] = 0; // Clear the string
    enterString(createFolderName, 32, "CREATE FOLDER", "Enter folder name:", ""); // TODO: Also pass the trimmed current path for convenience
  } else {
    // File list entry
    int entryIdx = getEntryIndexForRow(cursorRow);
    if (entries[entryIdx].isDirectory) {
      // Enter directory
      if (strcmp(entries[entryIdx].name, "..") == 0) {
        // Go up one level
        char* lastSeparator = strrchr(currentPath, FileSystem::kPathSeparatorChar);
        if (lastSeparator && lastSeparator != currentPath) {
          *lastSeparator = 0;
          loadFolder(nullptr); // TODO: Point to the folder we left
          events.dispatch(Event::redrawScreen());
        } else if (strlen(currentPath) > 1) {
          // Go to root (doesn't work on Windows...)
          strcpy(currentPath, FileSystem::kPathSeparatorStr);
          loadFolder(nullptr);
          events.dispatch(Event::redrawScreen());
        }
      } else {
        // Enter subdirectory
        int len = strlen(currentPath);
        if (len > 0 && currentPath[len - 1] != FileSystem::kPathSeparatorChar) {
          strcat(currentPath, FileSystem::kPathSeparatorStr);
        }
        strcat(currentPath, entries[entryIdx].name);
      }
      loadFolder(nullptr);
      events.dispatch(Event::redrawScreen());
    } else if (!isFolderMode && entryIdx >= 0) {
      // Select file (only in file mode)
      stopPreview();
      snprintf(outputPath, sizeof(outputPath), "%s%s%s", currentPath, FileSystem::kPathSeparatorStr, entries[entryIdx].name);
      events.dispatch(Event::popScreen(ScreenEnum::fileBrowser, ScreenData::fileBrowserOutput(false, outputPath, currentPath)));
    }
  }
}

void ScreenFileBrowser::loadFolder(const char* selectName) {
  entries = file.listDirectory(currentPath, extensions);
  if (entries.size() > 0) {
    std::sort(entries.begin(), entries.end(), [](const FileEntry& a, const FileEntry& b) {
      // Directories first
      if (a.isDirectory && !b.isDirectory) return true;
      if (!a.isDirectory && b.isDirectory) return false;
      // Then alphabetical
      return strcmp(a.name, b.name) < 0;
    });
  }

  cursorRow = 0;
  topRow = 0;

  // Find the specified entry and select it
  if (selectName && strlen(selectName) > 0) {
    for (int c = 0; c < entries.size(); c++) {
      if (strcmp(entries[c].name, selectName) == 0) {
        cursorRow = isFolderMode ? c + 2 : c;
        adjustVerticalScroll(true);
        break;
      }
    }
  }

  resetScrollState();
}

void ScreenFileBrowser::startPreview() {
  int entryIdx = getEntryIndexForRow(cursorRow);
  if (previewer == nullptr || entryIdx < 0 || entries[entryIdx].isDirectory) return;

  char fullPath[FileSystem::kPathMaxLength];
  snprintf(fullPath, sizeof(fullPath), "%s%s%s", currentPath, FileSystem::kPathSeparatorStr, entries[entryIdx].name);
  bool started = previewer->startFilePreview(fullPath);
  state = started ? State::previewing : State::normal;
}

void ScreenFileBrowser::stopPreview() {
  if (previewer != nullptr) {
    previewer->stopFilePreview();
  }
  state = State::normal;
}

int ScreenFileBrowser::getEntryIndexForRow(int itemIndex) {
  if (isFolderMode && (itemIndex == 0 || itemIndex == 1)) return -1;
  return isFolderMode ? itemIndex - 2 : itemIndex;
}

int ScreenFileBrowser::getMaxDisplayForEntry(int entryIdx) {
  return entries[entryIdx].isDirectory ? kMaxLenDir : kMaxLenFile;
}

void ScreenFileBrowser::resetScrollState() {
  int idx = getEntryIndexForRow(cursorRow);
  if (idx < 0) {
    scrollState = ScrollState::none; // No scrolling of non-file items
    return;
  }

  const char* name = entries[idx].name;
  int maxDisplay = getMaxDisplayForEntry(idx);
  int length = strlen(name);
  if (length <= maxDisplay) {
    scrollState = ScrollState::none; // No scrolling for short items
    return;
  }

  scrollItemLength = length;
  scrollIdemDisplayLength = maxDisplay;
  scrollPosition = 0;
  scrollState = ScrollState::initialWait;
  scrollTimer = kScrollDelayFrames;
}

bool ScreenFileBrowser::scrollUpdate() {
  if (scrollTimer > 0) {
    scrollTimer--;
    return false;
  }

  switch (scrollState) {
    case ScrollState::initialWait:
      scrollState = ScrollState::scrollLeft;
      scrollTimer = kScrollSpeedFrames;
      break;
    case ScrollState::scrollLeft:
      if (scrollPosition < (scrollItemLength - scrollIdemDisplayLength)) {
        scrollTimer = kScrollSpeedFrames;
        scrollPosition++;
      } else {
        scrollState = ScrollState::waitEnd;
        scrollTimer = kScrollBoundaryWaitFrames;
      }
      break;
    case ScrollState::waitEnd:
      scrollState = ScrollState::scrollRight;
      scrollTimer = kScrollSpeedFrames;
      break;
    case ScrollState::scrollRight:
      if (scrollPosition > 0) {
        scrollTimer = kScrollSpeedFrames;
        scrollPosition--;
      } else {
        scrollState = ScrollState::waitStart;
        scrollTimer = kScrollBoundaryWaitFrames;
      }
      break;
    case ScrollState::waitStart:
      scrollState = ScrollState::scrollLeft;
      scrollTimer = kScrollSpeedFrames;
      break;
    default:
      scrollState = ScrollState::none;
      break;
  }
  return true;
}
