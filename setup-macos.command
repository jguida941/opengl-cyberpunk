#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Setting up CS330Content on macOS"
echo "Repo: ${ROOT_DIR}"

if ! command -v brew >/dev/null 2>&1; then
  echo
  echo "Homebrew is not installed."
  echo "Install Homebrew first, then run this file again:"
  echo '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
  echo
  read -r -p "Press Enter to close..."
  exit 1
fi

echo
echo "Installing OpenGL dev dependencies with Homebrew..."
brew install cmake ninja pkg-config glfw glew glm

mkdir -p "${ROOT_DIR}/build"
mkdir -p "${ROOT_DIR}/out"

echo
echo "Setup complete."
echo "Next step: open README.md for project layout and run instructions."
echo
read -r -p "Press Enter to close..."
