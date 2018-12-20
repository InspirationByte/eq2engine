![eqengine_logo.jpg](https://bitbucket.org/repo/BzA5LG/images/960076965-eqengine_logo.jpg)

The game engine/framework developing for these titles (in chronological order):

As DarkTech Engine (2009-2010):

* Project Tokamak ** Revenge of Alan ** (project frozen)
* Project RoofManiac (project cancelled)

As Equilibrium Engine:

* Project Underdome **White Cage** (project frozen, will be likely developed on this engine)
* Project BastardPigeon (project frozen, probably will be continued on UE or Unity)
* Project Drivers **The Driver Syndicate** (active development)
* Other Secret driving game project (planned)

Folder overview
-------------

* **android_lib_mk**, **jni** - contains makefiles for android
* **android-project** - Android Studio project
* **Core** - eqCore, system module including vfs, console command processor, logging, platform stuff like CPU detection
* **MaterialSystem** - material system which derives concept of Source Engine material system
* MaterialSystem/**Renderers** - RHI API wrapper implementations over Direct3D9, Direct3D10, OpenGL (desktop and ES 3.0)
* MaterialSystem/**EngineShaders** - shader stages/pipeline basic library
* **public** - public headers and sources of most engine modules
* public/**math** - the vector math library
* **shared_engine** - shared engine modules between different applications and other modules (Audio, Network, UI system, Particle System, Model cache)
* **shared_game** - shared game sources (Animation system, helpers)
* **utils** - various SDK-related utilities. Most notable are **egfCa**, **animCa**, **egfMan**, **fcompress**
* **wxForms** - various wxFormBuilder fbp files for DriversEditor, Editor projects

Games:

* **DriversGame** - Driver Syndicate engine and game sources
* **DriversEditor** - wxWidgets-based editor for Driver Syndicate

* **Engine** - legacy Equilibrium Engine, the massive and undone part
* **WorldEdit** - legacy Equilibrium World Editor
* **Game** - Equilibrium Engine Game source files, undone part. Mostly a **White Cage** source code.
* utils/**EqWC** - world compiler for legacy engine. Performs subdivision, lightmapped surface breakdown, etc
* utils/**EqLC** - hardware lightmap rendering compiler.

Dependencies
-------------

Before you begin, you need to download dependencies and extract them to **src_dependency/** folder, located in project root.

List of dependencies:

* **lua** - use latest LuaJIT 2.0, and just make.
* **oolua** - needs to be configured first with "premake4 --class_functions=30 oolua-gen", optionally OOLUA_RUNTIME_CHECKS_ENABLED=0 and then build.
* **openal-soft** - download tarball from http://kcat.strangesoft.net/openal.html
* **Shiny** - Shiny C++ profiler. Should be taken from https://sourceforge.net/projects/shinyprofiler/ , please download only snapshot.
* **bullet** - Bullet Physics Library 2.83, you may use latest release. Windows build must use Multithreaded DLL option, otherwise it won't link
* **ogg** and **vorbis**
* **wx** - wxWidgets 3.1, which the all editors stands on.
* **SDL2** - For mobile development please configure in SDL_config_android.h with SDL_VIDEO_RENDER_OGL_ES=0 and SDL_VIDEO_RENDER_OGL_ES2=0

Mobile platforms dependencies are:
* **libjpeg** (jpeg8)
* **libpng**

Also dependencies which not located in src_dependency are:
* **gtk+-2.0** dev lib