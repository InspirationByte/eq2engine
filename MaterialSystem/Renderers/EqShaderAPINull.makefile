# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../public/materialsystem/renderers" -I"../../src_dependency/sdl2/include" 
Release_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../public/materialsystem/renderers" -I"../../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -luser32 -lgdi32  -Wl,--end-group
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D RENDER_EXPORT 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D RENDER_EXPORT 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-fPIC -Wall -O0 -g 
Release_Compiler_Flags=-fPIC -Wall -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug_empty/CTexture.o gccDebug_empty/ShaderAPI_Base.o gccDebug_empty/Empty/emptylibrary.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPINull.so -o gccDebug_empty/libEqShaderAPINull.so gccDebug_empty/CTexture.o gccDebug_empty/ShaderAPI_Base.o gccDebug_empty/Empty/emptylibrary.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the Debug configuration...
-include gccDebug_empty/CTexture.d
gccDebug_empty/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CTexture.cpp $(Debug_Include_Path) -o gccDebug_empty/CTexture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CTexture.cpp $(Debug_Include_Path) > gccDebug_empty/CTexture.d

# Compiles file ShaderAPI_Base.cpp for the Debug configuration...
-include gccDebug_empty/ShaderAPI_Base.d
gccDebug_empty/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ShaderAPI_Base.cpp $(Debug_Include_Path) -o gccDebug_empty/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Debug_Include_Path) > gccDebug_empty/ShaderAPI_Base.d

# Compiles file Empty/emptylibrary.cpp for the Debug configuration...
-include gccDebug_empty/Empty/emptylibrary.d
gccDebug_empty/Empty/emptylibrary.o: Empty/emptylibrary.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Empty/emptylibrary.cpp $(Debug_Include_Path) -o gccDebug_empty/Empty/emptylibrary.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Empty/emptylibrary.cpp $(Debug_Include_Path) > gccDebug_empty/Empty/emptylibrary.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease_empty/CTexture.o gccRelease_empty/ShaderAPI_Base.o gccRelease_empty/Empty/emptylibrary.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPINull.so -o gccRelease_empty/libEqShaderAPINull.so gccRelease_empty/CTexture.o gccRelease_empty/ShaderAPI_Base.o gccRelease_empty/Empty/emptylibrary.o  $(Release_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the Release configuration...
-include gccRelease_empty/CTexture.d
gccRelease_empty/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CTexture.cpp $(Release_Include_Path) -o gccRelease_empty/CTexture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CTexture.cpp $(Release_Include_Path) > gccRelease_empty/CTexture.d

# Compiles file ShaderAPI_Base.cpp for the Release configuration...
-include gccRelease_empty/ShaderAPI_Base.d
gccRelease_empty/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ShaderAPI_Base.cpp $(Release_Include_Path) -o gccRelease_empty/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Release_Include_Path) > gccRelease_empty/ShaderAPI_Base.d

# Compiles file Empty/emptylibrary.cpp for the Release configuration...
-include gccRelease_empty/Empty/emptylibrary.d
gccRelease_empty/Empty/emptylibrary.o: Empty/emptylibrary.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Empty/emptylibrary.cpp $(Release_Include_Path) -o gccRelease_empty/Empty/emptylibrary.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Empty/emptylibrary.cpp $(Release_Include_Path) > gccRelease_empty/Empty/emptylibrary.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug_empty/source
	mkdir -p gccRelease_empty/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug_empty/*.o
	rm -f gccDebug_empty/*.d
	rm -f gccDebug_empty/*.a
	rm -f gccDebug_empty/*.so
	rm -f gccDebug_empty/*.dll
	rm -f gccDebug_empty/*.exe
	rm -f gccRelease_empty/*.o
	rm -f gccRelease_empty/*.d
	rm -f gccRelease_empty/*.a
	rm -f gccRelease_empty/*.so
	rm -f gccRelease_empty/*.dll
	rm -f gccRelease_empty/*.exe

