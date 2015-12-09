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
Debug_Libraries=-Wl,--start-group -ld3d9 -ld3dx9 -luser32 -lgdi32  -Wl,--end-group
Release_Libraries=-Wl,--start-group -ld3d9 -ld3dx9  -Wl,--end-group

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
Debug: create_folders gccDebug_d3d9/CTexture.o gccDebug_d3d9/D3D9/CD3D9Texture.o gccDebug_d3d9/D3D9/D3D9MeshBuilder.o gccDebug_d3d9/D3D9/D3D9ShaderProgram.o gccDebug_d3d9/D3D9/D3D9SwapChain.o gccDebug_d3d9/D3D9/D3DLibrary.o gccDebug_d3d9/D3D9/IndexBufferD3DX9.o gccDebug_d3d9/D3D9/ShaderAPID3DX9.o gccDebug_d3d9/D3D9/VertexBufferD3DX9.o gccDebug_d3d9/D3D9/VertexFormatD3DX9.o gccDebug_d3d9/ShaderAPI_Base.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPID3D9.so -o gccDebug_d3d9/libEqShaderAPID3D9.so gccDebug_d3d9/CTexture.o gccDebug_d3d9/D3D9/CD3D9Texture.o gccDebug_d3d9/D3D9/D3D9MeshBuilder.o gccDebug_d3d9/D3D9/D3D9ShaderProgram.o gccDebug_d3d9/D3D9/D3D9SwapChain.o gccDebug_d3d9/D3D9/D3DLibrary.o gccDebug_d3d9/D3D9/IndexBufferD3DX9.o gccDebug_d3d9/D3D9/ShaderAPID3DX9.o gccDebug_d3d9/D3D9/VertexBufferD3DX9.o gccDebug_d3d9/D3D9/VertexFormatD3DX9.o gccDebug_d3d9/ShaderAPI_Base.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the Debug configuration...
-include gccDebug_d3d9/CTexture.d
gccDebug_d3d9/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CTexture.cpp $(Debug_Include_Path) -o gccDebug_d3d9/CTexture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CTexture.cpp $(Debug_Include_Path) > gccDebug_d3d9/CTexture.d

# Compiles file D3D9/CD3D9Texture.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/CD3D9Texture.d
gccDebug_d3d9/D3D9/CD3D9Texture.o: D3D9/CD3D9Texture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/CD3D9Texture.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/CD3D9Texture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/CD3D9Texture.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/CD3D9Texture.d

# Compiles file D3D9/D3D9MeshBuilder.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/D3D9MeshBuilder.d
gccDebug_d3d9/D3D9/D3D9MeshBuilder.o: D3D9/D3D9MeshBuilder.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/D3D9MeshBuilder.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/D3D9MeshBuilder.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/D3D9MeshBuilder.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/D3D9MeshBuilder.d

# Compiles file D3D9/D3D9ShaderProgram.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/D3D9ShaderProgram.d
gccDebug_d3d9/D3D9/D3D9ShaderProgram.o: D3D9/D3D9ShaderProgram.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/D3D9ShaderProgram.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/D3D9ShaderProgram.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/D3D9ShaderProgram.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/D3D9ShaderProgram.d

# Compiles file D3D9/D3D9SwapChain.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/D3D9SwapChain.d
gccDebug_d3d9/D3D9/D3D9SwapChain.o: D3D9/D3D9SwapChain.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/D3D9SwapChain.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/D3D9SwapChain.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/D3D9SwapChain.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/D3D9SwapChain.d

# Compiles file D3D9/D3DLibrary.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/D3DLibrary.d
gccDebug_d3d9/D3D9/D3DLibrary.o: D3D9/D3DLibrary.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/D3DLibrary.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/D3DLibrary.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/D3DLibrary.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/D3DLibrary.d

# Compiles file D3D9/IndexBufferD3DX9.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/IndexBufferD3DX9.d
gccDebug_d3d9/D3D9/IndexBufferD3DX9.o: D3D9/IndexBufferD3DX9.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/IndexBufferD3DX9.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/IndexBufferD3DX9.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/IndexBufferD3DX9.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/IndexBufferD3DX9.d

# Compiles file D3D9/ShaderAPID3DX9.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/ShaderAPID3DX9.d
gccDebug_d3d9/D3D9/ShaderAPID3DX9.o: D3D9/ShaderAPID3DX9.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/ShaderAPID3DX9.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/ShaderAPID3DX9.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/ShaderAPID3DX9.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/ShaderAPID3DX9.d

# Compiles file D3D9/VertexBufferD3DX9.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/VertexBufferD3DX9.d
gccDebug_d3d9/D3D9/VertexBufferD3DX9.o: D3D9/VertexBufferD3DX9.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/VertexBufferD3DX9.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/VertexBufferD3DX9.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/VertexBufferD3DX9.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/VertexBufferD3DX9.d

# Compiles file D3D9/VertexFormatD3DX9.cpp for the Debug configuration...
-include gccDebug_d3d9/D3D9/VertexFormatD3DX9.d
gccDebug_d3d9/D3D9/VertexFormatD3DX9.o: D3D9/VertexFormatD3DX9.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D9/VertexFormatD3DX9.cpp $(Debug_Include_Path) -o gccDebug_d3d9/D3D9/VertexFormatD3DX9.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D9/VertexFormatD3DX9.cpp $(Debug_Include_Path) > gccDebug_d3d9/D3D9/VertexFormatD3DX9.d

# Compiles file ShaderAPI_Base.cpp for the Debug configuration...
-include gccDebug_d3d9/ShaderAPI_Base.d
gccDebug_d3d9/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ShaderAPI_Base.cpp $(Debug_Include_Path) -o gccDebug_d3d9/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Debug_Include_Path) > gccDebug_d3d9/ShaderAPI_Base.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease_d3d9/CTexture.o gccRelease_d3d9/D3D9/CD3D9Texture.o gccRelease_d3d9/D3D9/D3D9MeshBuilder.o gccRelease_d3d9/D3D9/D3D9ShaderProgram.o gccRelease_d3d9/D3D9/D3D9SwapChain.o gccRelease_d3d9/D3D9/D3DLibrary.o gccRelease_d3d9/D3D9/IndexBufferD3DX9.o gccRelease_d3d9/D3D9/ShaderAPID3DX9.o gccRelease_d3d9/D3D9/VertexBufferD3DX9.o gccRelease_d3d9/D3D9/VertexFormatD3DX9.o gccRelease_d3d9/ShaderAPI_Base.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPID3D9.so -o gccRelease_d3d9/libEqShaderAPID3D9.so gccRelease_d3d9/CTexture.o gccRelease_d3d9/D3D9/CD3D9Texture.o gccRelease_d3d9/D3D9/D3D9MeshBuilder.o gccRelease_d3d9/D3D9/D3D9ShaderProgram.o gccRelease_d3d9/D3D9/D3D9SwapChain.o gccRelease_d3d9/D3D9/D3DLibrary.o gccRelease_d3d9/D3D9/IndexBufferD3DX9.o gccRelease_d3d9/D3D9/ShaderAPID3DX9.o gccRelease_d3d9/D3D9/VertexBufferD3DX9.o gccRelease_d3d9/D3D9/VertexFormatD3DX9.o gccRelease_d3d9/ShaderAPI_Base.o  $(Release_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the Release configuration...
-include gccRelease_d3d9/CTexture.d
gccRelease_d3d9/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CTexture.cpp $(Release_Include_Path) -o gccRelease_d3d9/CTexture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CTexture.cpp $(Release_Include_Path) > gccRelease_d3d9/CTexture.d

# Compiles file D3D9/CD3D9Texture.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/CD3D9Texture.d
gccRelease_d3d9/D3D9/CD3D9Texture.o: D3D9/CD3D9Texture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/CD3D9Texture.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/CD3D9Texture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/CD3D9Texture.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/CD3D9Texture.d

# Compiles file D3D9/D3D9MeshBuilder.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/D3D9MeshBuilder.d
gccRelease_d3d9/D3D9/D3D9MeshBuilder.o: D3D9/D3D9MeshBuilder.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/D3D9MeshBuilder.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/D3D9MeshBuilder.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/D3D9MeshBuilder.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/D3D9MeshBuilder.d

# Compiles file D3D9/D3D9ShaderProgram.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/D3D9ShaderProgram.d
gccRelease_d3d9/D3D9/D3D9ShaderProgram.o: D3D9/D3D9ShaderProgram.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/D3D9ShaderProgram.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/D3D9ShaderProgram.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/D3D9ShaderProgram.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/D3D9ShaderProgram.d

# Compiles file D3D9/D3D9SwapChain.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/D3D9SwapChain.d
gccRelease_d3d9/D3D9/D3D9SwapChain.o: D3D9/D3D9SwapChain.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/D3D9SwapChain.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/D3D9SwapChain.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/D3D9SwapChain.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/D3D9SwapChain.d

# Compiles file D3D9/D3DLibrary.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/D3DLibrary.d
gccRelease_d3d9/D3D9/D3DLibrary.o: D3D9/D3DLibrary.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/D3DLibrary.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/D3DLibrary.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/D3DLibrary.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/D3DLibrary.d

# Compiles file D3D9/IndexBufferD3DX9.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/IndexBufferD3DX9.d
gccRelease_d3d9/D3D9/IndexBufferD3DX9.o: D3D9/IndexBufferD3DX9.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/IndexBufferD3DX9.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/IndexBufferD3DX9.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/IndexBufferD3DX9.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/IndexBufferD3DX9.d

# Compiles file D3D9/ShaderAPID3DX9.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/ShaderAPID3DX9.d
gccRelease_d3d9/D3D9/ShaderAPID3DX9.o: D3D9/ShaderAPID3DX9.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/ShaderAPID3DX9.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/ShaderAPID3DX9.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/ShaderAPID3DX9.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/ShaderAPID3DX9.d

# Compiles file D3D9/VertexBufferD3DX9.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/VertexBufferD3DX9.d
gccRelease_d3d9/D3D9/VertexBufferD3DX9.o: D3D9/VertexBufferD3DX9.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/VertexBufferD3DX9.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/VertexBufferD3DX9.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/VertexBufferD3DX9.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/VertexBufferD3DX9.d

# Compiles file D3D9/VertexFormatD3DX9.cpp for the Release configuration...
-include gccRelease_d3d9/D3D9/VertexFormatD3DX9.d
gccRelease_d3d9/D3D9/VertexFormatD3DX9.o: D3D9/VertexFormatD3DX9.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D9/VertexFormatD3DX9.cpp $(Release_Include_Path) -o gccRelease_d3d9/D3D9/VertexFormatD3DX9.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D9/VertexFormatD3DX9.cpp $(Release_Include_Path) > gccRelease_d3d9/D3D9/VertexFormatD3DX9.d

# Compiles file ShaderAPI_Base.cpp for the Release configuration...
-include gccRelease_d3d9/ShaderAPI_Base.d
gccRelease_d3d9/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ShaderAPI_Base.cpp $(Release_Include_Path) -o gccRelease_d3d9/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Release_Include_Path) > gccRelease_d3d9/ShaderAPI_Base.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug_d3d9/source
	mkdir -p gccRelease_d3d9/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug_d3d9/*.o
	rm -f gccDebug_d3d9/*.d
	rm -f gccDebug_d3d9/*.a
	rm -f gccDebug_d3d9/*.so
	rm -f gccDebug_d3d9/*.dll
	rm -f gccDebug_d3d9/*.exe
	rm -f gccRelease_d3d9/*.o
	rm -f gccRelease_d3d9/*.d
	rm -f gccRelease_d3d9/*.a
	rm -f gccRelease_d3d9/*.so
	rm -f gccRelease_d3d9/*.dll
	rm -f gccRelease_d3d9/*.exe

