#!/bin/bash
set -x

FFMPEG_VER1="autobuild-2025-10-31-13-40"
FFMPEG_VER2="N-121583-g4348bde2d2"
SLANG_VER="2025.19.1"

# TODO: openal-soft, SDL2, X11, Wayland, wxGTK packages

wget "https://github.com/BtbN/FFmpeg-Builds/releases/download/${FFMPEG_VER1}/ffmpeg-${FFMPEG_VER2}-linux64-lgpl-shared.tar.xz"
tar -xf ./ffmpeg-${FFMPEG_VER}.tar.xz
mv ./ffmpeg-${FFMPEG_VER2}-linux64-lgpl-shared/* "./src_dependency/ffmpeg/"
rm -rf ./ffmpeg-${FFMPEG_VER2}-linux64-lgpl-shared
rm -f ./ffmpeg-${FFMPEG_VER2}.tar.xz

wget "https://github.com/shader-slang/slang/releases/download/v${SLANG_VER}/slang-${SLANG_VER}-linux-x86_64.tar.gz"
tar -xzf ./slang-${SLANG_VER}-linux-x86_64.tar.gz -C "./src_dependency/slang/"
rm -f ./slang-${SLANG_VER}-linux-x86_64.tar.gz

echo "Done"
