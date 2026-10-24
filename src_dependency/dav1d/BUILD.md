# dav1d

Prebuilt dav1d 1.5.4 (https://code.videolan.org/videolan/dav1d, tag `1.5.4`), release, shared, with asm.

## Windows x64 (MSVC 2022)

Requires meson (`pip install meson`), ninja and nasm in `PATH`. Run from the "x64 Native Tools" prompt:

```
meson setup build <dav1d_src> --backend=ninja --buildtype=release -Ddefault_library=shared -Denable_tools=false -Denable_tests=false -Denable_examples=false --prefix=<install_dir>
meson compile -C build
meson install -C build
```

Copy `<install_dir>/include/dav1d` to `include/`, `lib/dav1d.lib` to `lib/`, `bin/dav1d.dll` to `bin/`.

## Movie content

`movieLib` plays WebM files with 8 bit 4:2:0 AV1 video and Vorbis audio (mono or stereo). Convert videos with:

```
ffmpeg -i in.mp4 -c:v libsvtav1 -crf 35 -preset 6 -g 240 -pix_fmt yuv420p -c:a libvorbis -q:a 5 out.webm
```

The player also needs the `MovieYUV` shader (`shadersSRC/MovieYUV.def`, `source_MovieYUV.fx`) in the engine data.
