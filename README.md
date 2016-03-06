# Equilibrium Engine #

The game engine/framework originally developed for White Cage

Folder overview
-------------

* **android_lib_mk**, **jni** - contains makefiles for android
* **android-project** - Android Studio project
* **Core** - eqCore, system module including vfs, console command processor, logging, platform stuff like CPU detection
* **MaterialSystem** - the material system base of EQ Engine
* MaterialSystem/**Renderers** - RHI API wrapper implementations of Direct3D9, Direct3D10, OpenGL (desktop and ES 2.0)
* MaterialSystem/**EngineShaders** - shader stages/pipeline basic library
* **public** - public headers and sources
* public/**math** - the vector math library
* **shared_engine** - engine shared sources between different games / engine modules
* **shared_game** - shared game sources
* **utils** - various SDK-related utilities. Most notable are **egfCa**, **animCa**, **egfMan**, **fcompress**
* **wxForms** - various wxFormBuilder fbp files for DriversEditor, Editor projects

Games:

* **DriversGame** - Driver Syndicate engine and game sources
* **DriversEditor** - wxWidgets-based editor for Driver Syndicate

* **Engine** - Legacy Equilibrium Engine, the massive and undone part
* **Editor** - wxWidgets-based Equilibrium World Editor
* **Game** - Equilibrium Engine Game source files, undone part

Dependencies
-------------

Before you begin, you need to download dependencies and extract them to **src_dependency/** folder, located in project root.

List of dependencies:

* **luajit** - use latest LuaJIT 2.0, and just make.
* **oolua** - needs to be configured first with OOLUA_RUNTIME_CHECKS_ENABLED=0 and "premake4 --class_functions=30 oolua-gen", then build.
* **openal-soft** - download tarball from http://kcat.strangesoft.net/openal.html
* **Shiny** - Shiny C++ profiler. Should be taken from https://sourceforge.net/projects/shinyprofiler/ , please download only snapshot.
* **bullet** - Bullet Physics Library 2.83, you may use latest release

Mobile platforms dependencies are:

* **OpenAL-MOB** - used instead of just openal-soft - take it from https://github.com/Jawbone/OpenAL-MOB
* **SDL2** - optionally can be configured in SDL_config_android.h with SDL_VIDEO_RENDER_OGL_ES=0 and SDL_VIDEO_RENDER_OGL_ES2=0
* **ogg** and **vorbis**
* **libjpeg** (jpeg8)
* **libpng**

Also dependencies which not located in src_dependency are:

* **gtk+-2.0** dev lib