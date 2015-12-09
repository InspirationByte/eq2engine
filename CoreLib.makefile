# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"public" -I"public/platform" -I"src_dependency/sdl2/include" 
Release_Include_Path=-I"public" -I"public/platform" -I"src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D _DEBUG -D GCC_BUILD 
Release_Preprocessor_Definitions=-D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D CROSSLINK_LIB -D GCC_BUILD 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-O0 
Release_Compiler_Flags=-O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/public/core/cmdlib.o gccDebug/public/core/cmd_pacifier.o gccDebug/public/core/ConCommand.o gccDebug/public/core/ConCommandBase.o gccDebug/public/core/ConVar.o 
	ar rcs gccDebug/libCoreLib.a gccDebug/public/core/cmdlib.o gccDebug/public/core/cmd_pacifier.o gccDebug/public/core/ConCommand.o gccDebug/public/core/ConCommandBase.o gccDebug/public/core/ConVar.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file public/core/cmdlib.cpp for the Debug configuration...
-include gccDebug/public/core/cmdlib.d
gccDebug/public/core/cmdlib.o: public/core/cmdlib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/core/cmdlib.cpp $(Debug_Include_Path) -o gccDebug/public/core/cmdlib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/core/cmdlib.cpp $(Debug_Include_Path) > gccDebug/public/core/cmdlib.d

# Compiles file public/core/cmd_pacifier.cpp for the Debug configuration...
-include gccDebug/public/core/cmd_pacifier.d
gccDebug/public/core/cmd_pacifier.o: public/core/cmd_pacifier.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/core/cmd_pacifier.cpp $(Debug_Include_Path) -o gccDebug/public/core/cmd_pacifier.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/core/cmd_pacifier.cpp $(Debug_Include_Path) > gccDebug/public/core/cmd_pacifier.d

# Compiles file public/core/ConCommand.cpp for the Debug configuration...
-include gccDebug/public/core/ConCommand.d
gccDebug/public/core/ConCommand.o: public/core/ConCommand.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/core/ConCommand.cpp $(Debug_Include_Path) -o gccDebug/public/core/ConCommand.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/core/ConCommand.cpp $(Debug_Include_Path) > gccDebug/public/core/ConCommand.d

# Compiles file public/core/ConCommandBase.cpp for the Debug configuration...
-include gccDebug/public/core/ConCommandBase.d
gccDebug/public/core/ConCommandBase.o: public/core/ConCommandBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/core/ConCommandBase.cpp $(Debug_Include_Path) -o gccDebug/public/core/ConCommandBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/core/ConCommandBase.cpp $(Debug_Include_Path) > gccDebug/public/core/ConCommandBase.d

# Compiles file public/core/ConVar.cpp for the Debug configuration...
-include gccDebug/public/core/ConVar.d
gccDebug/public/core/ConVar.o: public/core/ConVar.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c public/core/ConVar.cpp $(Debug_Include_Path) -o gccDebug/public/core/ConVar.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM public/core/ConVar.cpp $(Debug_Include_Path) > gccDebug/public/core/ConVar.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/public/core/cmdlib.o gccRelease/public/core/cmd_pacifier.o gccRelease/public/core/ConCommand.o gccRelease/public/core/ConCommandBase.o gccRelease/public/core/ConVar.o 
	ar rcs gccRelease/libCoreLib.a gccRelease/public/core/cmdlib.o gccRelease/public/core/cmd_pacifier.o gccRelease/public/core/ConCommand.o gccRelease/public/core/ConCommandBase.o gccRelease/public/core/ConVar.o  $(Release_Implicitly_Linked_Objects)

# Compiles file public/core/cmdlib.cpp for the Release configuration...
-include gccRelease/public/core/cmdlib.d
gccRelease/public/core/cmdlib.o: public/core/cmdlib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/core/cmdlib.cpp $(Release_Include_Path) -o gccRelease/public/core/cmdlib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/core/cmdlib.cpp $(Release_Include_Path) > gccRelease/public/core/cmdlib.d

# Compiles file public/core/cmd_pacifier.cpp for the Release configuration...
-include gccRelease/public/core/cmd_pacifier.d
gccRelease/public/core/cmd_pacifier.o: public/core/cmd_pacifier.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/core/cmd_pacifier.cpp $(Release_Include_Path) -o gccRelease/public/core/cmd_pacifier.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/core/cmd_pacifier.cpp $(Release_Include_Path) > gccRelease/public/core/cmd_pacifier.d

# Compiles file public/core/ConCommand.cpp for the Release configuration...
-include gccRelease/public/core/ConCommand.d
gccRelease/public/core/ConCommand.o: public/core/ConCommand.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/core/ConCommand.cpp $(Release_Include_Path) -o gccRelease/public/core/ConCommand.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/core/ConCommand.cpp $(Release_Include_Path) > gccRelease/public/core/ConCommand.d

# Compiles file public/core/ConCommandBase.cpp for the Release configuration...
-include gccRelease/public/core/ConCommandBase.d
gccRelease/public/core/ConCommandBase.o: public/core/ConCommandBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/core/ConCommandBase.cpp $(Release_Include_Path) -o gccRelease/public/core/ConCommandBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/core/ConCommandBase.cpp $(Release_Include_Path) > gccRelease/public/core/ConCommandBase.d

# Compiles file public/core/ConVar.cpp for the Release configuration...
-include gccRelease/public/core/ConVar.d
gccRelease/public/core/ConVar.o: public/core/ConVar.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c public/core/ConVar.cpp $(Release_Include_Path) -o gccRelease/public/core/ConVar.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM public/core/ConVar.cpp $(Release_Include_Path) > gccRelease/public/core/ConVar.d

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

