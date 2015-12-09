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
Debug_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lgdi32 -lshell32 -lwinmm  -Wl,--end-group
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE -D _TOOLS 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE -D _TOOLS 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-Wall -O2 
Release_Compiler_Flags=-Wall -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../../public/dsm_esm_loader.o gccDebug/../../public/dsm_loader.o gccDebug/../../public/dsm_obj_loader.o gccDebug/AnimCa_main.o gccDebug/AnimScriptCompiler.o 
	g++ gccDebug/../../public/dsm_esm_loader.o gccDebug/../../public/dsm_loader.o gccDebug/../../public/dsm_obj_loader.o gccDebug/AnimCa_main.o gccDebug/AnimScriptCompiler.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/animca.exe

# Compiles file ../../public/dsm_esm_loader.cpp for the Debug configuration...
-include gccDebug/../../public/dsm_esm_loader.d
gccDebug/../../public/dsm_esm_loader.o: ../../public/dsm_esm_loader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/dsm_esm_loader.cpp $(Debug_Include_Path) -o gccDebug/../../public/dsm_esm_loader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/dsm_esm_loader.cpp $(Debug_Include_Path) > gccDebug/../../public/dsm_esm_loader.d

# Compiles file ../../public/dsm_loader.cpp for the Debug configuration...
-include gccDebug/../../public/dsm_loader.d
gccDebug/../../public/dsm_loader.o: ../../public/dsm_loader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/dsm_loader.cpp $(Debug_Include_Path) -o gccDebug/../../public/dsm_loader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/dsm_loader.cpp $(Debug_Include_Path) > gccDebug/../../public/dsm_loader.d

# Compiles file ../../public/dsm_obj_loader.cpp for the Debug configuration...
-include gccDebug/../../public/dsm_obj_loader.d
gccDebug/../../public/dsm_obj_loader.o: ../../public/dsm_obj_loader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/dsm_obj_loader.cpp $(Debug_Include_Path) -o gccDebug/../../public/dsm_obj_loader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/dsm_obj_loader.cpp $(Debug_Include_Path) > gccDebug/../../public/dsm_obj_loader.d

# Compiles file AnimCa_main.cpp for the Debug configuration...
-include gccDebug/AnimCa_main.d
gccDebug/AnimCa_main.o: AnimCa_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AnimCa_main.cpp $(Debug_Include_Path) -o gccDebug/AnimCa_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AnimCa_main.cpp $(Debug_Include_Path) > gccDebug/AnimCa_main.d

# Compiles file AnimScriptCompiler.cpp for the Debug configuration...
-include gccDebug/AnimScriptCompiler.d
gccDebug/AnimScriptCompiler.o: AnimScriptCompiler.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AnimScriptCompiler.cpp $(Debug_Include_Path) -o gccDebug/AnimScriptCompiler.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AnimScriptCompiler.cpp $(Debug_Include_Path) > gccDebug/AnimScriptCompiler.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../public/dsm_esm_loader.o gccRelease/../../public/dsm_loader.o gccRelease/../../public/dsm_obj_loader.o gccRelease/AnimCa_main.o gccRelease/AnimScriptCompiler.o 
	g++ gccRelease/../../public/dsm_esm_loader.o gccRelease/../../public/dsm_loader.o gccRelease/../../public/dsm_obj_loader.o gccRelease/AnimCa_main.o gccRelease/AnimScriptCompiler.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/animca.exe

# Compiles file ../../public/dsm_esm_loader.cpp for the Release configuration...
-include gccRelease/../../public/dsm_esm_loader.d
gccRelease/../../public/dsm_esm_loader.o: ../../public/dsm_esm_loader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/dsm_esm_loader.cpp $(Release_Include_Path) -o gccRelease/../../public/dsm_esm_loader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/dsm_esm_loader.cpp $(Release_Include_Path) > gccRelease/../../public/dsm_esm_loader.d

# Compiles file ../../public/dsm_loader.cpp for the Release configuration...
-include gccRelease/../../public/dsm_loader.d
gccRelease/../../public/dsm_loader.o: ../../public/dsm_loader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/dsm_loader.cpp $(Release_Include_Path) -o gccRelease/../../public/dsm_loader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/dsm_loader.cpp $(Release_Include_Path) > gccRelease/../../public/dsm_loader.d

# Compiles file ../../public/dsm_obj_loader.cpp for the Release configuration...
-include gccRelease/../../public/dsm_obj_loader.d
gccRelease/../../public/dsm_obj_loader.o: ../../public/dsm_obj_loader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/dsm_obj_loader.cpp $(Release_Include_Path) -o gccRelease/../../public/dsm_obj_loader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/dsm_obj_loader.cpp $(Release_Include_Path) > gccRelease/../../public/dsm_obj_loader.d

# Compiles file AnimCa_main.cpp for the Release configuration...
-include gccRelease/AnimCa_main.d
gccRelease/AnimCa_main.o: AnimCa_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AnimCa_main.cpp $(Release_Include_Path) -o gccRelease/AnimCa_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AnimCa_main.cpp $(Release_Include_Path) > gccRelease/AnimCa_main.d

# Compiles file AnimScriptCompiler.cpp for the Release configuration...
-include gccRelease/AnimScriptCompiler.d
gccRelease/AnimScriptCompiler.o: AnimScriptCompiler.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AnimScriptCompiler.cpp $(Release_Include_Path) -o gccRelease/AnimScriptCompiler.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AnimScriptCompiler.cpp $(Release_Include_Path) > gccRelease/AnimScriptCompiler.d

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

