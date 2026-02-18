#include "../../../chipnomad_lib/corelib/corelib_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

// macOS-specific default directory implementation
static int fileGetDefaultDirectoryMacOS(char* buffer, int bufferSize) {
    const char* home = getenv("HOME");
    if (home) {
        snprintf(buffer, bufferSize, "%s/Library/Application Support/ChipNomad", home);
        // Create directory if it doesn't exist
        mkdir(buffer, 0755);
    } else {
        snprintf(buffer, bufferSize, ".");
    }
    LOGD("Default directory: %s", buffer);
    return 0;
}

// Initialize macOS-specific file operations
void fileOpsInitPlatform(void) {
    fileOps.getDefaultDirectory = fileGetDefaultDirectoryMacOS;
}
