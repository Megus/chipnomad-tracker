#include "../../../chipnomad_lib/corelib/corelib_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

static void createDirectoryRecursive(const char* path) {
    char tmp[4096];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

// macOS-specific default directory implementation
static int fileGetDefaultDirectoryMacOS(char* buffer, int bufferSize) {
    const char* home = getenv("HOME");
    if (home) {
        snprintf(buffer, bufferSize, "%s/Library/Application Support/ChipNomad", home);
        createDirectoryRecursive(buffer);
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
