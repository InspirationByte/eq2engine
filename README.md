# Equilibrium 2

![Equilibrium logo](https://i.ibb.co/dpyGps8/eq2-engine-logo.jpg)
A continuation of game engine/framework, fully refactored and re-structured.

**Work in progress**

## Features
#### Core
- Custom Data Structure containers such as **Array** and **Linked List** (including fixed allocated), **Map**, **Callable wrapper**, **Events**, **Promise and Future**, **Strong** and **Weak** pointers - very limited reliance on C++ std
- Own Vector math library with **Vectors**, **Planes**, **Volumes**, **Matrices**, **Quaternions**
- Multi-layer File system with custom encrypted package format (**EPK**) and LZ4 compression
- Custom Lua script binding library (**ESL**) designed for engine data structures
- Custom memory tracking allocator (**PPMEM**) for memory leak detection and limited detection of buffer overflows
- Simple multi-threading job system (**Parallel Jobs**), allowing easy setup of sequences, synchronization points
- Integration with Concurrency Visualizer to debug multithreading within the project
- Component system for adding properties to game and non-game classes
  
#### Rendering
- Material system renderer with RHI backends powered by WebGPU
- Data-driven rendering of instances done on GPU (**GRIM**)
- Custom full-featured 3D model format (**EGF**) with physics meshes and animation support
- Animation system with multiple layers, pose blending and controller features
- Debugging overlay for visualizing and console command system for activating various debug systems

#### User Interface
- Own Interface library (**EqUI**) for creating interactive in-game interfaces
- ImGui integration for debugging
- In-game console for fast console variables switching

#### Audio
- Scripted Audio System with use of nodes to enable control over playing sounds

#### Input
- Input system supporting complex multi-key binds, quake-like commands binding (+/-) and custom input handlers

#### Projects using this framework
- *The Driver Syndicate* - https://driver-syndicate.com

## Building and Running
Build system used: [Premake](https://premake.github.io/)

#### Windows
- Install VS2022
- Run `windows_dev_prepare.ps1` in repo root
- Run `premake5 vs2022`
- Open solution, located in `project_vs2022`
- Build All

#### Linux
- Install packages: `build-essential SDL2-devel openal-soft-devel libXxf86vm-devel wxBase3-devel wxGTK3-devel`
- Get premake5, extract to repo root and `chmod +x premake5`
- Open VS Code folder, install Makefile Tools and C/C++ extensions
- Run `./premake5 gmake2` in repo root
- In VS Code, press F5, wait for build to complete and game will run.

#### Android
- Install Android Studio
- Run `premake5 androidndk`
- Launch Android Studio, open `android-project`
- Build

