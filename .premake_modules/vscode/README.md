# Premake-VSCodeGen

[Visual Studio Code](https://code.visualstudio.com/) workspace generator for [Premake](https://github.com/premake/premake-core).

Supports GCC (& Clang, untested) through Make and MSVC through MSBuild.

## Usage

1. Put these files in a `vscode` subdirectory in one of [Premake's search paths](https://premake.github.io/docs/Locating-Scripts/).
2. Add the line `require "vscode"` preferably to your [premake-system.lua](https://premake.github.io/docs/System-Scripts/), or to your premake5.lua script.
3. Generate project files and then VSCode workspace

```
premake5 gmake2/vs2022
premake5 vscode
```

Note: On Windows make sure that `msbuild.exe` is accessible from CMD.

## Limitations

Currently this generater only supports a single [Premake Workspace](https://premake.github.io/docs/workspace/).  
Currently the generated tasks will only use 3/4 of the available CPU threads (guaranteed minimum is 1 thread).  
This generator only supports C/C++, all other project types are ignored.  

## What files get generated

This generator generates the following files:

- \<WorkspaceName\>.code-workspace (only if not already exists)
- .vscode/c_cpp_properties.json
- .vscode/launch.json
- .vscode/tasks.json
