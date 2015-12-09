# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../public/materialsystem/" -I"../public/materialsystem/renderers" -I"../src_dependency/sdl2/include" 
Release_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../public/materialsystem/" -I"../public/materialsystem/renderers" -I"../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lgdi32 -lshell32 -lwinmm -lcomctl32  -Wl,--end-group
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D USE_SIMD -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D USE_SIMD -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-fPIC -O0 -g 
Release_Compiler_Flags=-fPIC -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/CMatProxyFactory.o gccDebug/material.o gccDebug/MaterialSystem.o gccDebug/materialvar.o 
	g++ -fPIC -shared -Wl,-soname,libEqMatSystem.so -o gccDebug/libEqMatSystem.so gccDebug/CMatProxyFactory.o gccDebug/material.o gccDebug/MaterialSystem.o gccDebug/materialvar.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file CMatProxyFactory.cpp for the Debug configuration...
-include gccDebug/CMatProxyFactory.d
gccDebug/CMatProxyFactory.o: CMatProxyFactory.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CMatProxyFactory.cpp $(Debug_Include_Path) -o gccDebug/CMatProxyFactory.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CMatProxyFactory.cpp $(Debug_Include_Path) > gccDebug/CMatProxyFactory.d

# Compiles file material.cpp for the Debug configuration...
-include gccDebug/material.d
gccDebug/material.o: material.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c material.cpp $(Debug_Include_Path) -o gccDebug/material.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM material.cpp $(Debug_Include_Path) > gccDebug/material.d

# Compiles file MaterialSystem.cpp for the Debug configuration...
-include gccDebug/MaterialSystem.d
gccDebug/MaterialSystem.o: MaterialSystem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c MaterialSystem.cpp $(Debug_Include_Path) -o gccDebug/MaterialSystem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM MaterialSystem.cpp $(Debug_Include_Path) > gccDebug/MaterialSystem.d

# Compiles file materialvar.cpp for the Debug configuration...
-include gccDebug/materialvar.d
gccDebug/materialvar.o: materialvar.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c materialvar.cpp $(Debug_Include_Path) -o gccDebug/materialvar.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM materialvar.cpp $(Debug_Include_Path) > gccDebug/materialvar.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/CMatProxyFactory.o gccRelease/material.o gccRelease/MaterialSystem.o gccRelease/materialvar.o 
	g++ -fPIC -shared -Wl,-soname,libEqMatSystem.so -o gccRelease/libEqMatSystem.so gccRelease/CMatProxyFactory.o gccRelease/material.o gccRelease/MaterialSystem.o gccRelease/materialvar.o  $(Release_Implicitly_Linked_Objects)

# Compiles file CMatProxyFactory.cpp for the Release configuration...
-include gccRelease/CMatProxyFactory.d
gccRelease/CMatProxyFactory.o: CMatProxyFactory.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CMatProxyFactory.cpp $(Release_Include_Path) -o gccRelease/CMatProxyFactory.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CMatProxyFactory.cpp $(Release_Include_Path) > gccRelease/CMatProxyFactory.d

# Compiles file material.cpp for the Release configuration...
-include gccRelease/material.d
gccRelease/material.o: material.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c material.cpp $(Release_Include_Path) -o gccRelease/material.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM material.cpp $(Release_Include_Path) > gccRelease/material.d

# Compiles file MaterialSystem.cpp for the Release configuration...
-include gccRelease/MaterialSystem.d
gccRelease/MaterialSystem.o: MaterialSystem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c MaterialSystem.cpp $(Release_Include_Path) -o gccRelease/MaterialSystem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM MaterialSystem.cpp $(Release_Include_Path) > gccRelease/MaterialSystem.d

# Compiles file materialvar.cpp for the Release configuration...
-include gccRelease/materialvar.d
gccRelease/materialvar.o: materialvar.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c materialvar.cpp $(Release_Include_Path) -o gccRelease/materialvar.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM materialvar.cpp $(Release_Include_Path) > gccRelease/materialvar.d

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

