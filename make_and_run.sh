#!/usr/bin/env bash
# POSIX wrapper to run the build from the project root
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR/build"

echo "Running: make clean"
make clean

echo "Running: make all"
make all

echo "Build finished."

../main
