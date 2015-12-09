# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../Engine/Imaging" -I"../../FileSystem" -I"../../core" -I"../../public" -I"../../public/core" -I"../../public/Platform" -I"../../Engine" -I"../../src_dependency/sdl2/include" 
Release_Include_Path=-I"../../Engine/Imaging" -I"../../FileSystem" -I"../../core" -I"../../public" -I"../../public/core" -I"../../public/Platform" -I"../../Engine" -I"../../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-O2 
Release_Compiler_Flags=-O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../../public/Utils/RectanglePacker.o gccDebug/atlasgen.o gccDebug/AtlasImagePacker.o 
	g++ gccDebug/../../public/Utils/RectanglePacker.o gccDebug/atlasgen.o gccDebug/AtlasImagePacker.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/atlasgen.exe

# Compiles file ../../public/Utils/RectanglePacker.cpp for the Debug configuration...
-include gccDebug/../../public/Utils/RectanglePacker.d
gccDebug/../../public/Utils/RectanglePacker.o: ../../public/Utils/RectanglePacker.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/Utils/RectanglePacker.cpp $(Debug_Include_Path) -o gccDebug/../../public/Utils/RectanglePacker.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/Utils/RectanglePacker.cpp $(Debug_Include_Path) > gccDebug/../../public/Utils/RectanglePacker.d

# Compiles file atlasgen.cpp for the Debug configuration...
-include gccDebug/atlasgen.d
gccDebug/atlasgen.o: atlasgen.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c atlasgen.cpp $(Debug_Include_Path) -o gccDebug/atlasgen.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM atlasgen.cpp $(Debug_Include_Path) > gccDebug/atlasgen.d

# Compiles file AtlasImagePacker.cpp for the Debug configuration...
-include gccDebug/AtlasImagePacker.d
gccDebug/AtlasImagePacker.o: AtlasImagePacker.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AtlasImagePacker.cpp $(Debug_Include_Path) -o gccDebug/AtlasImagePacker.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AtlasImagePacker.cpp $(Debug_Include_Path) > gccDebug/AtlasImagePacker.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../public/Utils/RectanglePacker.o gccRelease/atlasgen.o gccRelease/AtlasImagePacker.o 
	g++ gccRelease/../../public/Utils/RectanglePacker.o gccRelease/atlasgen.o gccRelease/AtlasImagePacker.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/atlasgen.exe

# Compiles file ../../public/Utils/RectanglePacker.cpp for the Release configuration...
-include gccRelease/../../public/Utils/RectanglePacker.d
gccRelease/../../public/Utils/RectanglePacker.o: ../../public/Utils/RectanglePacker.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/Utils/RectanglePacker.cpp $(Release_Include_Path) -o gccRelease/../../public/Utils/RectanglePacker.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/Utils/RectanglePacker.cpp $(Release_Include_Path) > gccRelease/../../public/Utils/RectanglePacker.d

# Compiles file atlasgen.cpp for the Release configuration...
-include gccRelease/atlasgen.d
gccRelease/atlasgen.o: atlasgen.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c atlasgen.cpp $(Release_Include_Path) -o gccRelease/atlasgen.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM atlasgen.cpp $(Release_Include_Path) > gccRelease/atlasgen.d

# Compiles file AtlasImagePacker.cpp for the Release configuration...
-include gccRelease/AtlasImagePacker.d
gccRelease/AtlasImagePacker.o: AtlasImagePacker.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AtlasImagePacker.cpp $(Release_Include_Path) -o gccRelease/AtlasImagePacker.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AtlasImagePacker.cpp $(Release_Include_Path) > gccRelease/AtlasImagePacker.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug/source
	mkdir -p gccRelease/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug/*.o
	rm -f gccDebug/*.d
	rm -f gccDebug/*.a
	rm -f gccDebug/*.so
	rm -f gccDebug/*.dll
	rm -f gccDebug/*.exe
	rm -f gccRelease/*.o
	rm -f gccRelease/*.d
	rm -f gccRelease/*.a
	rm -f gccRelease/*.so
	rm -f gccRelease/*.dll
	rm -f gccRelease/*.exe

