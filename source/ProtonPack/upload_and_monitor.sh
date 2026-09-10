#!/bin/bash

# ProtonPack Upload and Monitor Script
# This script compiles, uploads, and monitors the ProtonPack firmware in one go

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="/dev/cu.usbmodem1101"
BAUD="115200"
ENV="esp32s3"

echo "=========================================="
echo "ProtonPack Upload & Monitor"
echo "=========================================="
echo "Project: $SCRIPT_DIR"
echo "Port: $PORT"
echo "Baud: $BAUD"
echo "Environment: $ENV"
echo ""

# Step 1: Clean
echo "[1/3] Cleaning old build..."
platformio run -d "$SCRIPT_DIR" --target clean --environment "$ENV"

if [ $? -ne 0 ]; then
    echo "Clean failed!"
    exit 1
fi

echo ""

# Step 2: Upload
echo "[2/3] Uploading firmware..."
platformio run -d "$SCRIPT_DIR" --target upload --environment "$ENV"

if [ $? -ne 0 ]; then
    echo "Upload failed!"
    exit 1
fi

echo ""
echo "[3/3] Starting serial monitor..."
echo "Press Ctrl+C to stop monitoring"
echo ""

platformio device monitor -d "$SCRIPT_DIR" --port "$PORT" --baud "$BAUD"
