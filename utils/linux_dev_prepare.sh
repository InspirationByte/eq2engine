#!/bin/bash
set -x

SLANG_VER="2025.19.1"

# TODO: openal-soft, SDL2, X11, Wayland, wxGTK packages

wget "https://github.com/shader-slang/slang/releases/download/v${SLANG_VER}/slang-${SLANG_VER}-linux-x86_64.tar.gz"
tar -xzf ./slang-${SLANG_VER}-linux-x86_64.tar.gz -C "./src_dependency/slang/"
rm -f ./slang-${SLANG_VER}-linux-x86_64.tar.gz

echo "Done"
