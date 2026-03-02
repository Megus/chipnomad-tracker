#include "../../../chipnomad_lib/corelib/corelib_file.h"
#include "asset_bundling.h"
#include <sys/stat.h>
#include <stdio.h>
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

// Android-specific default directory implementation
static int fileGetDefaultDirectoryAndroid(char* buffer, int bufferSize) {
    const char* dataPath = assetsGetDataPath();
    createDirectoryRecursive(dataPath);
    snprintf(buffer, bufferSize, "%s", dataPath);
    //LOGD("Default directory: %s", buffer);
    return 0;
}

// Initialize Android-specific file operations
void fileOpsInitPlatform(void) {
    fileOps.getDefaultDirectory = fileGetDefaultDirectoryAndroid;
}
