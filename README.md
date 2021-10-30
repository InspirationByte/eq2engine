# Equilibrium 2

![Equilibrium logo](https://i.ibb.co/DwPvwpv/eq-engine-logo2.jpg)
A continuation of game engine/framework, fully refactored and re-structured.

**Work in progress**

Build system used: [Premake](https://premake.github.io/)

#### Currently implemented
- Equilibrium Graphics Files - full-featured 3D model format with animation support
- Vector math library
- Filesystem with Zip archive and Equilibrium encrypted packages support
- Console command system
- Localization system
- User interfaces with buttons, panels, lablels
- Input system with controllers support
- Backported Equilibrium 1 MaterialSystem with Direct3D9, OpenGL 2 / ES 3.1
- Particle system
- Audio system
- EGF Animation system with multiple layers, pose blending and controller features
- FBX support with *OpenFBX* - for EGF compiling, cutscene support etc.

#### Projects using this framework
- *The Driver Syndicate* - https://driver-syndicate.com

#### Planned
- Streamlined process of building game engine and games for Mobile platforms
- Improve *Equilibrium Graphics File* format (caching-side, move attachments to EGF from Motion packages)
- *MaterialSys2* based on **DiligentEngine**, add test bench projects for it
- Completely remove legacy renderers (*Direct3D9, OpenGL2.1*)
- Model rendering interfaces for best running on *MaterialSys2* pipeline
