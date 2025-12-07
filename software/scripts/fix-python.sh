#!/bin/bash

# Quick fix for Python externally-managed-environment error
# Run this if setup.sh fails with Python package installation

echo "Installing Zephyr Python dependencies with --break-system-packages..."

ZEPHYR_DIR="${ZEPHYR_BASE:-$HOME/zephyrproject/zephyr}"

if [ ! -f "$ZEPHYR_DIR/scripts/requirements.txt" ]; then
    echo "Error: Cannot find requirements.txt at $ZEPHYR_DIR/scripts/requirements.txt"
    echo "Please set ZEPHYR_BASE or run setup.sh first"
    exit 1
fi

# Install west if not already installed
if ! command -v west &> /dev/null; then
    echo "Installing west..."
    pip3 install --user --break-system-packages west
fi

# Install Zephyr requirements
echo "Installing Zephyr requirements..."
pip3 install --user --break-system-packages -r "$ZEPHYR_DIR/scripts/requirements.txt"

echo ""
echo "Done! Now you can run: ./make.sh build"
