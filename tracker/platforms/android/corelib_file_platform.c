#include "../../../chipnomad_lib/corelib/corelib_file.h"
#include "asset_bundling.h"
#include <sys/stat.h>
#include <stdio.h>

// Android-specific default directory implementation
static int fileGetDefaultDirectoryAndroid(char* buffer, int bufferSize) {
    const char* dataPath = assetsGetDataPath();
    
    // Create directory if it doesn't exist
    mkdir(dataPath, 0755);
    
    snprintf(buffer, bufferSize, "%s", dataPath);
    LOGD("Default directory: %s", buffer);
    return 0;
}

// Initialize Android-specific file operations
void fileOpsInitPlatform(void) {
    fileOps.getDefaultDirectory = fileGetDefaultDirectoryAndroid;
}
