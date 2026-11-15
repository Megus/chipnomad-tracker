#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <vector>

struct FileEntry;

// File/directory operations
class FileSystem {
  public:
#ifdef _WIN32
    static constexpr char kPathSeparatorChar = '\\';
    static constexpr char kPathSeparatorStr[2] = "\\";
#else
    static constexpr char kPathSeparatorChar = '/';
    static constexpr char kPathSeparatorStr[2] = "/";
#endif
    static constexpr int kPathMaxLength = 4096;
    static constexpr int kFilenameMaxLength = 256;

    virtual ~FileSystem() = default;

    // Get platform-specific default directory for ChipNomad files.
    virtual bool getDefaultDirectory(char* buffer, int bufferSize);
    virtual const char* getSettingsPath(void);
    virtual const char* getAutosavePath(void);

    virtual bool directoryExists(const char* path); // Check if a directory exists
    virtual bool fileExists(const char* path); // Check if a file exists
    virtual bool createDirectory(const char* path); // Create a directory
    virtual bool deleteFile(const char* path); // Delete a file

    // List directory contents with an optional comma-separated extension filter.
    // Returns a malloc'd array of FileEntry (caller must free), or NULL on error.
    // entryCount is set to the number of entries.
    virtual std::vector<FileEntry> listDirectory(const char* path, const char* extensions);

    virtual void extractFilenameWithoutExtension(const char* path, char* output, int maxLength);
};

// Directory entry structure
struct FileEntry {
  char name[FileSystem::kFilenameMaxLength + 1];
  bool isDirectory;
};

#endif // __FILE_SYSTEM_H__
