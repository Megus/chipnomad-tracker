#include "asset_bundling.h"
#include "../../../chipnomad_lib/corelib/corelib_file.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char dataPath[512] = {0};
static AAssetManager* assetManager = NULL;

// Get asset manager from SDL
static AAssetManager* getAssetManager(void) {
    if (assetManager) return assetManager;
    
    // Get JNI environment from SDL
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env) return NULL;
    
    // Get activity from SDL
    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!activity) return NULL;
    
    // Get AssetManager
    jclass activityClass = (*env)->GetObjectClass(env, activity);
    jmethodID getAssets = (*env)->GetMethodID(env, activityClass, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject assetManagerObj = (*env)->CallObjectMethod(env, activity, getAssets);
    
    assetManager = AAssetManager_fromJava(env, assetManagerObj);
    
    (*env)->DeleteLocalRef(env, activity);
    (*env)->DeleteLocalRef(env, activityClass);
    (*env)->DeleteLocalRef(env, assetManagerObj);
    
    return assetManager;
}

const char* assetsGetDataPath(void) {
    if (dataPath[0] == 0) {
        // Use the shared Documents folder that all apps and users can access
        // This is typically /storage/emulated/0/Documents/ChipNomad
        snprintf(dataPath, sizeof(dataPath), "/storage/emulated/0/Documents/ChipNomad");
    }
    return dataPath;
}

const char* assetsGetBundledContentPath(void) {
    static char bundledPath[512] = {0};
    if (bundledPath[0] == 0) {
        snprintf(bundledPath, sizeof(bundledPath), "%s/BundledContent", assetsGetDataPath());
    }
    return bundledPath;
}

// Known asset directories
static const char* assetDirs[] = {
    "chipnomad_data/fonts",
    "chipnomad_data/instruments", 
    "chipnomad_data/pitch-tables",
    "chipnomad_data/projects",
    "chipnomad_data/themes",
    NULL
};

static int fileExists(const char* path) {
    return access(path, F_OK) == 0;
}

// Copy a single asset file
static int copyAssetFile(const char* assetPath, const char* destPath) {
    AAssetManager* mgr = getAssetManager();
    if (!mgr) {
        LOGD("No asset manager");
        return 1;
    }
    
    AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGD("Failed to open asset: %s", assetPath);
        return 1;
    }
    
    // Create destination directory if needed
    char dirPath[512];
    strcpy(dirPath, destPath);
    char* lastSlash = strrchr(dirPath, '/');
    if (lastSlash) {
        *lastSlash = 0;
        mkdir(dirPath, 0755);
    }
    
    // Copy file
    FILE* outFile = fopen(destPath, "wb");
    if (!outFile) {
        LOGD("Failed to create file: %s", destPath);
        AAsset_close(asset);
        return 1;
    }
    
    const void* buffer = AAsset_getBuffer(asset);
    size_t size = AAsset_getLength(asset);
    
    if (fwrite(buffer, 1, size, outFile) != size) {
        LOGD("Failed to write file: %s", destPath);
        fclose(outFile);
        AAsset_close(asset);
        return 1;
    }
    
    LOGD("Copied %s -> %s (%zu bytes)", assetPath, destPath, size);
    fclose(outFile);
    AAsset_close(asset);
    return 0;
}

// Copy assets recursively, skipping files that already exist
static int copyAssetsRecursive(const char* assetDir, const char* destDir) {
    AAssetManager* mgr = getAssetManager();
    if (!mgr) return 1;
    
    LOGD("Opening asset dir: %s", assetDir);
    AAssetDir* dir = AAssetManager_openDir(mgr, assetDir);
    if (!dir) {
        LOGD("Failed to open asset dir: %s", assetDir);
        return 1;
    }
    
    mkdir(destDir, 0755);
    
    int fileCount = 0;
    const char* filename;
    while ((filename = AAssetDir_getNextFileName(dir)) != NULL) {
        fileCount++;
        LOGD("Found entry #%d: %s", fileCount, filename);
        
        char assetPath[512];
        char destPath[512];
        
        if (strlen(assetDir) > 0) {
            snprintf(assetPath, sizeof(assetPath), "%s/%s", assetDir, filename);
        } else {
            snprintf(assetPath, sizeof(assetPath), "%s", filename);
        }
        snprintf(destPath, sizeof(destPath), "%s/%s", destDir, filename);
        
        LOGD("Trying to open as file: %s", assetPath);
        // Try to open as file first
        AAsset* testAsset = AAssetManager_open(mgr, assetPath, AASSET_MODE_STREAMING);
        if (testAsset) {
            // It's a file
            AAsset_close(testAsset);
            LOGD("Confirmed as file: %s", assetPath);
            
            // Skip if file already exists
            if (fileExists(destPath)) {
                LOGD("Skipping existing file: %s", destPath);
            } else {
                copyAssetFile(assetPath, destPath);
            }
        } else {
            // It's a directory - recurse
            LOGD("Treating as directory: %s", assetPath);
            copyAssetsRecursive(assetPath, destPath);
        }
    }
    
    LOGD("Finished directory %s - found %d entries", assetDir, fileCount);
    AAssetDir_close(dir);
    return 0;
}

int assetsExtract(void) {
    const char* bundledPath = assetsGetBundledContentPath();
    AAssetManager* mgr = getAssetManager();
    
    // Create main ChipNomad directory
    mkdir(assetsGetDataPath(), 0755);
    
    // Create BundledContent directory  
    mkdir(bundledPath, 0755);
    
    int totalCopied = 0;
    int totalSkipped = 0;
    
    // Process each known directory
    for (int i = 0; assetDirs[i] != NULL; i++) {
        const char* assetDir = assetDirs[i];
        const char* subpath = assetDir + 15; // Strip "chipnomad_data/" prefix
        
        char destDir[512];
        snprintf(destDir, sizeof(destDir), "%s/%s", bundledPath, subpath);
        mkdir(destDir, 0755);
        
        LOGD("Processing directory: %s", assetDir);
        
        // List files in this directory
        AAssetDir* dir = AAssetManager_openDir(mgr, assetDir);
        if (!dir) {
            LOGD("Failed to open asset dir: %s", assetDir);
            continue;
        }
        
        const char* filename;
        int fileCount = 0;
        while ((filename = AAssetDir_getNextFileName(dir)) != NULL) {
            fileCount++;
            
            char assetPath[512];
            char destPath[512];
            snprintf(assetPath, sizeof(assetPath), "%s/%s", assetDir, filename);
            snprintf(destPath, sizeof(destPath), "%s/%s", destDir, filename);
            
            // Skip if file already exists
            if (fileExists(destPath)) {
                totalSkipped++;
                continue;
            }
            
            // Copy the file
            if (copyAssetFile(assetPath, destPath) == 0) {
                totalCopied++;
            }
        }
        
        LOGD("Found %d files in %s", fileCount, assetDir);
        AAssetDir_close(dir);
    }
    
    LOGD("Asset sync complete: %d copied, %d skipped", totalCopied, totalSkipped);
    return 0;
}

int assetsInit(void) {
    // Initialize asset manager
    if (!getAssetManager()) {
        return 1;
    }
    
    // Always sync bundled content on startup
    LOGD("Syncing bundled content to %s", assetsGetBundledContentPath());
    int result = assetsExtract();
    if (result == 0) {
        LOGD("Bundled content synced successfully");
    } else {
        LOGD("Bundled content sync failed");
    }
    return result;
}