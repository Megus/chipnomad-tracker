#!/bin/bash

# Check if Docker is available and running

check_docker() {
    if ! command -v docker &> /dev/null; then
        echo "Error: Docker is not installed."
        echo "Please install Docker from https://www.docker.com/get-started"
        return 1
    fi

    if ! docker info &> /dev/null; then
        echo "Error: Docker is not running."
        echo "Please start Docker and try again."
        return 1
    fi

    echo "Docker is available and running."
    return 0
}

check_docker