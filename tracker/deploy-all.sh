#!/bin/bash

# Deploy all ChipNomad platforms

set -e  # Exit on any error

echo "=========================================="
echo "ChipNomad - Building All Platform Releases"
echo "=========================================="

# Check if Docker is available for cross-platform builds
if ! ./check-docker.sh; then
    echo "Error: Docker is required for cross-platform builds"
    exit 1
fi

# Clean previous builds
echo "Cleaning previous builds..."
make clean

# Build all platforms
echo ""
echo "Building macOS release..."
make macOS-deploy

echo ""
echo "Building Windows release..."
make windows-deploy

echo ""
echo "Building PortMaster release..."
make PortMaster-deploy

echo ""
echo "Building RG35xx release..."
make RG35xx-deploy

echo ""
echo "Building Linux package..."
make linux-package-deploy

echo ""
echo "=========================================="
echo "All platform releases completed!"
echo "Check the ../../releases/ directory for packages."
echo "=========================================="
