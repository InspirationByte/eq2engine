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
