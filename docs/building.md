# Building EQ2 Engine

How to prepare dependencies and generate project files for the engine, the tests and the samples.
Workspace options, configurations and the project structure are described in
[architecture.md](architecture.md).

All commands below are run from the repository root. Project files are generated with premake5,
which is shipped in `utils/` (`utils/premake5.exe` for Windows, `utils/premake5` for Linux).
Use only these binaries: the engine scripts rely on the `usage` / `uses` implementation from
`utils/.premake_modules/usage`, which conflicts with the built-in usages of newer premake5
releases and works differently from them.

## Windows

### Prerequisites

- Visual Studio 2022 with the "Desktop development with C++" workload.
- Windows SDK 10.0.26100 or newer (required by Tracy). The SDK version can be set with the
  `WINSDK_VER` environment variable (for example `10.0.26100.0`); without it the latest
  installed SDK is used.
- PowerShell module `7Zip4PowerShell`, used to unpack dependency archives. Install it once with
  `utils/win_install_7zip4powershell.ps1`.

### Dependencies

```
powershell -ExecutionPolicy Bypass -File utils\windows_dev_prepare.ps1
```

The script downloads SDL2, OpenAL-soft, wxWidgets and Slang and unpacks them into `src_dependency/`.
Run it once after cloning and again after the dependency versions in the script change.
wxWidgets is not used by the engine projects, but is required by the game project that includes
the engine (`src_game`).

`utils/cross_windows_dev_prepare.sh` does the same from a Linux shell (needs `wget`, `unzip` and
`7z`), for example to prepare a Windows checkout from WSL.

### Generating projects

| Workspace | Command | Solution |
|---|---|---|
| Engine (`Equilibrium2`) | `gen_engine_vcx2022.bat` | `build/Equilibrium2.sln` |
| Tests (`auto_tests`) | `gen_tests_vcx2022.bat` | `build/auto_tests/auto_tests.sln` |
| Sample | `utils\premake5 --file=samples\GPUDriven\premake5.lua vs2022` | `samples/GPUDriven/build/GPUDriven.sln` |

The batch files run `utils\premake5 vs2022` with the corresponding script. Open the solution, select
a configuration and platform `x64` and build. From the command line:

```
msbuild /m /p:Configuration=Release /p:Platform=x64 build\auto_tests\auto_tests.sln
```

## Linux

### Prerequisites

- clang, make (`build-essential`).
- Meson, ninja and nasm to build dav1d.
- Development packages for SDL2, OpenAL-soft and libXxf86vm. On Ubuntu:

  ```
  sudo apt-get install build-essential meson ninja-build nasm libsdl2-dev libopenal-dev libxxf86vm-dev
  ```

### Dependencies

```
./utils/linux_dev_prepare.sh
```

The script downloads Slang into `src_dependency/slang/` and builds dav1d 1.5.4 from source into
`src_dependency/dav1d/`. SDL2 and OpenAL come from system packages.

### Generating projects

```
./utils/premake5 --file=premake5-tests.lua gmake2
cd build/auto_tests
make config=release_x64 -j$(nproc)
```

The root `Makefile` is generated in the workspace build directory (`build/` for the engine
workspace, `build/auto_tests/` for tests, `samples/<name>/build/` for samples). Make configuration
names are the lowercase configuration and platform: `debug_x64`, `release_x64`,
`releaseasan_x64`, `profile_x64`, `retail_x64`.

For VS Code, additionally run the `vscode` action for the same script
(`./utils/premake5 --file=samples/GPUDriven/premake5.lua vscode`), open the folder and press F5.
GUI tools (`egfMan2`) are not built on Linux.

## Output layout

Everything a workspace generates goes into its `BUILD_DIR` (see [architecture.md](architecture.md)):

| Path | Contents |
|---|---|
| `<BUILD_DIR>/<group>/<project>/` | Project files |
| `<BUILD_DIR>/obj/` | Intermediate files |
| `<BUILD_DIR>/lib/<platform>/<configuration>/` | Engine static libraries |
| `<BUILD_DIR>/thirdpartylib/<platform>/<configuration>/` | Third-party static libraries |
| `<BUILD_DIR>/bin/<platform>/<configuration>/` | Executables and shared libraries |

Test executables (`ds_tests`, `ppmem_tests`, `scripting_tests`) are placed in
`build/auto_tests/bin/x64/<configuration>/`.

## Overriding dependency paths

| Environment variable | Default (in `src_dependency/`) |
|---|---|
| `SDL2_DIR` | `SDL2` |
| `OPENAL_DIR` | `openal-soft` |
| `WINSDK_VER` | `latest` |

## Continuous integration

GitHub Actions build the tests workspace (`x64`, `Debug` and `Release`) on pushes and pull
requests to `develop`, then run every test executable listed in the workflow `TESTS` variable
from its output directory. A job fails if any test executable returns a non-zero exit code.
On Linux the tests run with `LD_LIBRARY_PATH` set to the output directory, because `e2Core` is a
shared library and the build does not set an rpath.

- `.github/workflows/msbuild.yml` — Windows, `vs2022`, `build/auto_tests/auto_tests.sln`.
- `.github/workflows/linux_build.yml` — Ubuntu, `gmake2`, `build/auto_tests/Makefile`.

`Debug` is built because PPMem is enabled only there (`ppmem_tests` skips in other configurations).

When adding a test project to `tests/premake5.lua`, add its name to `TESTS` in both workflows.
