#!/bin/bash
# Check if SDL2.framework is properly set up for macOS builds

FRAMEWORK_PATH="platforms/macos/Frameworks/SDL2.framework"

echo "Checking SDL2.framework setup..."
echo

if [ ! -d "$FRAMEWORK_PATH" ]; then
    echo "❌ SDL2.framework not found!"
    echo
    echo "Please download SDL2.framework:"
    echo "  1. Visit https://github.com/libsdl-org/SDL/releases"
    echo "  2. Download the macOS .dmg file (e.g., SDL2-2.30.0.dmg)"
    echo "  3. Open the DMG and copy SDL2.framework to:"
    echo "     $FRAMEWORK_PATH"
    echo
    echo "See platforms/macos/Frameworks/README.md for detailed instructions."
    exit 1
fi

if [ ! -f "$FRAMEWORK_PATH/SDL2" ]; then
    echo "❌ SDL2.framework appears to be incomplete (missing SDL2 binary)"
    exit 1
fi

if [ ! -d "$FRAMEWORK_PATH/Headers" ]; then
    echo "❌ SDL2.framework appears to be incomplete (missing Headers)"
    exit 1
fi

echo "✅ SDL2.framework is properly installed"
echo

# Show framework info
SDL2_VERSION=$(otool -L "$FRAMEWORK_PATH/SDL2" 2>/dev/null | grep "SDL2.framework" | awk '{print $1}')
echo "Framework path: $SDL2_VERSION"

# Check architectures
echo
echo "Supported architectures:"
lipo -info "$FRAMEWORK_PATH/SDL2" 2>/dev/null || echo "  (Unable to check architectures)"

echo
echo "✅ Ready to build for macOS!"
