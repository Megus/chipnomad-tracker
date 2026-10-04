#include "doctest.h"
#include "i_file.h"
#include "file_system.h"
#include "file_mock.h"

#include <cstring>
#include <cstdlib>
#include <unistd.h>

TEST_SUITE("file") {

// ============================================================================
// FileMock through the IFile interface
// ============================================================================

TEST_CASE("mock getDefaultDirectory success") {
  FileMock mock;
  mock.defaultDirectory = "/tmp/chipnomad";
  char buf[128];
  int result = mock.getDefaultDirectory(buf, sizeof(buf));
  CHECK(result == 1);
  CHECK(std::strcmp(buf, "/tmp/chipnomad") == 0);
  CHECK(mock.getDefaultDirectoryCalls == 1);
}

TEST_CASE("mock getDefaultDirectory failure") {
  FileMock mock;
  mock.defaultDirectorySucceeds = false;
  char buf[128];
  CHECK(mock.getDefaultDirectory(buf, sizeof(buf)) == 0);
}

TEST_CASE("mock directoryExists") {
  FileMock mock;
  mock.existingDirectories.push_back("/exists");
  CHECK(mock.directoryExists("/exists") == 1);
  CHECK(mock.directoryExists("/missing") == 0);
  CHECK(mock.directoryExistsCalls == 2);
}

TEST_CASE("mock createDirectory records and reports") {
  FileMock mock;
  CHECK(mock.createDirectory("/new/dir") == 1);
  CHECK(mock.lastCreatedDirectory == "/new/dir");
  // After creation the directory should now "exist"
  CHECK(mock.directoryExists("/new/dir") == 1);

  mock.createDirectorySucceeds = false;
  CHECK(mock.createDirectory("/no") == 0);
}

TEST_CASE("mock deleteFile") {
  FileMock mock;
  CHECK(mock.deleteFile("/a/b.cnm") == 1);
  CHECK(mock.lastDeletedFile == "/a/b.cnm");
  mock.deleteFileSucceeds = false;
  CHECK(mock.deleteFile("/x") == 0);
}

TEST_CASE("mock listDirectory returns programmed entries") {
  FileMock mock;
  mock.addEntry("..", 1);
  mock.addEntry("song.cnm", 0);
  mock.addEntry("subdir", 1);

  int count = 0;
  FileEntry* entries = mock.listDirectory("/music", ".cnm", &count);
  REQUIRE(entries != nullptr);
  CHECK(count == 3);
  CHECK(std::strcmp(entries[0].name, "..") == 0);
  CHECK(entries[0].isDirectory == 1);
  CHECK(std::strcmp(entries[1].name, "song.cnm") == 0);
  CHECK(entries[1].isDirectory == 0);
  CHECK(mock.lastListedPath == "/music");
  free(entries);
}

TEST_CASE("mock listDirectory empty returns null") {
  FileMock mock;
  int count = -1;
  FileEntry* entries = mock.listDirectory("/empty", nullptr, &count);
  CHECK(entries == nullptr);
  CHECK(count == 0);
}

// ============================================================================
// The global `file` is swappable (DI seam)
// ============================================================================

TEST_CASE("global file instance can be swapped and used, then restored") {
  IFile* previous = file; // save the default FileStdio

  FileMock mock;
  mock.defaultDirectory = "/injected";
  mock.existingDirectories.push_back("/injected/exists");
  file = &mock;

  char buf[128];
  CHECK(file->getDefaultDirectory(buf, sizeof(buf)) == 1);
  CHECK(std::strcmp(buf, "/injected") == 0);

  CHECK(file->directoryExists("/injected/exists") == 1);
  CHECK(file->directoryExists("/nope") == 0);

  CHECK(file->createDirectory("/injected/new") == 1);
  CHECK(mock.lastCreatedDirectory == "/injected/new");

  CHECK(file->deleteFile("/injected/song.cnm") == 1);
  CHECK(mock.lastDeletedFile == "/injected/song.cnm");

  file = previous; // restore
  CHECK(file == previous);
}

// ============================================================================
// FileStdio smoke test (touches the real filesystem in a temp dir)
// ============================================================================

TEST_CASE("FileStdio create/exists/delete round trip") {
  FileStdio fs;
  const char* dir = "/tmp/chipnomad_file_test_dir";

  // Clean any leftover from a previous run
  rmdir(dir);

  CHECK(fs.directoryExists(dir) == 0);
  CHECK(fs.createDirectory(dir) == 1);
  CHECK(fs.directoryExists(dir) == 1);

  // Cleanup
  rmdir(dir);
}

} // TEST_SUITE("file")
