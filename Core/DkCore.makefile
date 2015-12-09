# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../FileSystem" -I"../Engine/Imaging" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../src_dependency/sdl2/include" 
Release_Include_Path=-I"../FileSystem" -I"../Engine/Imaging" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -luser32 -ladvapi32 -ldbghelp  -Wl,--end-group
Release_Libraries=-Wl,--start-group -luser32 -ladvapi32 -ldbghelp  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D SUPRESS_DEVMESSAGES -D USECRTMEMDEBUG -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D SUPRESS_DEVMESSAGES -D USECRTMEMDEBUG -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 

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
Debug: create_folders gccDebug/CoreVersion.o gccDebug/CPU.o gccDebug/DkCore.o gccDebug/ExceptionHandler.o gccDebug/CommandAccessor.o gccDebug/ConCommandFactory.o gccDebug/CmdLineParser.o gccDebug/DebugInterface.o gccDebug/DPKFileReader.o gccDebug/DPKFileWriter.o gccDebug/FileSystem.o gccDebug/Localize.o gccDebug/ppmem.o 
	g++ -fPIC -shared -Wl,-soname,libDkCore.so -o gccDebug/libDkCore.so gccDebug/CoreVersion.o gccDebug/CPU.o gccDebug/DkCore.o gccDebug/ExceptionHandler.o gccDebug/CommandAccessor.o gccDebug/ConCommandFactory.o gccDebug/CmdLineParser.o gccDebug/DebugInterface.o gccDebug/DPKFileReader.o gccDebug/DPKFileWriter.o gccDebug/FileSystem.o gccDebug/Localize.o gccDebug/ppmem.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file CoreVersion.cpp for the Debug configuration...
-include gccDebug/CoreVersion.d
gccDebug/CoreVersion.o: CoreVersion.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CoreVersion.cpp $(Debug_Include_Path) -o gccDebug/CoreVersion.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CoreVersion.cpp $(Debug_Include_Path) > gccDebug/CoreVersion.d

# Compiles file CPU.cpp for the Debug configuration...
-include gccDebug/CPU.d
gccDebug/CPU.o: CPU.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CPU.cpp $(Debug_Include_Path) -o gccDebug/CPU.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CPU.cpp $(Debug_Include_Path) > gccDebug/CPU.d

# Compiles file DkCore.cpp for the Debug configuration...
-include gccDebug/DkCore.d
gccDebug/DkCore.o: DkCore.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DkCore.cpp $(Debug_Include_Path) -o gccDebug/DkCore.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DkCore.cpp $(Debug_Include_Path) > gccDebug/DkCore.d

# Compiles file ExceptionHandler.cpp for the Debug configuration...
-include gccDebug/ExceptionHandler.d
gccDebug/ExceptionHandler.o: ExceptionHandler.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ExceptionHandler.cpp $(Debug_Include_Path) -o gccDebug/ExceptionHandler.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ExceptionHandler.cpp $(Debug_Include_Path) > gccDebug/ExceptionHandler.d

# Compiles file CommandAccessor.cpp for the Debug configuration...
-include gccDebug/CommandAccessor.d
gccDebug/CommandAccessor.o: CommandAccessor.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CommandAccessor.cpp $(Debug_Include_Path) -o gccDebug/CommandAccessor.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CommandAccessor.cpp $(Debug_Include_Path) > gccDebug/CommandAccessor.d

# Compiles file ConCommandFactory.cpp for the Debug configuration...
-include gccDebug/ConCommandFactory.d
gccDebug/ConCommandFactory.o: ConCommandFactory.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ConCommandFactory.cpp $(Debug_Include_Path) -o gccDebug/ConCommandFactory.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ConCommandFactory.cpp $(Debug_Include_Path) > gccDebug/ConCommandFactory.d

# Compiles file CmdLineParser.cpp for the Debug configuration...
-include gccDebug/CmdLineParser.d
gccDebug/CmdLineParser.o: CmdLineParser.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CmdLineParser.cpp $(Debug_Include_Path) -o gccDebug/CmdLineParser.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CmdLineParser.cpp $(Debug_Include_Path) > gccDebug/CmdLineParser.d

# Compiles file DebugInterface.cpp for the Debug configuration...
-include gccDebug/DebugInterface.d
gccDebug/DebugInterface.o: DebugInterface.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DebugInterface.cpp $(Debug_Include_Path) -o gccDebug/DebugInterface.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DebugInterface.cpp $(Debug_Include_Path) > gccDebug/DebugInterface.d

# Compiles file DPKFileReader.cpp for the Debug configuration...
-include gccDebug/DPKFileReader.d
gccDebug/DPKFileReader.o: DPKFileReader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DPKFileReader.cpp $(Debug_Include_Path) -o gccDebug/DPKFileReader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DPKFileReader.cpp $(Debug_Include_Path) > gccDebug/DPKFileReader.d

# Compiles file DPKFileWriter.cpp for the Debug configuration...
-include gccDebug/DPKFileWriter.d
gccDebug/DPKFileWriter.o: DPKFileWriter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DPKFileWriter.cpp $(Debug_Include_Path) -o gccDebug/DPKFileWriter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DPKFileWriter.cpp $(Debug_Include_Path) > gccDebug/DPKFileWriter.d

# Compiles file FileSystem.cpp for the Debug configuration...
-include gccDebug/FileSystem.d
gccDebug/FileSystem.o: FileSystem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c FileSystem.cpp $(Debug_Include_Path) -o gccDebug/FileSystem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM FileSystem.cpp $(Debug_Include_Path) > gccDebug/FileSystem.d

# Compiles file Localize.cpp for the Debug configuration...
-include gccDebug/Localize.d
gccDebug/Localize.o: Localize.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Localize.cpp $(Debug_Include_Path) -o gccDebug/Localize.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Localize.cpp $(Debug_Include_Path) > gccDebug/Localize.d

# Compiles file ppmem.cpp for the Debug configuration...
-include gccDebug/ppmem.d
gccDebug/ppmem.o: ppmem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ppmem.cpp $(Debug_Include_Path) -o gccDebug/ppmem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ppmem.cpp $(Debug_Include_Path) > gccDebug/ppmem.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/CoreVersion.o gccRelease/CPU.o gccRelease/DkCore.o gccRelease/ExceptionHandler.o gccRelease/CommandAccessor.o gccRelease/ConCommandFactory.o gccRelease/CmdLineParser.o gccRelease/DebugInterface.o gccRelease/DPKFileReader.o gccRelease/DPKFileWriter.o gccRelease/FileSystem.o gccRelease/Localize.o gccRelease/ppmem.o 
	g++ -fPIC -shared -Wl,-soname,libDkCore.so -o gccRelease/libDkCore.so gccRelease/CoreVersion.o gccRelease/CPU.o gccRelease/DkCore.o gccRelease/ExceptionHandler.o gccRelease/CommandAccessor.o gccRelease/ConCommandFactory.o gccRelease/CmdLineParser.o gccRelease/DebugInterface.o gccRelease/DPKFileReader.o gccRelease/DPKFileWriter.o gccRelease/FileSystem.o gccRelease/Localize.o gccRelease/ppmem.o  $(Release_Implicitly_Linked_Objects)

# Compiles file CoreVersion.cpp for the Release configuration...
-include gccRelease/CoreVersion.d
gccRelease/CoreVersion.o: CoreVersion.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CoreVersion.cpp $(Release_Include_Path) -o gccRelease/CoreVersion.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CoreVersion.cpp $(Release_Include_Path) > gccRelease/CoreVersion.d

# Compiles file CPU.cpp for the Release configuration...
-include gccRelease/CPU.d
gccRelease/CPU.o: CPU.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CPU.cpp $(Release_Include_Path) -o gccRelease/CPU.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CPU.cpp $(Release_Include_Path) > gccRelease/CPU.d

# Compiles file DkCore.cpp for the Release configuration...
-include gccRelease/DkCore.d
gccRelease/DkCore.o: DkCore.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DkCore.cpp $(Release_Include_Path) -o gccRelease/DkCore.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DkCore.cpp $(Release_Include_Path) > gccRelease/DkCore.d

# Compiles file ExceptionHandler.cpp for the Release configuration...
-include gccRelease/ExceptionHandler.d
gccRelease/ExceptionHandler.o: ExceptionHandler.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ExceptionHandler.cpp $(Release_Include_Path) -o gccRelease/ExceptionHandler.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ExceptionHandler.cpp $(Release_Include_Path) > gccRelease/ExceptionHandler.d

# Compiles file CommandAccessor.cpp for the Release configuration...
-include gccRelease/CommandAccessor.d
gccRelease/CommandAccessor.o: CommandAccessor.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CommandAccessor.cpp $(Release_Include_Path) -o gccRelease/CommandAccessor.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CommandAccessor.cpp $(Release_Include_Path) > gccRelease/CommandAccessor.d

# Compiles file ConCommandFactory.cpp for the Release configuration...
-include gccRelease/ConCommandFactory.d
gccRelease/ConCommandFactory.o: ConCommandFactory.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ConCommandFactory.cpp $(Release_Include_Path) -o gccRelease/ConCommandFactory.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ConCommandFactory.cpp $(Release_Include_Path) > gccRelease/ConCommandFactory.d

# Compiles file CmdLineParser.cpp for the Release configuration...
-include gccRelease/CmdLineParser.d
gccRelease/CmdLineParser.o: CmdLineParser.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CmdLineParser.cpp $(Release_Include_Path) -o gccRelease/CmdLineParser.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CmdLineParser.cpp $(Release_Include_Path) > gccRelease/CmdLineParser.d

# Compiles file DebugInterface.cpp for the Release configuration...
-include gccRelease/DebugInterface.d
gccRelease/DebugInterface.o: DebugInterface.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DebugInterface.cpp $(Release_Include_Path) -o gccRelease/DebugInterface.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DebugInterface.cpp $(Release_Include_Path) > gccRelease/DebugInterface.d

# Compiles file DPKFileReader.cpp for the Release configuration...
-include gccRelease/DPKFileReader.d
gccRelease/DPKFileReader.o: DPKFileReader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DPKFileReader.cpp $(Release_Include_Path) -o gccRelease/DPKFileReader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DPKFileReader.cpp $(Release_Include_Path) > gccRelease/DPKFileReader.d

# Compiles file DPKFileWriter.cpp for the Release configuration...
-include gccRelease/DPKFileWriter.d
gccRelease/DPKFileWriter.o: DPKFileWriter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DPKFileWriter.cpp $(Release_Include_Path) -o gccRelease/DPKFileWriter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DPKFileWriter.cpp $(Release_Include_Path) > gccRelease/DPKFileWriter.d

# Compiles file FileSystem.cpp for the Release configuration...
-include gccRelease/FileSystem.d
gccRelease/FileSystem.o: FileSystem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c FileSystem.cpp $(Release_Include_Path) -o gccRelease/FileSystem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM FileSystem.cpp $(Release_Include_Path) > gccRelease/FileSystem.d

# Compiles file Localize.cpp for the Release configuration...
-include gccRelease/Localize.d
gccRelease/Localize.o: Localize.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Localize.cpp $(Release_Include_Path) -o gccRelease/Localize.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Localize.cpp $(Release_Include_Path) > gccRelease/Localize.d

# Compiles file ppmem.cpp for the Release configuration...
-include gccRelease/ppmem.d
gccRelease/ppmem.o: ppmem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ppmem.cpp $(Release_Include_Path) -o gccRelease/ppmem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ppmem.cpp $(Release_Include_Path) > gccRelease/ppmem.d

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

