#!/bin/bash

# Build SDL2 from source for Android

set -e

SDL2_SOURCE_PATH="$1"
if [ -z "$SDL2_SOURCE_PATH" ]; then
    echo "Usage: $0 /path/to/SDL2-source"
    echo "Example: $0 ~/Downloads/SDL2-2.30.9"
    exit 1
fi

if [ ! -d "$SDL2_SOURCE_PATH" ]; then
    echo "Error: SDL2 source directory not found: $SDL2_SOURCE_PATH"
    exit 1
fi

# Check NDK
if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "Error: ANDROID_NDK_ROOT not set"
    exit 1
fi

echo "Building SDL2 for Android..."
echo "SDL2 source: $SDL2_SOURCE_PATH"
echo "NDK: $ANDROID_NDK_ROOT"

# Create build directory
BUILD_DIR="build/sdl2-android"
mkdir -p "$BUILD_DIR"

# Copy SDL2 source to build directory
cp -r "$SDL2_SOURCE_PATH"/* "$BUILD_DIR/"

# Build for arm64-v8a
echo "Building for arm64-v8a..."
cd "$BUILD_DIR"
$ANDROID_NDK_ROOT/ndk-build \
    APP_ABI=arm64-v8a \
    APP_PLATFORM=android-33 \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=Android.mk

# Build for armeabi-v7a  
echo "Building for armeabi-v7a..."
$ANDROID_NDK_ROOT/ndk-build \
    APP_ABI=armeabi-v7a \
    APP_PLATFORM=android-33 \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=Android.mk

cd ../..

# Copy libraries to Android project
echo "Copying libraries to Android project..."
mkdir -p platforms/android/app/src/main/jniLibs/arm64-v8a
mkdir -p platforms/android/app/src/main/jniLibs/armeabi-v7a

cp "$BUILD_DIR/libs/arm64-v8a/libSDL2.so" platforms/android/app/src/main/jniLibs/arm64-v8a/
cp "$BUILD_DIR/libs/armeabi-v7a/libSDL2.so" platforms/android/app/src/main/jniLibs/armeabi-v7a/

# Copy SDL2 Java files
echo "Copying SDL2 Java files..."
mkdir -p platforms/android/app/src/main/java/org/libsdl/app
cp -r "$SDL2_SOURCE_PATH/android-project/app/src/main/java/org/libsdl/app"/* platforms/android/app/src/main/java/org/libsdl/app/

# Create SDL2 directory structure for headers
echo "Setting up SDL2 headers..."
mkdir -p "$BUILD_DIR/include/SDL2"
cp "$BUILD_DIR/include"/*.h "$BUILD_DIR/include/SDL2/"

echo "SDL2 build complete!"
echo "Libraries copied to platforms/android/app/src/main/jniLibs/"
echo "Java files copied to platforms/android/app/src/main/java/org/libsdl/app/"
echo "Headers available in build/sdl2-android/include/SDL2/"