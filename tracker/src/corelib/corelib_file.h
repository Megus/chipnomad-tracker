#ifndef CORELIB_FILE_H
#define CORELIB_FILE_H

// Platform-specific file and directory utilities
// For general file I/O, use standard stdio.h functions

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

// Directory entry structure
typedef struct {
  char name[256];
  int isDirectory;
} FileEntry;

// Get platform-specific default directory for ChipNomad files
// Returns 0 on success, -1 on failure
int fileGetDefaultDirectory(char* buffer, int bufferSize);

// Check if a directory exists
// Returns 1 if exists, 0 if not
int fileDirectoryExists(const char* path);

// Create a directory
// Returns 0 on success, -1 on failure
int fileCreateDirectory(const char* path);

// Delete a file
// Returns 0 on success, -1 on failure
int fileDelete(const char* path);

// List directory contents with optional extension filter
// Returns array of FileEntry (caller must free), or NULL on error
// entryCount is set to number of entries
FileEntry* fileListDirectory(const char* path, const char* extension, int* entryCount);

#endif // CORELIB_FILE_H
