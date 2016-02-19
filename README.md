# Equilibrium Engine #

The complicated game engine/framework originally developed for White Cage
Now used for Driver Syndicate

### Compiling ###

Dependencies
-------------

Before you begin, you should create folder "src_dependency" on root

List of dependencies:

*luajit - use latest LuaJIT, and just make.
*oolua - needs to be configured first with "premake4 --class_functions=30 oolua-gen", then build.
*openal-soft - download tarball from http://kcat.strangesoft.net/openal.html
*Shiny - Shiny C++ profiler. Should be taken from https://sourceforge.net/projects/shinyprofiler/ , please download only snapshot
*bullet - Bullet Physics Library 2.83

Also dependencies which not located in src_dependency are:
*jpeg8
*ogg, vorbis, vorbisfile
*gtk+-2.0
*SDL2