#!/bin/bash

# Build ChipNomad Linux package using Docker

set -e

echo "Building ChipNomad Linux package..."

# Check if Docker is running
if ! ./check-docker.sh; then
    echo "Error: Docker is not running or not available."
    exit 1
fi

# Build Docker image
echo "Building Docker image..."
docker build --platform linux/amd64 -f Dockerfile.linux -t chipnomad-linux-builder .

# Create build directory
mkdir -p build/linux

# Run build in Docker container
echo "Building Linux package in Docker container..."
docker run --platform linux/amd64 --rm \
    -v "$(pwd)/..:/workspace" \
    chipnomad-linux-builder \
    make -f Makefile.linux linux-package

echo "Linux package build completed!"
if [ -f build/linux/ChipNomad-*-Linux-*.tar.gz ]; then
    echo "Output: $(ls build/linux/ChipNomad-*-Linux-*.tar.gz)"
else
    echo "Error: Package not found in build/linux/"
    exit 1
fi