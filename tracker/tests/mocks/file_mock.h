#ifndef __FILE_MOCK_H__
#define __FILE_MOCK_H__

#include "i_file.h"
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

// FileMock: an in-memory IFile implementation for tests.
//
// It records calls and returns programmable results, so code that depends on
// IFile can be exercised without touching the real filesystem.
class FileMock : public IFile {
  public:
    // Programmable behaviour
    std::string defaultDirectory = "/mock/home";
    bool defaultDirectorySucceeds = true;

    // A directory is considered to exist if it's in this set.
    std::vector<std::string> existingDirectories;
    bool createDirectorySucceeds = true;
    bool deleteFileSucceeds = true;

    // Entries returned by listDirectory (ignores path/extension by default).
    std::vector<FileEntry> listing;

    // Call counters / last arguments for assertions
    int getDefaultDirectoryCalls = 0;
    int directoryExistsCalls = 0;
    int createDirectoryCalls = 0;
    int deleteFileCalls = 0;
    int listDirectoryCalls = 0;
    std::string lastCreatedDirectory;
    std::string lastDeletedFile;
    std::string lastListedPath;

    int getDefaultDirectory(char* buffer, int bufferSize) override {
      getDefaultDirectoryCalls++;
      if (!defaultDirectorySucceeds) return 0;
      snprintf(buffer, bufferSize, "%s", defaultDirectory.c_str());
      return 1;
    }

    int directoryExists(const char* path) override {
      directoryExistsCalls++;
      for (const auto& d : existingDirectories) {
        if (d == path) return 1;
      }
      return 0;
    }

    int createDirectory(const char* path) override {
      createDirectoryCalls++;
      lastCreatedDirectory = path;
      if (createDirectorySucceeds) existingDirectories.push_back(path);
      return createDirectorySucceeds ? 1 : 0;
    }

    int deleteFile(const char* path) override {
      deleteFileCalls++;
      lastDeletedFile = path;
      return deleteFileSucceeds ? 1 : 0;
    }

    FileEntry* listDirectory(const char* path, const char* extension, int* entryCount) override {
      (void)extension;
      listDirectoryCalls++;
      lastListedPath = path;
      if (listing.empty()) {
        *entryCount = 0;
        return nullptr;
      }
      FileEntry* out = (FileEntry*)malloc(listing.size() * sizeof(FileEntry));
      for (size_t i = 0; i < listing.size(); i++) out[i] = listing[i];
      *entryCount = (int)listing.size();
      return out;
    }

    // Helper to add a listing entry
    void addEntry(const char* name, int isDirectory) {
      FileEntry e;
      strncpy(e.name, name, 255);
      e.name[255] = 0;
      e.isDirectory = isDirectory;
      listing.push_back(e);
    }
};

#endif // __FILE_MOCK_H__
