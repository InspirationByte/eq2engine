# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../Engine/Imaging" -I"../../FileSystem" -I"../../core" -I"../../public/Platform" -I"../../public" -I"../../Engine" 
Release_Include_Path=-I"../../Engine/Imaging" -I"../../FileSystem" -I"../../core" -I"../../public/Platform" -I"../../public" -I"../../Engine" -I"../../src_dependency/sdl2/include" 

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
Debug: create_folders gccDebug/../../public/ConCommand.o gccDebug/../../public/ConCommandBase.o gccDebug/../../public/ConVar.o gccDebug/../../public/KeyValuesA.o gccDebug/convert.o gccDebug/kvconv.o 
	g++ gccDebug/../../public/ConCommand.o gccDebug/../../public/ConCommandBase.o gccDebug/../../public/ConVar.o gccDebug/../../public/KeyValuesA.o gccDebug/convert.o gccDebug/kvconv.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/kvconv.exe

# Compiles file ../../public/ConCommand.cpp for the Debug configuration...
-include gccDebug/../../public/ConCommand.d
gccDebug/../../public/ConCommand.o: ../../public/ConCommand.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ConCommand.cpp $(Debug_Include_Path) -o gccDebug/../../public/ConCommand.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ConCommand.cpp $(Debug_Include_Path) > gccDebug/../../public/ConCommand.d

# Compiles file ../../public/ConCommandBase.cpp for the Debug configuration...
-include gccDebug/../../public/ConCommandBase.d
gccDebug/../../public/ConCommandBase.o: ../../public/ConCommandBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ConCommandBase.cpp $(Debug_Include_Path) -o gccDebug/../../public/ConCommandBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ConCommandBase.cpp $(Debug_Include_Path) > gccDebug/../../public/ConCommandBase.d

# Compiles file ../../public/ConVar.cpp for the Debug configuration...
-include gccDebug/../../public/ConVar.d
gccDebug/../../public/ConVar.o: ../../public/ConVar.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ConVar.cpp $(Debug_Include_Path) -o gccDebug/../../public/ConVar.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ConVar.cpp $(Debug_Include_Path) > gccDebug/../../public/ConVar.d

# Compiles file ../../public/KeyValuesA.cpp for the Debug configuration...
-include gccDebug/../../public/KeyValuesA.d
gccDebug/../../public/KeyValuesA.o: ../../public/KeyValuesA.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/KeyValuesA.cpp $(Debug_Include_Path) -o gccDebug/../../public/KeyValuesA.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/KeyValuesA.cpp $(Debug_Include_Path) > gccDebug/../../public/KeyValuesA.d

# Compiles file convert.cpp for the Debug configuration...
-include gccDebug/convert.d
gccDebug/convert.o: convert.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c convert.cpp $(Debug_Include_Path) -o gccDebug/convert.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM convert.cpp $(Debug_Include_Path) > gccDebug/convert.d

# Compiles file kvconv.cpp for the Debug configuration...
-include gccDebug/kvconv.d
gccDebug/kvconv.o: kvconv.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c kvconv.cpp $(Debug_Include_Path) -o gccDebug/kvconv.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM kvconv.cpp $(Debug_Include_Path) > gccDebug/kvconv.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../public/ConCommand.o gccRelease/../../public/ConCommandBase.o gccRelease/../../public/ConVar.o gccRelease/../../public/KeyValuesA.o gccRelease/convert.o gccRelease/kvconv.o 
	g++ gccRelease/../../public/ConCommand.o gccRelease/../../public/ConCommandBase.o gccRelease/../../public/ConVar.o gccRelease/../../public/KeyValuesA.o gccRelease/convert.o gccRelease/kvconv.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/kvconv.exe

# Compiles file ../../public/ConCommand.cpp for the Release configuration...
-include gccRelease/../../public/ConCommand.d
gccRelease/../../public/ConCommand.o: ../../public/ConCommand.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ConCommand.cpp $(Release_Include_Path) -o gccRelease/../../public/ConCommand.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ConCommand.cpp $(Release_Include_Path) > gccRelease/../../public/ConCommand.d

# Compiles file ../../public/ConCommandBase.cpp for the Release configuration...
-include gccRelease/../../public/ConCommandBase.d
gccRelease/../../public/ConCommandBase.o: ../../public/ConCommandBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ConCommandBase.cpp $(Release_Include_Path) -o gccRelease/../../public/ConCommandBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ConCommandBase.cpp $(Release_Include_Path) > gccRelease/../../public/ConCommandBase.d

# Compiles file ../../public/ConVar.cpp for the Release configuration...
-include gccRelease/../../public/ConVar.d
gccRelease/../../public/ConVar.o: ../../public/ConVar.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ConVar.cpp $(Release_Include_Path) -o gccRelease/../../public/ConVar.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ConVar.cpp $(Release_Include_Path) > gccRelease/../../public/ConVar.d

# Compiles file ../../public/KeyValuesA.cpp for the Release configuration...
-include gccRelease/../../public/KeyValuesA.d
gccRelease/../../public/KeyValuesA.o: ../../public/KeyValuesA.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/KeyValuesA.cpp $(Release_Include_Path) -o gccRelease/../../public/KeyValuesA.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/KeyValuesA.cpp $(Release_Include_Path) > gccRelease/../../public/KeyValuesA.d

# Compiles file convert.cpp for the Release configuration...
-include gccRelease/convert.d
gccRelease/convert.o: convert.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c convert.cpp $(Release_Include_Path) -o gccRelease/convert.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM convert.cpp $(Release_Include_Path) > gccRelease/convert.d

# Compiles file kvconv.cpp for the Release configuration...
-include gccRelease/kvconv.d
gccRelease/kvconv.o: kvconv.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c kvconv.cpp $(Release_Include_Path) -o gccRelease/kvconv.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM kvconv.cpp $(Release_Include_Path) > gccRelease/kvconv.d

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

