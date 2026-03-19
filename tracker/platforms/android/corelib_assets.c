#include "../src/corelib/corelib_assets.h"
#include "../../chipnomad_lib/corelib/corelib_file.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static AAssetManager* assetManager = NULL;

static AAssetManager* getAssetManager(void) {
    if (assetManager) return assetManager;

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env) return NULL;

    jobject activity = (jobject)SDL_AndroidGetActivity();
    if (!activity) return NULL;

    jclass activityClass = (*env)->GetObjectClass(env, activity);
    jmethodID getAssets = (*env)->GetMethodID(env, activityClass, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject assetManagerObj = (*env)->CallObjectMethod(env, activity, getAssets);

    assetManager = AAssetManager_fromJava(env, assetManagerObj);

    (*env)->DeleteLocalRef(env, activity);
    (*env)->DeleteLocalRef(env, activityClass);
    (*env)->DeleteLocalRef(env, assetManagerObj);

    return assetManager;
}

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

static int copyAssetFile(const char* assetPath, const char* destPath) {
    AAssetManager* mgr = getAssetManager();
    if (!mgr) return 1;

    AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_BUFFER);
    if (!asset) return 1;

    char dirPath[512];
    strcpy(dirPath, destPath);
    char* lastSlash = strrchr(dirPath, '/');
    if (lastSlash) {
        *lastSlash = 0;
        mkdir(dirPath, 0755);
    }

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

int assetsInit(void) {
    if (!getAssetManager()) return 1;

    const char* dataPath = "/storage/emulated/0/Documents/ChipNomad";
    const char* bundledPath = "/storage/emulated/0/Documents/ChipNomad/BundledContent";

    mkdir(dataPath, 0755);
    mkdir(bundledPath, 0755);

    AAssetManager* mgr = getAssetManager();

    for (int i = 0; assetDirs[i] != NULL; i++) {
        const char* assetDir = assetDirs[i];
        const char* subpath = assetDir + 15; // Strip "chipnomad_data/" prefix

        char destDir[512];
        snprintf(destDir, sizeof(destDir), "%s/%s", bundledPath, subpath);
        mkdir(destDir, 0755);

        AAssetDir* dir = AAssetManager_openDir(mgr, assetDir);
        if (!dir) continue;

        const char* filename;
        while ((filename = AAssetDir_getNextFileName(dir)) != NULL) {
            char assetPath[512];
            char destPath[512];
            snprintf(assetPath, sizeof(assetPath), "%s/%s", assetDir, filename);
            snprintf(destPath, sizeof(destPath), "%s/%s", destDir, filename);

            if (!fileExists(destPath)) {
                copyAssetFile(assetPath, destPath);
            }
        }

        AAssetDir_close(dir);
    }

    return 0;
}
