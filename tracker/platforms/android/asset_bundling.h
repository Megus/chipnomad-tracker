#ifndef ASSET_BUNDLING_H
#define ASSET_BUNDLING_H

// Asset bundling system for Android
// Copies bundled assets from APK to app internal storage on first run

// Initialize asset system and copy assets if needed
// Returns 0 on success, 1 on error
int assetsInit(void);

// Get the path to the app's data directory
// Returns pointer to static string with the data path
const char* assetsGetDataPath(void);

// Check if assets have been extracted
// Returns 1 if extracted, 0 if not
int assetsAreExtracted(void);

// Extract all bundled assets to internal storage
// Returns 0 on success, 1 on error
int assetsExtract(void);

#endif // ASSET_BUNDLING_H