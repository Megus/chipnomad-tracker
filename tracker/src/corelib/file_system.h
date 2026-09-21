#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#ifdef _WIN32
constexpr char kPathSeparator = '\\';
constexpr char kPathSeparatorStr[2] = "\\";
#else
constexpr char kPathSeparator = '/';
constexpr char kPathSeparatorStr[2] = "/";
#endif

constexpr int kPathMaxLength = 4096;

// Directory entry structure
struct FileEntry {
  char name[256];
  bool isDirectory;
};

// File/directory operations
class FileSystem {
  public:
    virtual ~FileSystem() = default;

    // Get platform-specific default directory for ChipNomad files.
    virtual bool getDefaultDirectory(char* buffer, int bufferSize);
    virtual const char* getSettingsPath(void);
    virtual const char* getAutosavePath(void);

    // Check if a directory exists
    virtual bool directoryExists(const char* path);

    // Create a directory
    virtual bool createDirectory(const char* path);

    // Delete a file
    virtual bool deleteFile(const char* path);

    // List directory contents with an optional comma-separated extension filter.
    // Returns a malloc'd array of FileEntry (caller must free), or NULL on error.
    // entryCount is set to the number of entries.
    virtual FileEntry* listDirectory(const char* path, const char* extensions, int* entryCount);

    virtual void extractFilenameWithoutExtension(const char* path, char* output, int maxLength);
};

#endif // __FILE_SYSTEM_H__
