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
- Material system renderer with RHI backends Direct3D9, OpenGL 2 / ES 3.1
- Flexible Particle system
- Scripted Audio System with use of nodes to enable control over playing sounds
- Custom Data Structure containers such as Map, Array, List, Callable wrapper, Events, Promise and Future, Strong and Weak ref pointers and fixed arrays and lists
- Custom memory tracking allocator for ease of finding memory leaks
- Integration with Concurrency Visualizer to debug multithreading within the project
- Simple multi-thread job system
- Component system for adding properties to game and non-game classes

#### Projects using this framework
- *The Driver Syndicate* - https://driver-syndicate.com
