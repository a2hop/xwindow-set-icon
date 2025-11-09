#!/bin/bash

# Build script for xwindow-icon-set-gui utility

echo "Building xwindow-icon-set-gui..."

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Create build directory
mkdir -p "$PROJECT_ROOT/build"

# Get compiler flags
GLIB_FLAGS=$(pkg-config --cflags --libs glib-2.0)
GTK_FLAGS=$(pkg-config --cflags --libs gtk+-3.0 gdk-pixbuf-2.0)

# Compile the library object file first
echo "Compiling library..."
g++ -c "$PROJECT_ROOT/x-window-icon-set.cpp" -o "$PROJECT_ROOT/build/x-window-icon-set.o" \
    $GLIB_FLAGS

if [ $? -ne 0 ]; then
    echo "Library compilation failed!"
    exit 1
fi

# Compile the GUI application
echo "Compiling GUI application..."
g++ "$SCRIPT_DIR/main.cpp" "$PROJECT_ROOT/build/x-window-icon-set.o" -o "$PROJECT_ROOT/build/xwindow-icon-set-gui" \
    $GLIB_FLAGS \
    $GTK_FLAGS \
    -lX11 -lXmu -lgd \
    -std=c++17

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Build successful! Executable: $PROJECT_ROOT/build/xwindow-icon-set-gui"
    exit 0
else
    echo "Build failed!"
    exit 1
fi
