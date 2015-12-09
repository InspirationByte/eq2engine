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
Debug_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lshell32 -lwinmm  -Wl,--end-group
Release_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lshell32 -lwinmm  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE -D _CRT_SECURE_NO_DEPRECATE -D _TOOLS -D SUPRESS_DEVMESSAGES 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE -D _CRT_SECURE_NO_DEPRECATE -D _TOOLS -D SUPRESS_DEVMESSAGES 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-O3 
Release_Compiler_Flags=-O3 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../../FileSystem/DPKFileReader.o gccDebug/DPKFileWriter.o gccDebug/fcompress_main.o gccDebug/../../public/IceKey.o gccDebug/SHAFWriter.o 
	g++ gccDebug/../../FileSystem/DPKFileReader.o gccDebug/DPKFileWriter.o gccDebug/fcompress_main.o gccDebug/../../public/IceKey.o gccDebug/SHAFWriter.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/fcompress.exe

# Compiles file ../../FileSystem/DPKFileReader.cpp for the Debug configuration...
-include gccDebug/../../FileSystem/DPKFileReader.d
gccDebug/../../FileSystem/DPKFileReader.o: ../../FileSystem/DPKFileReader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../FileSystem/DPKFileReader.cpp $(Debug_Include_Path) -o gccDebug/../../FileSystem/DPKFileReader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../FileSystem/DPKFileReader.cpp $(Debug_Include_Path) > gccDebug/../../FileSystem/DPKFileReader.d

# Compiles file DPKFileWriter.cpp for the Debug configuration...
-include gccDebug/DPKFileWriter.d
gccDebug/DPKFileWriter.o: DPKFileWriter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DPKFileWriter.cpp $(Debug_Include_Path) -o gccDebug/DPKFileWriter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DPKFileWriter.cpp $(Debug_Include_Path) > gccDebug/DPKFileWriter.d

# Compiles file fcompress_main.cpp for the Debug configuration...
-include gccDebug/fcompress_main.d
gccDebug/fcompress_main.o: fcompress_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c fcompress_main.cpp $(Debug_Include_Path) -o gccDebug/fcompress_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM fcompress_main.cpp $(Debug_Include_Path) > gccDebug/fcompress_main.d

# Compiles file ../../public/IceKey.cpp for the Debug configuration...
-include gccDebug/../../public/IceKey.d
gccDebug/../../public/IceKey.o: ../../public/IceKey.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/IceKey.cpp $(Debug_Include_Path) -o gccDebug/../../public/IceKey.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/IceKey.cpp $(Debug_Include_Path) > gccDebug/../../public/IceKey.d

# Compiles file SHAFWriter.cpp for the Debug configuration...
-include gccDebug/SHAFWriter.d
gccDebug/SHAFWriter.o: SHAFWriter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SHAFWriter.cpp $(Debug_Include_Path) -o gccDebug/SHAFWriter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SHAFWriter.cpp $(Debug_Include_Path) > gccDebug/SHAFWriter.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../FileSystem/DPKFileReader.o gccRelease/DPKFileWriter.o gccRelease/fcompress_main.o gccRelease/../../public/IceKey.o gccRelease/SHAFWriter.o 
	g++ gccRelease/../../FileSystem/DPKFileReader.o gccRelease/DPKFileWriter.o gccRelease/fcompress_main.o gccRelease/../../public/IceKey.o gccRelease/SHAFWriter.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/fcompress.exe

# Compiles file ../../FileSystem/DPKFileReader.cpp for the Release configuration...
-include gccRelease/../../FileSystem/DPKFileReader.d
gccRelease/../../FileSystem/DPKFileReader.o: ../../FileSystem/DPKFileReader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../FileSystem/DPKFileReader.cpp $(Release_Include_Path) -o gccRelease/../../FileSystem/DPKFileReader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../FileSystem/DPKFileReader.cpp $(Release_Include_Path) > gccRelease/../../FileSystem/DPKFileReader.d

# Compiles file DPKFileWriter.cpp for the Release configuration...
-include gccRelease/DPKFileWriter.d
gccRelease/DPKFileWriter.o: DPKFileWriter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DPKFileWriter.cpp $(Release_Include_Path) -o gccRelease/DPKFileWriter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DPKFileWriter.cpp $(Release_Include_Path) > gccRelease/DPKFileWriter.d

# Compiles file fcompress_main.cpp for the Release configuration...
-include gccRelease/fcompress_main.d
gccRelease/fcompress_main.o: fcompress_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c fcompress_main.cpp $(Release_Include_Path) -o gccRelease/fcompress_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM fcompress_main.cpp $(Release_Include_Path) > gccRelease/fcompress_main.d

# Compiles file ../../public/IceKey.cpp for the Release configuration...
-include gccRelease/../../public/IceKey.d
gccRelease/../../public/IceKey.o: ../../public/IceKey.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/IceKey.cpp $(Release_Include_Path) -o gccRelease/../../public/IceKey.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/IceKey.cpp $(Release_Include_Path) > gccRelease/../../public/IceKey.d

# Compiles file SHAFWriter.cpp for the Release configuration...
-include gccRelease/SHAFWriter.d
gccRelease/SHAFWriter.o: SHAFWriter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SHAFWriter.cpp $(Release_Include_Path) -o gccRelease/SHAFWriter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SHAFWriter.cpp $(Release_Include_Path) > gccRelease/SHAFWriter.d

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

