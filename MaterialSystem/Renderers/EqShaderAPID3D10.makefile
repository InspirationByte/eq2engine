# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../public/materialsystem/renderers" -I"../../src_dependency/sdl2/include" 
ReleaseEX_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/platform" -I"../renderers/" -I"../renderers/D3D10" 
Release_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../public/materialsystem/renderers" -I"../../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
ReleaseEX_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -ldxgi -lD3D10 -lD3DX10 -luser32 -lgdi32  -Wl,--end-group
ReleaseEX_Libraries=-Wl,--start-group -lD3D10 -lD3DX10 -luser32 -lgdi32  -Wl,--end-group
Release_Libraries=-Wl,--start-group -ldxgi -lD3D10 -lD3DX10  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D RENDER_EXPORT 
ReleaseEX_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_DEPRECATE -D SUPRESS_DEVMESSAGES -D USECRTMEMDEBUG -D RENDER_EXPORT -D USE_D3DEX 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D RENDER_EXPORT 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
ReleaseEX_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-fPIC -Wall -O0 -g 
ReleaseEX_Compiler_Flags=-fPIC -Wall -O2 
Release_Compiler_Flags=-fPIC -Wall -O3 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug ReleaseEX Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug_d3d10/CTexture.o gccDebug_d3d10/D3D10/D3D10SwapChain.o gccDebug_d3d10/D3D10/D3D10Texture.o gccDebug_d3d10/D3D10/D3D10MeshBuilder.o gccDebug_d3d10/D3D10/D3D10ShaderProgram.o gccDebug_d3d10/D3D10/D3DLibrary.o gccDebug_d3d10/D3D10/IndexBufferD3DX10.o gccDebug_d3d10/D3D10/ShaderAPID3DX10.o gccDebug_d3d10/D3D10/VertexBufferD3DX10.o gccDebug_d3d10/D3D10/VertexFormatD3DX10.o gccDebug_d3d10/ShaderAPI_Base.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPID3D10.so -o gccDebug_d3d10/libEqShaderAPID3D10.so gccDebug_d3d10/CTexture.o gccDebug_d3d10/D3D10/D3D10SwapChain.o gccDebug_d3d10/D3D10/D3D10Texture.o gccDebug_d3d10/D3D10/D3D10MeshBuilder.o gccDebug_d3d10/D3D10/D3D10ShaderProgram.o gccDebug_d3d10/D3D10/D3DLibrary.o gccDebug_d3d10/D3D10/IndexBufferD3DX10.o gccDebug_d3d10/D3D10/ShaderAPID3DX10.o gccDebug_d3d10/D3D10/VertexBufferD3DX10.o gccDebug_d3d10/D3D10/VertexFormatD3DX10.o gccDebug_d3d10/ShaderAPI_Base.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the Debug configuration...
-include gccDebug_d3d10/CTexture.d
gccDebug_d3d10/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CTexture.cpp $(Debug_Include_Path) -o gccDebug_d3d10/CTexture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CTexture.cpp $(Debug_Include_Path) > gccDebug_d3d10/CTexture.d

# Compiles file D3D10/D3D10SwapChain.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/D3D10SwapChain.d
gccDebug_d3d10/D3D10/D3D10SwapChain.o: D3D10/D3D10SwapChain.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/D3D10SwapChain.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/D3D10SwapChain.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/D3D10SwapChain.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/D3D10SwapChain.d

# Compiles file D3D10/D3D10Texture.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/D3D10Texture.d
gccDebug_d3d10/D3D10/D3D10Texture.o: D3D10/D3D10Texture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/D3D10Texture.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/D3D10Texture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/D3D10Texture.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/D3D10Texture.d

# Compiles file D3D10/D3D10MeshBuilder.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/D3D10MeshBuilder.d
gccDebug_d3d10/D3D10/D3D10MeshBuilder.o: D3D10/D3D10MeshBuilder.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/D3D10MeshBuilder.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/D3D10MeshBuilder.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/D3D10MeshBuilder.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/D3D10MeshBuilder.d

# Compiles file D3D10/D3D10ShaderProgram.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/D3D10ShaderProgram.d
gccDebug_d3d10/D3D10/D3D10ShaderProgram.o: D3D10/D3D10ShaderProgram.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/D3D10ShaderProgram.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/D3D10ShaderProgram.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/D3D10ShaderProgram.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/D3D10ShaderProgram.d

# Compiles file D3D10/D3DLibrary.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/D3DLibrary.d
gccDebug_d3d10/D3D10/D3DLibrary.o: D3D10/D3DLibrary.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/D3DLibrary.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/D3DLibrary.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/D3DLibrary.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/D3DLibrary.d

# Compiles file D3D10/IndexBufferD3DX10.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/IndexBufferD3DX10.d
gccDebug_d3d10/D3D10/IndexBufferD3DX10.o: D3D10/IndexBufferD3DX10.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/IndexBufferD3DX10.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/IndexBufferD3DX10.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/IndexBufferD3DX10.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/IndexBufferD3DX10.d

# Compiles file D3D10/ShaderAPID3DX10.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/ShaderAPID3DX10.d
gccDebug_d3d10/D3D10/ShaderAPID3DX10.o: D3D10/ShaderAPID3DX10.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/ShaderAPID3DX10.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/ShaderAPID3DX10.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/ShaderAPID3DX10.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/ShaderAPID3DX10.d

# Compiles file D3D10/VertexBufferD3DX10.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/VertexBufferD3DX10.d
gccDebug_d3d10/D3D10/VertexBufferD3DX10.o: D3D10/VertexBufferD3DX10.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/VertexBufferD3DX10.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/VertexBufferD3DX10.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/VertexBufferD3DX10.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/VertexBufferD3DX10.d

# Compiles file D3D10/VertexFormatD3DX10.cpp for the Debug configuration...
-include gccDebug_d3d10/D3D10/VertexFormatD3DX10.d
gccDebug_d3d10/D3D10/VertexFormatD3DX10.o: D3D10/VertexFormatD3DX10.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c D3D10/VertexFormatD3DX10.cpp $(Debug_Include_Path) -o gccDebug_d3d10/D3D10/VertexFormatD3DX10.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM D3D10/VertexFormatD3DX10.cpp $(Debug_Include_Path) > gccDebug_d3d10/D3D10/VertexFormatD3DX10.d

# Compiles file ShaderAPI_Base.cpp for the Debug configuration...
-include gccDebug_d3d10/ShaderAPI_Base.d
gccDebug_d3d10/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ShaderAPI_Base.cpp $(Debug_Include_Path) -o gccDebug_d3d10/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Debug_Include_Path) > gccDebug_d3d10/ShaderAPI_Base.d

# Builds the ReleaseEX configuration...
.PHONY: ReleaseEX
ReleaseEX: create_folders gccReleaseEX/CTexture.o gccReleaseEX/D3D10/D3D10SwapChain.o gccReleaseEX/D3D10/D3D10Texture.o gccReleaseEX/D3D10/D3D10MeshBuilder.o gccReleaseEX/D3D10/D3D10ShaderProgram.o gccReleaseEX/D3D10/D3DLibrary.o gccReleaseEX/D3D10/IndexBufferD3DX10.o gccReleaseEX/D3D10/ShaderAPID3DX10.o gccReleaseEX/D3D10/VertexBufferD3DX10.o gccReleaseEX/D3D10/VertexFormatD3DX10.o gccReleaseEX/ShaderAPI_Base.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPID3D10.so -o ../../gccReleaseEX/libEqShaderAPID3D10.so gccReleaseEX/CTexture.o gccReleaseEX/D3D10/D3D10SwapChain.o gccReleaseEX/D3D10/D3D10Texture.o gccReleaseEX/D3D10/D3D10MeshBuilder.o gccReleaseEX/D3D10/D3D10ShaderProgram.o gccReleaseEX/D3D10/D3DLibrary.o gccReleaseEX/D3D10/IndexBufferD3DX10.o gccReleaseEX/D3D10/ShaderAPID3DX10.o gccReleaseEX/D3D10/VertexBufferD3DX10.o gccReleaseEX/D3D10/VertexFormatD3DX10.o gccReleaseEX/ShaderAPI_Base.o  $(ReleaseEX_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the ReleaseEX configuration...
-include gccReleaseEX/CTexture.d
gccReleaseEX/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c CTexture.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/CTexture.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM CTexture.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/CTexture.d

# Compiles file D3D10/D3D10SwapChain.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/D3D10SwapChain.d
gccReleaseEX/D3D10/D3D10SwapChain.o: D3D10/D3D10SwapChain.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/D3D10SwapChain.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/D3D10SwapChain.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/D3D10SwapChain.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/D3D10SwapChain.d

# Compiles file D3D10/D3D10Texture.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/D3D10Texture.d
gccReleaseEX/D3D10/D3D10Texture.o: D3D10/D3D10Texture.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/D3D10Texture.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/D3D10Texture.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/D3D10Texture.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/D3D10Texture.d

# Compiles file D3D10/D3D10MeshBuilder.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/D3D10MeshBuilder.d
gccReleaseEX/D3D10/D3D10MeshBuilder.o: D3D10/D3D10MeshBuilder.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/D3D10MeshBuilder.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/D3D10MeshBuilder.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/D3D10MeshBuilder.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/D3D10MeshBuilder.d

# Compiles file D3D10/D3D10ShaderProgram.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/D3D10ShaderProgram.d
gccReleaseEX/D3D10/D3D10ShaderProgram.o: D3D10/D3D10ShaderProgram.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/D3D10ShaderProgram.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/D3D10ShaderProgram.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/D3D10ShaderProgram.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/D3D10ShaderProgram.d

# Compiles file D3D10/D3DLibrary.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/D3DLibrary.d
gccReleaseEX/D3D10/D3DLibrary.o: D3D10/D3DLibrary.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/D3DLibrary.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/D3DLibrary.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/D3DLibrary.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/D3DLibrary.d

# Compiles file D3D10/IndexBufferD3DX10.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/IndexBufferD3DX10.d
gccReleaseEX/D3D10/IndexBufferD3DX10.o: D3D10/IndexBufferD3DX10.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/IndexBufferD3DX10.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/IndexBufferD3DX10.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/IndexBufferD3DX10.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/IndexBufferD3DX10.d

# Compiles file D3D10/ShaderAPID3DX10.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/ShaderAPID3DX10.d
gccReleaseEX/D3D10/ShaderAPID3DX10.o: D3D10/ShaderAPID3DX10.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/ShaderAPID3DX10.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/ShaderAPID3DX10.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/ShaderAPID3DX10.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/ShaderAPID3DX10.d

# Compiles file D3D10/VertexBufferD3DX10.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/VertexBufferD3DX10.d
gccReleaseEX/D3D10/VertexBufferD3DX10.o: D3D10/VertexBufferD3DX10.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/VertexBufferD3DX10.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/VertexBufferD3DX10.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/VertexBufferD3DX10.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/VertexBufferD3DX10.d

# Compiles file D3D10/VertexFormatD3DX10.cpp for the ReleaseEX configuration...
-include gccReleaseEX/D3D10/VertexFormatD3DX10.d
gccReleaseEX/D3D10/VertexFormatD3DX10.o: D3D10/VertexFormatD3DX10.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c D3D10/VertexFormatD3DX10.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/D3D10/VertexFormatD3DX10.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM D3D10/VertexFormatD3DX10.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/D3D10/VertexFormatD3DX10.d

# Compiles file ShaderAPI_Base.cpp for the ReleaseEX configuration...
-include gccReleaseEX/ShaderAPI_Base.d
gccReleaseEX/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -c ShaderAPI_Base.cpp $(ReleaseEX_Include_Path) -o gccReleaseEX/ShaderAPI_Base.o
	$(CPP_COMPILER) $(ReleaseEX_Preprocessor_Definitions) $(ReleaseEX_Compiler_Flags) -MM ShaderAPI_Base.cpp $(ReleaseEX_Include_Path) > gccReleaseEX/ShaderAPI_Base.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease_d3d10/CTexture.o gccRelease_d3d10/D3D10/D3D10SwapChain.o gccRelease_d3d10/D3D10/D3D10Texture.o gccRelease_d3d10/D3D10/D3D10MeshBuilder.o gccRelease_d3d10/D3D10/D3D10ShaderProgram.o gccRelease_d3d10/D3D10/D3DLibrary.o gccRelease_d3d10/D3D10/IndexBufferD3DX10.o gccRelease_d3d10/D3D10/ShaderAPID3DX10.o gccRelease_d3d10/D3D10/VertexBufferD3DX10.o gccRelease_d3d10/D3D10/VertexFormatD3DX10.o gccRelease_d3d10/ShaderAPI_Base.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPID3D10.so -o gccRelease_d3d10/libEqShaderAPID3D10.so gccRelease_d3d10/CTexture.o gccRelease_d3d10/D3D10/D3D10SwapChain.o gccRelease_d3d10/D3D10/D3D10Texture.o gccRelease_d3d10/D3D10/D3D10MeshBuilder.o gccRelease_d3d10/D3D10/D3D10ShaderProgram.o gccRelease_d3d10/D3D10/D3DLibrary.o gccRelease_d3d10/D3D10/IndexBufferD3DX10.o gccRelease_d3d10/D3D10/ShaderAPID3DX10.o gccRelease_d3d10/D3D10/VertexBufferD3DX10.o gccRelease_d3d10/D3D10/VertexFormatD3DX10.o gccRelease_d3d10/ShaderAPI_Base.o  $(Release_Implicitly_Linked_Objects)

# Compiles file CTexture.cpp for the Release configuration...
-include gccRelease_d3d10/CTexture.d
gccRelease_d3d10/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CTexture.cpp $(Release_Include_Path) -o gccRelease_d3d10/CTexture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CTexture.cpp $(Release_Include_Path) > gccRelease_d3d10/CTexture.d

# Compiles file D3D10/D3D10SwapChain.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/D3D10SwapChain.d
gccRelease_d3d10/D3D10/D3D10SwapChain.o: D3D10/D3D10SwapChain.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/D3D10SwapChain.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/D3D10SwapChain.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/D3D10SwapChain.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/D3D10SwapChain.d

# Compiles file D3D10/D3D10Texture.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/D3D10Texture.d
gccRelease_d3d10/D3D10/D3D10Texture.o: D3D10/D3D10Texture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/D3D10Texture.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/D3D10Texture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/D3D10Texture.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/D3D10Texture.d

# Compiles file D3D10/D3D10MeshBuilder.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/D3D10MeshBuilder.d
gccRelease_d3d10/D3D10/D3D10MeshBuilder.o: D3D10/D3D10MeshBuilder.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/D3D10MeshBuilder.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/D3D10MeshBuilder.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/D3D10MeshBuilder.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/D3D10MeshBuilder.d

# Compiles file D3D10/D3D10ShaderProgram.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/D3D10ShaderProgram.d
gccRelease_d3d10/D3D10/D3D10ShaderProgram.o: D3D10/D3D10ShaderProgram.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/D3D10ShaderProgram.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/D3D10ShaderProgram.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/D3D10ShaderProgram.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/D3D10ShaderProgram.d

# Compiles file D3D10/D3DLibrary.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/D3DLibrary.d
gccRelease_d3d10/D3D10/D3DLibrary.o: D3D10/D3DLibrary.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/D3DLibrary.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/D3DLibrary.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/D3DLibrary.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/D3DLibrary.d

# Compiles file D3D10/IndexBufferD3DX10.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/IndexBufferD3DX10.d
gccRelease_d3d10/D3D10/IndexBufferD3DX10.o: D3D10/IndexBufferD3DX10.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/IndexBufferD3DX10.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/IndexBufferD3DX10.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/IndexBufferD3DX10.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/IndexBufferD3DX10.d

# Compiles file D3D10/ShaderAPID3DX10.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/ShaderAPID3DX10.d
gccRelease_d3d10/D3D10/ShaderAPID3DX10.o: D3D10/ShaderAPID3DX10.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/ShaderAPID3DX10.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/ShaderAPID3DX10.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/ShaderAPID3DX10.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/ShaderAPID3DX10.d

# Compiles file D3D10/VertexBufferD3DX10.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/VertexBufferD3DX10.d
gccRelease_d3d10/D3D10/VertexBufferD3DX10.o: D3D10/VertexBufferD3DX10.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/VertexBufferD3DX10.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/VertexBufferD3DX10.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/VertexBufferD3DX10.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/VertexBufferD3DX10.d

# Compiles file D3D10/VertexFormatD3DX10.cpp for the Release configuration...
-include gccRelease_d3d10/D3D10/VertexFormatD3DX10.d
gccRelease_d3d10/D3D10/VertexFormatD3DX10.o: D3D10/VertexFormatD3DX10.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c D3D10/VertexFormatD3DX10.cpp $(Release_Include_Path) -o gccRelease_d3d10/D3D10/VertexFormatD3DX10.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM D3D10/VertexFormatD3DX10.cpp $(Release_Include_Path) > gccRelease_d3d10/D3D10/VertexFormatD3DX10.d

# Compiles file ShaderAPI_Base.cpp for the Release configuration...
-include gccRelease_d3d10/ShaderAPI_Base.d
gccRelease_d3d10/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ShaderAPI_Base.cpp $(Release_Include_Path) -o gccRelease_d3d10/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Release_Include_Path) > gccRelease_d3d10/ShaderAPI_Base.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug_d3d10/source
	mkdir -p gccReleaseEX/source
	mkdir -p ../../gccReleaseEX
	mkdir -p gccRelease_d3d10/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug_d3d10/*.o
	rm -f gccDebug_d3d10/*.d
	rm -f gccDebug_d3d10/*.a
	rm -f gccDebug_d3d10/*.so
	rm -f gccDebug_d3d10/*.dll
	rm -f gccDebug_d3d10/*.exe
	rm -f gccReleaseEX/*.o
	rm -f gccReleaseEX/*.d
	rm -f ../../gccReleaseEX/*.a
	rm -f ../../gccReleaseEX/*.so
	rm -f ../../gccReleaseEX/*.dll
	rm -f ../../gccReleaseEX/*.exe
	rm -f gccRelease_d3d10/*.o
	rm -f gccRelease_d3d10/*.d
	rm -f gccRelease_d3d10/*.a
	rm -f gccRelease_d3d10/*.so
	rm -f gccRelease_d3d10/*.dll
	rm -f gccRelease_d3d10/*.exe

