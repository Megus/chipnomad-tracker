#include "asset_bundling.h"
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

int assetsAreExtracted(void) {
    char markerPath[512];
    snprintf(markerPath, sizeof(markerPath), "%s/.assets_extracted", assetsGetDataPath());
    return access(markerPath, F_OK) == 0;
}

// Copy a single asset file
static int copyAssetFile(const char* assetPath, const char* destPath) {
    AAssetManager* mgr = getAssetManager();
    if (!mgr) return 1;
    
    AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_BUFFER);
    if (!asset) return 1;
    
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
        AAsset_close(asset);
        return 1;
    }
    
    const void* buffer = AAsset_getBuffer(asset);
    size_t size = AAsset_getLength(asset);
    
    if (fwrite(buffer, 1, size, outFile) != size) {
        fclose(outFile);
        AAsset_close(asset);
        return 1;
    }
    
    fclose(outFile);
    AAsset_close(asset);
    return 0;
}

// Copy assets recursively
static int copyAssetsRecursive(const char* assetDir, const char* destDir) {
    AAssetManager* mgr = getAssetManager();
    if (!mgr) return 1;
    
    AAssetDir* dir = AAssetManager_openDir(mgr, assetDir);
    if (!dir) return 1;
    
    mkdir(destDir, 0755);
    
    const char* filename;
    while ((filename = AAssetDir_getNextFileName(dir)) != NULL) {
        char assetPath[512];
        char destPath[512];
        
        if (strlen(assetDir) > 0) {
            snprintf(assetPath, sizeof(assetPath), "%s/%s", assetDir, filename);
        } else {
            snprintf(assetPath, sizeof(assetPath), "%s", filename);
        }
        snprintf(destPath, sizeof(destPath), "%s/%s", destDir, filename);
        
        // Try to open as directory first
        AAssetDir* subdir = AAssetManager_openDir(mgr, assetPath);
        if (subdir) {
            // Check if directory has contents
            const char* subdirFile = AAssetDir_getNextFileName(subdir);
            AAssetDir_close(subdir);
            
            if (subdirFile) {
                // It's a directory with contents - recurse
                copyAssetsRecursive(assetPath, destPath);
            } else {
                // It's a file - copy it
                copyAssetFile(assetPath, destPath);
            }
        } else {
            // It's a file - copy it
            copyAssetFile(assetPath, destPath);
        }
    }
    
    AAssetDir_close(dir);
    return 0;
}

int assetsExtract(void) {
    const char* dataDir = assetsGetDataPath();
    
    // Create main ChipNomad directory
    mkdir(dataDir, 0755);
    
    // Copy all contents from chipnomad_data directly to the main folder
    if (copyAssetsRecursive("chipnomad_data", dataDir) != 0) {
        return 1;
    }
    
    // Create marker file
    char markerPath[512];
    snprintf(markerPath, sizeof(markerPath), "%s/.assets_extracted", dataDir);
    FILE* marker = fopen(markerPath, "w");
    if (marker) {
        fprintf(marker, "1\n");
        fclose(marker);
    }
    
    return 0;
}

int assetsInit(void) {
    // Initialize asset manager
    if (!getAssetManager()) {
        return 1;
    }
    
    // Extract assets if not already done
    if (!assetsAreExtracted()) {
        printf("ChipNomad: Extracting assets to %s\n", assetsGetDataPath());
        int result = assetsExtract();
        if (result == 0) {
            printf("ChipNomad: Assets extracted successfully\n");
        } else {
            printf("ChipNomad: Asset extraction failed\n");
        }
        return result;
    } else {
        printf("ChipNomad: Assets already extracted to %s\n", assetsGetDataPath());
    }
    
    return 0;
}