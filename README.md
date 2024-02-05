# Equilibrium 2

![Equilibrium logo](https://i.ibb.co/dpyGps8/eq2-engine-logo.jpg)
A continuation of game engine/framework, fully refactored and re-structured.

**Work in progress**

Build system used: [Premake](https://premake.github.io/)

#### Features
- Multi-layer File system with custom encrypted package format and LZ4 compression
- User Interface library for creating interactive in-game interfaces
- Localization system
- Equilibrium Graphics Files - full-featured 3D model format with animation support and physics meshes
- Animation system with multiple layers, pose blending and controller features
- Debugging overlay for visualizing and console command system for activating various debug systems
- Vector math library
- Input system supporting controllers and touch controls
- Material system renderer with RHI backends powered by WebGPU
- Simple Particle system
- Scripted Audio System with use of nodes to enable control over playing sounds
- Custom Data Structure containers such as Map, Array, List, Callable wrapper, Events, Promise and Future, Strong and Weak ref pointers and fixed arrays and lists
- Own Lua script binding library (ESL)
- Custom memory tracking allocator for ease of finding memory leaks
- Integration with Concurrency Visualizer to debug multithreading within the project
- Simple multi-thread job system
- Component system for adding properties to game and non-game classes

#### Projects using this framework
- *The Driver Syndicate* - https://driver-syndicate.com

## Building and Running

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

