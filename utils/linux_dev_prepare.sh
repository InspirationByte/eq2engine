#!/bin/bash
set -x

FFMPEG_VER="master-latest-linux64-lgpl-shared"

# TODO: openal-soft, SDL2, X11, Wayland, wxGTK packages

wget "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-${FFMPEG_VER}.tar.xz"
tar -xf ./ffmpeg-${FFMPEG_VER}.tar.xz

mv ./ffmpeg-${FFMPEG_VER}/* "./src_dependency/ffmpeg/"
rm -rf ./ffmpeg-${FFMPEG_VER}
rm -f ./ffmpeg-${FFMPEG_VER}.tar.xz

echo "Done"
