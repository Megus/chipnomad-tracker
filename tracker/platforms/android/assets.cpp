#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "assets_android.h"

// List of asset directories to copy
static const char* assetDirs[] = {
  "chipnomad_data/fonts",
  "chipnomad_data/instruments",
  "chipnomad_data/pitch-tables",
  "chipnomad_data/projects",
  "chipnomad_data/samples",
  "chipnomad_data/themes",
  "chipnomad_data/wavetables",
  NULL
};

// Document directory paths for Android
static const char* dataPath = "/storage/emulated/0/Documents/ChipNomad";
static const char* bundledPath = "/storage/emulated/0/Documents/ChipNomad/BundledContent";

// Android asset manager instance
static AAssetManager* assetManager = NULL;

// Get the Android asset manager instance
static AAssetManager* getAssetManager(void) {
  if (assetManager) return assetManager;

  JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
  if (!env) return NULL;

  jobject activity = (jobject)SDL_AndroidGetActivity();
  if (!activity) return NULL;

  jclass activityClass = env->GetObjectClass(activity);
  jmethodID getAssets = env->GetMethodID(activityClass, "getAssets", "()Landroid/content/res/AssetManager;");
  jobject assetManagerObj = env->CallObjectMethod(activity, getAssets);

  assetManager = AAssetManager_fromJava(env, assetManagerObj);

  env->DeleteLocalRef(activity);
  env->DeleteLocalRef(activityClass);
  env->DeleteLocalRef(assetManagerObj);

  return assetManager;
}

static bool fileExists(const char* path) {
  return access(path, F_OK) == 0;
}

static bool copyAssetFile(const char* assetPath, const char* destPath) {
  AAssetManager* mgr = getAssetManager();
  if (!mgr) return false;

  AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_BUFFER);
  if (!asset) return false;

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
    return false;
  }

  const void* buffer = AAsset_getBuffer(asset);
  size_t size = AAsset_getLength(asset);

  if (fwrite(buffer, 1, size, outFile) != size) {
    fclose(outFile);
    AAsset_close(asset);
    return false;
  }

  fclose(outFile);
  AAsset_close(asset);
  return true;
}

bool AssetsImpl::copyAssets() {
  AAssetManager* mgr = getAssetManager();
  if (!mgr) return false;

  mkdir(dataPath, 0755);
  mkdir(bundledPath, 0755);

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

  return true;
}
