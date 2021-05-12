# Equilibrium 2

![Equilibrium logo](https://i.ibb.co/DwPvwpv/eq-engine-logo2.jpg)
A continuation of game engine/framework in form of next generation game engine/framework

**Work in progress**

Build system used: [Xmake](https://xmake.io/)

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

#### Planned
- *MaterialSys2* based on **DiligentEngine**, add test bench projects for it
- Completely remove legacy renderers (*Direct3D9, OpenGL2.1*)
- Model rendering interfaces for best running on *MaterialSys2* pipeline
- Improve *Equilibrium Graphics File* format (caching-side, move attachments to EGF from Motion packages)
- Port *The Driver Syndicate* back on it?
- FBX support with *OpenFBX* - for EGF compiling, cutscene support etc.