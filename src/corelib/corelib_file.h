#ifndef __CORELIB_FILE_H__
#define __CORELIB_FILE_H__

#define CORELIB_MAX_OPEN_FILES (16)

// Return a fileId
int fileOpen(const char* path, int isWriting);
int fileClose(int fileId);

// Returns number of bytes read
int fileRead(int fileId, void* buffer, int maxLength);
// Reads a text line from the file
char* fileReadString(int fileId);

// Returns number of bytes written
int fileWrite(int fileId, void* data, int length);
// Returns number of bytes written
int filePrintf(int fileId, const char* format, ...);

// File browser functions
struct FileEntry {
  char name[256];
  int isDirectory;
};

// Returns allocated array of entries, NULL on error. Caller must free.
struct FileEntry* fileListDirectory(const char* path, const char* extension, int* entryCount);
// Get current working directory
int fileGetCurrentDirectory(char* buffer, int bufferSize);
// Check if directory exists
int fileDirectoryExists(const char* path);

#endif