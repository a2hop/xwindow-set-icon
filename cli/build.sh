#!/bin/bash

echo "Building xwindow-icon-set CLI..."

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Create build directory
mkdir -p "$PROJECT_ROOT/build"

# Get compiler flags
GLIB_FLAGS=$(pkg-config --cflags --libs glib-2.0)

# Compile the library object file first
echo "Compiling library..."
g++ -c "$PROJECT_ROOT/x-window-icon-set.cpp" -o "$PROJECT_ROOT/build/x-window-icon-set.o" \
    $GLIB_FLAGS

if [ $? -ne 0 ]; then
    echo "Library compilation failed!"
    exit 1
fi

# Compile the CLI tool
echo "Compiling CLI tool..."
g++ "$SCRIPT_DIR/main.cpp" "$PROJECT_ROOT/build/x-window-icon-set.o" -o "$PROJECT_ROOT/build/xwindow-icon-set" \
    $GLIB_FLAGS \
    -lX11 -lXmu -lgd

if [ $? -eq 0 ]; then
    echo "CLI tool built successfully: $PROJECT_ROOT/build/xwindow-icon-set"
else
    echo "Build failed!"
    exit 1
fi
