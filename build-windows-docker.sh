#!/bin/bash

# Build Windows executable using Docker cross-compilation

set -e  # Exit on any error

echo "Building Windows cross-compilation Docker image..."
docker build -f Dockerfile.windows -t chipnomad-windows-builder .

echo "Running Windows build in Docker container..."
docker run --rm -v "$(pwd):/workspace" chipnomad-windows-builder make -f Makefile.windows .windows-docker

echo "Windows build completed in build/ directory"