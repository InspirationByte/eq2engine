# Equilibrium 2
![Equilibrium logo](https://i.ibb.co/dpyGps8/eq2-engine-logo.jpg)

## Disclaimer
This project is developed solely by one developer in free time mostly for the research and recreational purposes and provided here mostly for the archival purposes.
While it demonstrates some best practices used in AA and AAA game development, some of them already outdated and should be used at your own risks.

## Features
#### Core
- Custom Data Structure containers such as **Array** and **Linked List** (including fixed allocated), **Map**, **Callable wrapper**, **Events**, **Promise and Future**, **Strong** and **Weak** pointers - very limited reliance on C++ std, built with performance in mind
- Own Vector math library with **Vectors**, **Planes**, **Volumes**, **Matrices**, **Quaternions**
- Source1-like multi-layer File system with custom encrypted package format (**EPK**) and LZ4 compression
- Custom Lua script binding library (**ESL**) designed for engine data structures. Supporting shared pointers, by-value data and native weak pointers.
- Custom memory tracking allocator (**PPMEM**) for memory leak detection and limited detection of buffer overflows
- Simple multi-threading job system (**Parallel Jobs**), allowing easy setup of sequences, synchronization points
- Integration with Concurrency Visualizer to debug multithreading within the project
  
#### Rendering
- RHI backend powered by **NVRHI** & **WebGPU**
- Batch shader compilation for multiple RHIs using Slang
- Source1-like Material System
- Data-driven rendering of instances done on GPU (**GRIM**), designed for continous data streaming
- Custom full-featured 3D model format (**EGF**) with physics meshes and animation support
- Animation system with multiple layers, pose blending and controller features
- Debugging overlay for visualizing and console command system for activating various debug systems

#### User Interface
- Own Interface library (**EqUI**) for creating interactive in-game interfaces
- ImGui integration for debugging
- In-game console

#### Audio
- Scripted Audio System with use of nodes to enable control over playing sounds
- Audio backend running on OpenAL-soft

#### Input
- Input system supporting complex multi-key binds, quake-like commands binding (+/-) and custom input handlers

#### Other features
- Custom physics dynamics engine (**EqPhysics**), utilizing Bullet Collision Library only
- Component system for adding properties to game and non-game classes

## Projects using this engine/framework
- [The Driver Syndicate](https://soapyman.itch.io/driversyndicate)

## Building and Running

#### Windows
- Install VS2022
- Run `utils/windows_dev_prepare.ps1` in repo root
- Run `utils/premake5 --file=samples\GPUDriven\premake5.lua vs2022`
- Open solution file located in `samples\GPUDriven\build`
- Build All

#### Linux
- Install packages: `build-essential SDL2-devel openal-soft-devel libXxf86vm-devel wxBase3-devel wxGTK3-devel`
- Run `utils/linux_dev_prepare.sh`
- Run `utils/premake5 --file=samples\GPUDriven\premake5.lua gmake2 && utils/premake5 --file=samples\GPUDriven\premake5.lua vscode` in repo root
- Open VS Code and open `samples\GPUDriven\build` folder, install recommended C++ extensions
- In VS Code, press F5, wait for build to complete and game will run.

#### Android
- not supported currently
