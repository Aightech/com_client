#!/bin/bash
# Bootstrap script for initializing the project and its dependencies
# Run this after cloning: ./scripts/setup.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

echo "Initializing submodules..."
git submodule update --init --recursive

echo "Creating build directory..."
mkdir -p build

echo ""
echo "Setup complete. To build:"
echo "  cd build"
echo "  cmake .. && make"
echo ""
echo "To build with examples:"
echo "  cmake .. -DBUILD_EXAMPLES=ON && make"
