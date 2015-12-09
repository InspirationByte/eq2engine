# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../src_dependency/sdl2/include" 
Long_Include_Path=-I"FileSystem" -I"Engine" -I"public" -I"public/platform" 
PigeonGameRelease_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/platform" 
Release_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Long_Library_Path=
PigeonGameRelease_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=
Long_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lgdi32 -lshell32 -lwinmm -lopengl32  -Wl,--end-group
PigeonGameRelease_Libraries=
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _CRT_SECURE_NO_DEPRECATE -D _DKLAUNCHER_ -D GAME_DEMO -D _CRT_SECURE_NO_WARNINGS 
Long_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D USE_OCC_QUERY_WORKAROUND -D GENERAL_ENGINE_LIBRARY -D _CRT_SECURE_NO_DEPRECATE -D _DKLAUNCHER_ -D LONG_LAUNCH 
PigeonGameRelease_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _CRT_SECURE_NO_DEPRECATE -D _DKLAUNCHER_ -D GAME_DEMO -D _CRT_SECURE_NO_WARNINGS -D PIGEONGAME 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _CRT_SECURE_NO_DEPRECATE -D _DKLAUNCHER_ -D GAME_DEMO -D _CRT_SECURE_NO_WARNINGS 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Long_Implicitly_Linked_Objects=
PigeonGameRelease_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-Wall -O0 -g 
Long_Compiler_Flags=-Wall -O2 
PigeonGameRelease_Compiler_Flags=-Wall -O2 
Release_Compiler_Flags=-Wall -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Long PigeonGameRelease Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../public/Platform/Platform.o gccDebug/launcher_win.o 
	g++ gccDebug/../public/Platform/Platform.o gccDebug/launcher_win.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/EqLauncher.exe

# Compiles file ../public/Platform/Platform.cpp for the Debug configuration...
-include gccDebug/../public/Platform/Platform.d
gccDebug/../public/Platform/Platform.o: ../public/Platform/Platform.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Platform/Platform.cpp $(Debug_Include_Path) -o gccDebug/../public/Platform/Platform.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Platform/Platform.cpp $(Debug_Include_Path) > gccDebug/../public/Platform/Platform.d

# Compiles file launcher_win.cpp for the Debug configuration...
-include gccDebug/launcher_win.d
gccDebug/launcher_win.o: launcher_win.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c launcher_win.cpp $(Debug_Include_Path) -o gccDebug/launcher_win.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM launcher_win.cpp $(Debug_Include_Path) > gccDebug/launcher_win.d

# Builds the Long configuration...
.PHONY: Long
Long: create_folders gccLong/../public/Platform/Platform.o gccLong/launcher_win.o 
	g++ gccLong/../public/Platform/Platform.o gccLong/launcher_win.o  $(Long_Library_Path) $(Long_Libraries) -Wl,-rpath,./ -o gccLong/EqLauncher.exe

# Compiles file ../public/Platform/Platform.cpp for the Long configuration...
-include gccLong/../public/Platform/Platform.d
gccLong/../public/Platform/Platform.o: ../public/Platform/Platform.cpp
	$(CPP_COMPILER) $(Long_Preprocessor_Definitions) $(Long_Compiler_Flags) -c ../public/Platform/Platform.cpp $(Long_Include_Path) -o gccLong/../public/Platform/Platform.o
	$(CPP_COMPILER) $(Long_Preprocessor_Definitions) $(Long_Compiler_Flags) -MM ../public/Platform/Platform.cpp $(Long_Include_Path) > gccLong/../public/Platform/Platform.d

# Compiles file launcher_win.cpp for the Long configuration...
-include gccLong/launcher_win.d
gccLong/launcher_win.o: launcher_win.cpp
	$(CPP_COMPILER) $(Long_Preprocessor_Definitions) $(Long_Compiler_Flags) -c launcher_win.cpp $(Long_Include_Path) -o gccLong/launcher_win.o
	$(CPP_COMPILER) $(Long_Preprocessor_Definitions) $(Long_Compiler_Flags) -MM launcher_win.cpp $(Long_Include_Path) > gccLong/launcher_win.d

# Builds the PigeonGameRelease configuration...
.PHONY: PigeonGameRelease
PigeonGameRelease: create_folders ./Compile/Launcher/gccRelease/../public/Platform/Platform.o ./Compile/Launcher/gccRelease/launcher_win.o 
	g++ ./Compile/Launcher/gccRelease/../public/Platform/Platform.o ./Compile/Launcher/gccRelease/launcher_win.o  $(PigeonGameRelease_Library_Path) $(PigeonGameRelease_Libraries) -Wl,-rpath,./ -o ./Compile/Launcher/gccRelease/EqLauncher.exe

# Compiles file ../public/Platform/Platform.cpp for the PigeonGameRelease configuration...
-include ./Compile/Launcher/gccRelease/../public/Platform/Platform.d
./Compile/Launcher/gccRelease/../public/Platform/Platform.o: ../public/Platform/Platform.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/Platform/Platform.cpp $(PigeonGameRelease_Include_Path) -o ./Compile/Launcher/gccRelease/../public/Platform/Platform.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/Platform/Platform.cpp $(PigeonGameRelease_Include_Path) > ./Compile/Launcher/gccRelease/../public/Platform/Platform.d

# Compiles file launcher_win.cpp for the PigeonGameRelease configuration...
-include ./Compile/Launcher/gccRelease/launcher_win.d
./Compile/Launcher/gccRelease/launcher_win.o: launcher_win.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c launcher_win.cpp $(PigeonGameRelease_Include_Path) -o ./Compile/Launcher/gccRelease/launcher_win.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM launcher_win.cpp $(PigeonGameRelease_Include_Path) > ./Compile/Launcher/gccRelease/launcher_win.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../public/Platform/Platform.o gccRelease/launcher_win.o 
	g++ gccRelease/../public/Platform/Platform.o gccRelease/launcher_win.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/EqLauncher.exe

# Compiles file ../public/Platform/Platform.cpp for the Release configuration...
-include gccRelease/../public/Platform/Platform.d
gccRelease/../public/Platform/Platform.o: ../public/Platform/Platform.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Platform/Platform.cpp $(Release_Include_Path) -o gccRelease/../public/Platform/Platform.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Platform/Platform.cpp $(Release_Include_Path) > gccRelease/../public/Platform/Platform.d

# Compiles file launcher_win.cpp for the Release configuration...
-include gccRelease/launcher_win.d
gccRelease/launcher_win.o: launcher_win.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c launcher_win.cpp $(Release_Include_Path) -o gccRelease/launcher_win.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM launcher_win.cpp $(Release_Include_Path) > gccRelease/launcher_win.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug/source
	mkdir -p gccLong/source
	mkdir -p ./Compile/Launcher/gccRelease/source
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
	rm -f gccLong/*.o
	rm -f gccLong/*.d
	rm -f gccLong/*.a
	rm -f gccLong/*.so
	rm -f gccLong/*.dll
	rm -f gccLong/*.exe
	rm -f ./Compile/Launcher/gccRelease/*.o
	rm -f ./Compile/Launcher/gccRelease/*.d
	rm -f ./Compile/Launcher/gccRelease/*.a
	rm -f ./Compile/Launcher/gccRelease/*.so
	rm -f ./Compile/Launcher/gccRelease/*.dll
	rm -f ./Compile/Launcher/gccRelease/*.exe
	rm -f gccRelease/*.o
	rm -f gccRelease/*.d
	rm -f gccRelease/*.a
	rm -f gccRelease/*.so
	rm -f gccRelease/*.dll
	rm -f gccRelease/*.exe

