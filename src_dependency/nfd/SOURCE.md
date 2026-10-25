# nfd

Native File Dialog Extended from https://github.com/btzy/nativefiledialog-extended, tag `v1.4.1`
(commit `65b2f070a0fcfee0d453a64e770a217ab0b6ff4f`). Only `src/include/nfd.h`, `nfd.hpp`,
`nfd_sdl2.h`, `src/nfd_win.cpp`, `src/nfd_portal.cpp`, `src/nfd_linux_shared.hpp` and `LICENSE`
(as `LICENSE.txt`) are taken; the files are unmodified.

Backends: Win32 `IFileDialog` on Windows, xdg-desktop-portal (D-Bus) on Linux, built without
`NFD_X11` / `NFD_WAYLAND`, so dialogs are not parented to the application window on Linux.
