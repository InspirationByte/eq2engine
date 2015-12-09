# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../Engine/Imaging" -I"../../FileSystem" -I"../../core" -I"../../public" -I"../../public/core" -I"../../public/Platform" -I"../../Engine" -I"../../src_dependency/Bullet/src" -I"../../src_dependency/sdl2/include" 
Release_Include_Path=-I"../../Engine/Imaging" -I"../../FileSystem" -I"../../core" -I"../../public" -I"../../public/core" -I"../../public/Platform" -I"../../Engine" -I"../../src_dependency/Bullet/src" -I"../../src_dependency/sdl2/include" 

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
Debug: create_folders gccDebug/../../public/dsm_esm_loader.o gccDebug/../../public/dsm_loader.o gccDebug/../../public/dsm_obj_loader.o gccDebug/scriptcompile.o gccDebug/egfca_main.o gccDebug/physgen_main.o gccDebug/physgen_process.o gccDebug/write.o gccDebug/../actc/tc.o gccDebug/../../public/mtriangle_framework.o gccDebug/../../public/Utils/GeomTools.o 
	g++ gccDebug/../../public/dsm_esm_loader.o gccDebug/../../public/dsm_loader.o gccDebug/../../public/dsm_obj_loader.o gccDebug/scriptcompile.o gccDebug/egfca_main.o gccDebug/physgen_main.o gccDebug/physgen_process.o gccDebug/write.o gccDebug/../actc/tc.o gccDebug/../../public/mtriangle_framework.o gccDebug/../../public/Utils/GeomTools.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/egfca.exe

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

# Compiles file scriptcompile.cpp for the Debug configuration...
-include gccDebug/scriptcompile.d
gccDebug/scriptcompile.o: scriptcompile.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c scriptcompile.cpp $(Debug_Include_Path) -o gccDebug/scriptcompile.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM scriptcompile.cpp $(Debug_Include_Path) > gccDebug/scriptcompile.d

# Compiles file egfca_main.cpp for the Debug configuration...
-include gccDebug/egfca_main.d
gccDebug/egfca_main.o: egfca_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c egfca_main.cpp $(Debug_Include_Path) -o gccDebug/egfca_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM egfca_main.cpp $(Debug_Include_Path) > gccDebug/egfca_main.d

# Compiles file physgen_main.cpp for the Debug configuration...
-include gccDebug/physgen_main.d
gccDebug/physgen_main.o: physgen_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c physgen_main.cpp $(Debug_Include_Path) -o gccDebug/physgen_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM physgen_main.cpp $(Debug_Include_Path) > gccDebug/physgen_main.d

# Compiles file physgen_process.cpp for the Debug configuration...
-include gccDebug/physgen_process.d
gccDebug/physgen_process.o: physgen_process.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c physgen_process.cpp $(Debug_Include_Path) -o gccDebug/physgen_process.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM physgen_process.cpp $(Debug_Include_Path) > gccDebug/physgen_process.d

# Compiles file write.cpp for the Debug configuration...
-include gccDebug/write.d
gccDebug/write.o: write.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c write.cpp $(Debug_Include_Path) -o gccDebug/write.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM write.cpp $(Debug_Include_Path) > gccDebug/write.d

# Compiles file ../actc/tc.c for the Debug configuration...
-include gccDebug/../actc/tc.d
gccDebug/../actc/tc.o: ../actc/tc.c
	$(C_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../actc/tc.c $(Debug_Include_Path) -o gccDebug/../actc/tc.o
	$(C_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../actc/tc.c $(Debug_Include_Path) > gccDebug/../actc/tc.d

# Compiles file ../../public/mtriangle_framework.cpp for the Debug configuration...
-include gccDebug/../../public/mtriangle_framework.d
gccDebug/../../public/mtriangle_framework.o: ../../public/mtriangle_framework.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/mtriangle_framework.cpp $(Debug_Include_Path) -o gccDebug/../../public/mtriangle_framework.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/mtriangle_framework.cpp $(Debug_Include_Path) > gccDebug/../../public/mtriangle_framework.d

# Compiles file ../../public/Utils/GeomTools.cpp for the Debug configuration...
-include gccDebug/../../public/Utils/GeomTools.d
gccDebug/../../public/Utils/GeomTools.o: ../../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/Utils/GeomTools.cpp $(Debug_Include_Path) -o gccDebug/../../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/Utils/GeomTools.cpp $(Debug_Include_Path) > gccDebug/../../public/Utils/GeomTools.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../public/dsm_esm_loader.o gccRelease/../../public/dsm_loader.o gccRelease/../../public/dsm_obj_loader.o gccRelease/scriptcompile.o gccRelease/egfca_main.o gccRelease/physgen_main.o gccRelease/physgen_process.o gccRelease/write.o gccRelease/../actc/tc.o gccRelease/../../public/mtriangle_framework.o gccRelease/../../public/Utils/GeomTools.o 
	g++ gccRelease/../../public/dsm_esm_loader.o gccRelease/../../public/dsm_loader.o gccRelease/../../public/dsm_obj_loader.o gccRelease/scriptcompile.o gccRelease/egfca_main.o gccRelease/physgen_main.o gccRelease/physgen_process.o gccRelease/write.o gccRelease/../actc/tc.o gccRelease/../../public/mtriangle_framework.o gccRelease/../../public/Utils/GeomTools.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/egfca.exe

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

# Compiles file scriptcompile.cpp for the Release configuration...
-include gccRelease/scriptcompile.d
gccRelease/scriptcompile.o: scriptcompile.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c scriptcompile.cpp $(Release_Include_Path) -o gccRelease/scriptcompile.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM scriptcompile.cpp $(Release_Include_Path) > gccRelease/scriptcompile.d

# Compiles file egfca_main.cpp for the Release configuration...
-include gccRelease/egfca_main.d
gccRelease/egfca_main.o: egfca_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c egfca_main.cpp $(Release_Include_Path) -o gccRelease/egfca_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM egfca_main.cpp $(Release_Include_Path) > gccRelease/egfca_main.d

# Compiles file physgen_main.cpp for the Release configuration...
-include gccRelease/physgen_main.d
gccRelease/physgen_main.o: physgen_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c physgen_main.cpp $(Release_Include_Path) -o gccRelease/physgen_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM physgen_main.cpp $(Release_Include_Path) > gccRelease/physgen_main.d

# Compiles file physgen_process.cpp for the Release configuration...
-include gccRelease/physgen_process.d
gccRelease/physgen_process.o: physgen_process.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c physgen_process.cpp $(Release_Include_Path) -o gccRelease/physgen_process.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM physgen_process.cpp $(Release_Include_Path) > gccRelease/physgen_process.d

# Compiles file write.cpp for the Release configuration...
-include gccRelease/write.d
gccRelease/write.o: write.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c write.cpp $(Release_Include_Path) -o gccRelease/write.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM write.cpp $(Release_Include_Path) > gccRelease/write.d

# Compiles file ../actc/tc.c for the Release configuration...
-include gccRelease/../actc/tc.d
gccRelease/../actc/tc.o: ../actc/tc.c
	$(C_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../actc/tc.c $(Release_Include_Path) -o gccRelease/../actc/tc.o
	$(C_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../actc/tc.c $(Release_Include_Path) > gccRelease/../actc/tc.d

# Compiles file ../../public/mtriangle_framework.cpp for the Release configuration...
-include gccRelease/../../public/mtriangle_framework.d
gccRelease/../../public/mtriangle_framework.o: ../../public/mtriangle_framework.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/mtriangle_framework.cpp $(Release_Include_Path) -o gccRelease/../../public/mtriangle_framework.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/mtriangle_framework.cpp $(Release_Include_Path) > gccRelease/../../public/mtriangle_framework.d

# Compiles file ../../public/Utils/GeomTools.cpp for the Release configuration...
-include gccRelease/../../public/Utils/GeomTools.d
gccRelease/../../public/Utils/GeomTools.o: ../../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/Utils/GeomTools.cpp $(Release_Include_Path) -o gccRelease/../../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/Utils/GeomTools.cpp $(Release_Include_Path) > gccRelease/../../public/Utils/GeomTools.d

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

