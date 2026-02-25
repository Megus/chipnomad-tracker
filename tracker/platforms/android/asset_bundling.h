#ifndef ASSET_BUNDLING_H
#define ASSET_BUNDLING_H

// Asset bundling system for Android
// Syncs bundled assets from APK to external storage on every startup

// Initialize asset system and sync bundled content
// Returns 0 on success, 1 on error
int assetsInit(void);

// Get the path to the app's data directory (Documents/ChipNomad)
// Returns pointer to static string with the data path
const char* assetsGetDataPath(void);

// Get the path to bundled content directory (Documents/ChipNomad/BundledContent)
// Returns pointer to static string with the bundled content path
const char* assetsGetBundledContentPath(void);

// Extract/sync all bundled assets to external storage
// Returns 0 on success, 1 on error
int assetsExtract(void);

#endif // ASSET_BUNDLING_H