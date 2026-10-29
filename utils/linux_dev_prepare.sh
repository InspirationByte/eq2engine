#!/bin/bash
set -e
set -x

SLANG_VER="2025.19.1"
DAV1D_VER="1.5.4"

# TODO: openal-soft, SDL2, X11, Wayland, wxGTK packages

wget "https://github.com/shader-slang/slang/releases/download/v${SLANG_VER}/slang-${SLANG_VER}-linux-x86_64.tar.gz"
tar -xzf ./slang-${SLANG_VER}-linux-x86_64.tar.gz -C "./src_dependency/slang/"
rm -f ./slang-${SLANG_VER}-linux-x86_64.tar.gz

# dav1d is built from source: distributions ship older versions than the engine expects
for tool in meson ninja nasm; do
	if ! command -v "$tool" >/dev/null; then
		echo "$tool is required to build dav1d (on Ubuntu: sudo apt-get install meson ninja-build nasm)" >&2
		exit 1
	fi
done

dav1d_build_dir="$(mktemp -d)"
trap 'rm -rf "$dav1d_build_dir"' EXIT

wget "https://code.videolan.org/videolan/dav1d/-/archive/${DAV1D_VER}/dav1d-${DAV1D_VER}.tar.gz" -O "${dav1d_build_dir}/dav1d.tar.gz"
tar -xzf "${dav1d_build_dir}/dav1d.tar.gz" -C "$dav1d_build_dir"

meson setup "${dav1d_build_dir}/build" "${dav1d_build_dir}/dav1d-${DAV1D_VER}" \
	--backend=ninja --buildtype=release --libdir=lib \
	-Ddefault_library=shared -Denable_tools=false -Denable_tests=false -Denable_examples=false \
	--prefix="${dav1d_build_dir}/install"
meson compile -C "${dav1d_build_dir}/build"
meson install -C "${dav1d_build_dir}/build"

cp -a "${dav1d_build_dir}/install/include/dav1d/." "./src_dependency/dav1d/include/dav1d/"
rm -rf "./src_dependency/dav1d/lib/x64"
mkdir -p "./src_dependency/dav1d/lib/x64"
cp -a "${dav1d_build_dir}/install/lib/libdav1d.so"* "./src_dependency/dav1d/lib/x64/"

echo "Done"
