# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/platform" -I"../../public/renderers" 
Release_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/platform" -I"../../public/renderers" -I"../../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -lOpenGL32 -luser32 -lgdi32  -Wl,--end-group
Release_Libraries=-Wl,--start-group -lOpenGL32  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_DEPRECATE -D SUPRESS_DEVMESSAGES -D USECRTMEMDEBUG -D RENDER_EXPORT 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D CORE_EXPORT -D DLL_EXPORT -D _CRT_SECURE_NO_DEPRECATE -D SUPRESS_DEVMESSAGES -D USECRTMEMDEBUG -D RENDER_EXPORT 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-fPIC -Wall -O2 -g 
Release_Compiler_Flags=-fPIC -Wall -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug_gl/../public/ConCommand.o gccDebug_gl/../public/ConCommandBase.o gccDebug_gl/../public/ConVar.o gccDebug_gl/../Engine/Renderer.o gccDebug_gl/CTexture.o gccDebug_gl/GL/CGL_Fog.o gccDebug_gl/GL/HardwareMesh/GLMesh.o gccDebug_gl/GL/OpenGLRenderer.o gccDebug_gl/ShaderAPI_Base.o gccDebug_gl/GL/CGLRenderLib.o gccDebug_gl/GL/CGLTexture.o gccDebug_gl/GL/IndexBufferGL.o gccDebug_gl/OpenGL3Extensions.o gccDebug_gl/GL/ShaderAPIGL.o gccDebug_gl/GL/VertexBufferGL.o gccDebug_gl/GL/VertexFormatGL.o gccDebug_gl/../Engine/Imaging/ImageLoader.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPIGL.so -o gccDebug_gl/libEqShaderAPIGL.so gccDebug_gl/../public/ConCommand.o gccDebug_gl/../public/ConCommandBase.o gccDebug_gl/../public/ConVar.o gccDebug_gl/../Engine/Renderer.o gccDebug_gl/CTexture.o gccDebug_gl/GL/CGL_Fog.o gccDebug_gl/GL/HardwareMesh/GLMesh.o gccDebug_gl/GL/OpenGLRenderer.o gccDebug_gl/ShaderAPI_Base.o gccDebug_gl/GL/CGLRenderLib.o gccDebug_gl/GL/CGLTexture.o gccDebug_gl/GL/IndexBufferGL.o gccDebug_gl/OpenGL3Extensions.o gccDebug_gl/GL/ShaderAPIGL.o gccDebug_gl/GL/VertexBufferGL.o gccDebug_gl/GL/VertexFormatGL.o gccDebug_gl/../Engine/Imaging/ImageLoader.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file ../public/ConCommand.cpp for the Debug configuration...
-include gccDebug_gl/../public/ConCommand.d
gccDebug_gl/../public/ConCommand.o: ../public/ConCommand.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ConCommand.cpp $(Debug_Include_Path) -o gccDebug_gl/../public/ConCommand.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ConCommand.cpp $(Debug_Include_Path) > gccDebug_gl/../public/ConCommand.d

# Compiles file ../public/ConCommandBase.cpp for the Debug configuration...
-include gccDebug_gl/../public/ConCommandBase.d
gccDebug_gl/../public/ConCommandBase.o: ../public/ConCommandBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ConCommandBase.cpp $(Debug_Include_Path) -o gccDebug_gl/../public/ConCommandBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ConCommandBase.cpp $(Debug_Include_Path) > gccDebug_gl/../public/ConCommandBase.d

# Compiles file ../public/ConVar.cpp for the Debug configuration...
-include gccDebug_gl/../public/ConVar.d
gccDebug_gl/../public/ConVar.o: ../public/ConVar.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ConVar.cpp $(Debug_Include_Path) -o gccDebug_gl/../public/ConVar.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ConVar.cpp $(Debug_Include_Path) > gccDebug_gl/../public/ConVar.d

# Compiles file ../Engine/Renderer.cpp for the Debug configuration...
-include gccDebug_gl/../Engine/Renderer.d
gccDebug_gl/../Engine/Renderer.o: ../Engine/Renderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Renderer.cpp $(Debug_Include_Path) -o gccDebug_gl/../Engine/Renderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Renderer.cpp $(Debug_Include_Path) > gccDebug_gl/../Engine/Renderer.d

# Compiles file CTexture.cpp for the Debug configuration...
-include gccDebug_gl/CTexture.d
gccDebug_gl/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CTexture.cpp $(Debug_Include_Path) -o gccDebug_gl/CTexture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CTexture.cpp $(Debug_Include_Path) > gccDebug_gl/CTexture.d

# Compiles file GL/CGL_Fog.cpp for the Debug configuration...
-include gccDebug_gl/GL/CGL_Fog.d
gccDebug_gl/GL/CGL_Fog.o: GL/CGL_Fog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/CGL_Fog.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/CGL_Fog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/CGL_Fog.cpp $(Debug_Include_Path) > gccDebug_gl/GL/CGL_Fog.d

# Compiles file GL/HardwareMesh/GLMesh.cpp for the Debug configuration...
-include gccDebug_gl/GL/HardwareMesh/GLMesh.d
gccDebug_gl/GL/HardwareMesh/GLMesh.o: GL/HardwareMesh/GLMesh.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/HardwareMesh/GLMesh.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/HardwareMesh/GLMesh.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/HardwareMesh/GLMesh.cpp $(Debug_Include_Path) > gccDebug_gl/GL/HardwareMesh/GLMesh.d

# Compiles file GL/OpenGLRenderer.cpp for the Debug configuration...
-include gccDebug_gl/GL/OpenGLRenderer.d
gccDebug_gl/GL/OpenGLRenderer.o: GL/OpenGLRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/OpenGLRenderer.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/OpenGLRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/OpenGLRenderer.cpp $(Debug_Include_Path) > gccDebug_gl/GL/OpenGLRenderer.d

# Compiles file ShaderAPI_Base.cpp for the Debug configuration...
-include gccDebug_gl/ShaderAPI_Base.d
gccDebug_gl/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ShaderAPI_Base.cpp $(Debug_Include_Path) -o gccDebug_gl/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Debug_Include_Path) > gccDebug_gl/ShaderAPI_Base.d

# Compiles file GL/CGLRenderLib.cpp for the Debug configuration...
-include gccDebug_gl/GL/CGLRenderLib.d
gccDebug_gl/GL/CGLRenderLib.o: GL/CGLRenderLib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/CGLRenderLib.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/CGLRenderLib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/CGLRenderLib.cpp $(Debug_Include_Path) > gccDebug_gl/GL/CGLRenderLib.d

# Compiles file GL/CGLTexture.cpp for the Debug configuration...
-include gccDebug_gl/GL/CGLTexture.d
gccDebug_gl/GL/CGLTexture.o: GL/CGLTexture.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/CGLTexture.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/CGLTexture.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/CGLTexture.cpp $(Debug_Include_Path) > gccDebug_gl/GL/CGLTexture.d

# Compiles file GL/IndexBufferGL.cpp for the Debug configuration...
-include gccDebug_gl/GL/IndexBufferGL.d
gccDebug_gl/GL/IndexBufferGL.o: GL/IndexBufferGL.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/IndexBufferGL.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/IndexBufferGL.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/IndexBufferGL.cpp $(Debug_Include_Path) > gccDebug_gl/GL/IndexBufferGL.d

# Compiles file OpenGL3Extensions.cpp for the Debug configuration...
-include gccDebug_gl/OpenGL3Extensions.d
gccDebug_gl/OpenGL3Extensions.o: OpenGL3Extensions.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c OpenGL3Extensions.cpp $(Debug_Include_Path) -o gccDebug_gl/OpenGL3Extensions.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM OpenGL3Extensions.cpp $(Debug_Include_Path) > gccDebug_gl/OpenGL3Extensions.d

# Compiles file GL/ShaderAPIGL.cpp for the Debug configuration...
-include gccDebug_gl/GL/ShaderAPIGL.d
gccDebug_gl/GL/ShaderAPIGL.o: GL/ShaderAPIGL.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/ShaderAPIGL.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/ShaderAPIGL.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/ShaderAPIGL.cpp $(Debug_Include_Path) > gccDebug_gl/GL/ShaderAPIGL.d

# Compiles file GL/VertexBufferGL.cpp for the Debug configuration...
-include gccDebug_gl/GL/VertexBufferGL.d
gccDebug_gl/GL/VertexBufferGL.o: GL/VertexBufferGL.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/VertexBufferGL.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/VertexBufferGL.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/VertexBufferGL.cpp $(Debug_Include_Path) > gccDebug_gl/GL/VertexBufferGL.d

# Compiles file GL/VertexFormatGL.cpp for the Debug configuration...
-include gccDebug_gl/GL/VertexFormatGL.d
gccDebug_gl/GL/VertexFormatGL.o: GL/VertexFormatGL.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GL/VertexFormatGL.cpp $(Debug_Include_Path) -o gccDebug_gl/GL/VertexFormatGL.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GL/VertexFormatGL.cpp $(Debug_Include_Path) > gccDebug_gl/GL/VertexFormatGL.d

# Compiles file ../Engine/Imaging/ImageLoader.cpp for the Debug configuration...
-include gccDebug_gl/../Engine/Imaging/ImageLoader.d
gccDebug_gl/../Engine/Imaging/ImageLoader.o: ../Engine/Imaging/ImageLoader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Imaging/ImageLoader.cpp $(Debug_Include_Path) -o gccDebug_gl/../Engine/Imaging/ImageLoader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Imaging/ImageLoader.cpp $(Debug_Include_Path) > gccDebug_gl/../Engine/Imaging/ImageLoader.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease_gl/../public/ConCommand.o gccRelease_gl/../public/ConCommandBase.o gccRelease_gl/../public/ConVar.o gccRelease_gl/../Engine/Renderer.o gccRelease_gl/CTexture.o gccRelease_gl/GL/CGL_Fog.o gccRelease_gl/GL/HardwareMesh/GLMesh.o gccRelease_gl/GL/OpenGLRenderer.o gccRelease_gl/ShaderAPI_Base.o gccRelease_gl/GL/CGLRenderLib.o gccRelease_gl/GL/CGLTexture.o gccRelease_gl/GL/IndexBufferGL.o gccRelease_gl/OpenGL3Extensions.o gccRelease_gl/GL/ShaderAPIGL.o gccRelease_gl/GL/VertexBufferGL.o gccRelease_gl/GL/VertexFormatGL.o gccRelease_gl/../Engine/Imaging/ImageLoader.o 
	g++ -fPIC -shared -Wl,-soname,libEqShaderAPIGL.so -o gccRelease_gl/libEqShaderAPIGL.so gccRelease_gl/../public/ConCommand.o gccRelease_gl/../public/ConCommandBase.o gccRelease_gl/../public/ConVar.o gccRelease_gl/../Engine/Renderer.o gccRelease_gl/CTexture.o gccRelease_gl/GL/CGL_Fog.o gccRelease_gl/GL/HardwareMesh/GLMesh.o gccRelease_gl/GL/OpenGLRenderer.o gccRelease_gl/ShaderAPI_Base.o gccRelease_gl/GL/CGLRenderLib.o gccRelease_gl/GL/CGLTexture.o gccRelease_gl/GL/IndexBufferGL.o gccRelease_gl/OpenGL3Extensions.o gccRelease_gl/GL/ShaderAPIGL.o gccRelease_gl/GL/VertexBufferGL.o gccRelease_gl/GL/VertexFormatGL.o gccRelease_gl/../Engine/Imaging/ImageLoader.o  $(Release_Implicitly_Linked_Objects)

# Compiles file ../public/ConCommand.cpp for the Release configuration...
-include gccRelease_gl/../public/ConCommand.d
gccRelease_gl/../public/ConCommand.o: ../public/ConCommand.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ConCommand.cpp $(Release_Include_Path) -o gccRelease_gl/../public/ConCommand.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ConCommand.cpp $(Release_Include_Path) > gccRelease_gl/../public/ConCommand.d

# Compiles file ../public/ConCommandBase.cpp for the Release configuration...
-include gccRelease_gl/../public/ConCommandBase.d
gccRelease_gl/../public/ConCommandBase.o: ../public/ConCommandBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ConCommandBase.cpp $(Release_Include_Path) -o gccRelease_gl/../public/ConCommandBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ConCommandBase.cpp $(Release_Include_Path) > gccRelease_gl/../public/ConCommandBase.d

# Compiles file ../public/ConVar.cpp for the Release configuration...
-include gccRelease_gl/../public/ConVar.d
gccRelease_gl/../public/ConVar.o: ../public/ConVar.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ConVar.cpp $(Release_Include_Path) -o gccRelease_gl/../public/ConVar.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ConVar.cpp $(Release_Include_Path) > gccRelease_gl/../public/ConVar.d

# Compiles file ../Engine/Renderer.cpp for the Release configuration...
-include gccRelease_gl/../Engine/Renderer.d
gccRelease_gl/../Engine/Renderer.o: ../Engine/Renderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Renderer.cpp $(Release_Include_Path) -o gccRelease_gl/../Engine/Renderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Renderer.cpp $(Release_Include_Path) > gccRelease_gl/../Engine/Renderer.d

# Compiles file CTexture.cpp for the Release configuration...
-include gccRelease_gl/CTexture.d
gccRelease_gl/CTexture.o: CTexture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CTexture.cpp $(Release_Include_Path) -o gccRelease_gl/CTexture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CTexture.cpp $(Release_Include_Path) > gccRelease_gl/CTexture.d

# Compiles file GL/CGL_Fog.cpp for the Release configuration...
-include gccRelease_gl/GL/CGL_Fog.d
gccRelease_gl/GL/CGL_Fog.o: GL/CGL_Fog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/CGL_Fog.cpp $(Release_Include_Path) -o gccRelease_gl/GL/CGL_Fog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/CGL_Fog.cpp $(Release_Include_Path) > gccRelease_gl/GL/CGL_Fog.d

# Compiles file GL/HardwareMesh/GLMesh.cpp for the Release configuration...
-include gccRelease_gl/GL/HardwareMesh/GLMesh.d
gccRelease_gl/GL/HardwareMesh/GLMesh.o: GL/HardwareMesh/GLMesh.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/HardwareMesh/GLMesh.cpp $(Release_Include_Path) -o gccRelease_gl/GL/HardwareMesh/GLMesh.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/HardwareMesh/GLMesh.cpp $(Release_Include_Path) > gccRelease_gl/GL/HardwareMesh/GLMesh.d

# Compiles file GL/OpenGLRenderer.cpp for the Release configuration...
-include gccRelease_gl/GL/OpenGLRenderer.d
gccRelease_gl/GL/OpenGLRenderer.o: GL/OpenGLRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/OpenGLRenderer.cpp $(Release_Include_Path) -o gccRelease_gl/GL/OpenGLRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/OpenGLRenderer.cpp $(Release_Include_Path) > gccRelease_gl/GL/OpenGLRenderer.d

# Compiles file ShaderAPI_Base.cpp for the Release configuration...
-include gccRelease_gl/ShaderAPI_Base.d
gccRelease_gl/ShaderAPI_Base.o: ShaderAPI_Base.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ShaderAPI_Base.cpp $(Release_Include_Path) -o gccRelease_gl/ShaderAPI_Base.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ShaderAPI_Base.cpp $(Release_Include_Path) > gccRelease_gl/ShaderAPI_Base.d

# Compiles file GL/CGLRenderLib.cpp for the Release configuration...
-include gccRelease_gl/GL/CGLRenderLib.d
gccRelease_gl/GL/CGLRenderLib.o: GL/CGLRenderLib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/CGLRenderLib.cpp $(Release_Include_Path) -o gccRelease_gl/GL/CGLRenderLib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/CGLRenderLib.cpp $(Release_Include_Path) > gccRelease_gl/GL/CGLRenderLib.d

# Compiles file GL/CGLTexture.cpp for the Release configuration...
-include gccRelease_gl/GL/CGLTexture.d
gccRelease_gl/GL/CGLTexture.o: GL/CGLTexture.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/CGLTexture.cpp $(Release_Include_Path) -o gccRelease_gl/GL/CGLTexture.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/CGLTexture.cpp $(Release_Include_Path) > gccRelease_gl/GL/CGLTexture.d

# Compiles file GL/IndexBufferGL.cpp for the Release configuration...
-include gccRelease_gl/GL/IndexBufferGL.d
gccRelease_gl/GL/IndexBufferGL.o: GL/IndexBufferGL.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/IndexBufferGL.cpp $(Release_Include_Path) -o gccRelease_gl/GL/IndexBufferGL.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/IndexBufferGL.cpp $(Release_Include_Path) > gccRelease_gl/GL/IndexBufferGL.d

# Compiles file OpenGL3Extensions.cpp for the Release configuration...
-include gccRelease_gl/OpenGL3Extensions.d
gccRelease_gl/OpenGL3Extensions.o: OpenGL3Extensions.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c OpenGL3Extensions.cpp $(Release_Include_Path) -o gccRelease_gl/OpenGL3Extensions.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM OpenGL3Extensions.cpp $(Release_Include_Path) > gccRelease_gl/OpenGL3Extensions.d

# Compiles file GL/ShaderAPIGL.cpp for the Release configuration...
-include gccRelease_gl/GL/ShaderAPIGL.d
gccRelease_gl/GL/ShaderAPIGL.o: GL/ShaderAPIGL.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/ShaderAPIGL.cpp $(Release_Include_Path) -o gccRelease_gl/GL/ShaderAPIGL.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/ShaderAPIGL.cpp $(Release_Include_Path) > gccRelease_gl/GL/ShaderAPIGL.d

# Compiles file GL/VertexBufferGL.cpp for the Release configuration...
-include gccRelease_gl/GL/VertexBufferGL.d
gccRelease_gl/GL/VertexBufferGL.o: GL/VertexBufferGL.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/VertexBufferGL.cpp $(Release_Include_Path) -o gccRelease_gl/GL/VertexBufferGL.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/VertexBufferGL.cpp $(Release_Include_Path) > gccRelease_gl/GL/VertexBufferGL.d

# Compiles file GL/VertexFormatGL.cpp for the Release configuration...
-include gccRelease_gl/GL/VertexFormatGL.d
gccRelease_gl/GL/VertexFormatGL.o: GL/VertexFormatGL.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GL/VertexFormatGL.cpp $(Release_Include_Path) -o gccRelease_gl/GL/VertexFormatGL.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GL/VertexFormatGL.cpp $(Release_Include_Path) > gccRelease_gl/GL/VertexFormatGL.d

# Compiles file ../Engine/Imaging/ImageLoader.cpp for the Release configuration...
-include gccRelease_gl/../Engine/Imaging/ImageLoader.d
gccRelease_gl/../Engine/Imaging/ImageLoader.o: ../Engine/Imaging/ImageLoader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Imaging/ImageLoader.cpp $(Release_Include_Path) -o gccRelease_gl/../Engine/Imaging/ImageLoader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Imaging/ImageLoader.cpp $(Release_Include_Path) > gccRelease_gl/../Engine/Imaging/ImageLoader.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug_gl/source
	mkdir -p gccRelease_gl/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug_gl/*.o
	rm -f gccDebug_gl/*.d
	rm -f gccDebug_gl/*.a
	rm -f gccDebug_gl/*.so
	rm -f gccDebug_gl/*.dll
	rm -f gccDebug_gl/*.exe
	rm -f gccRelease_gl/*.o
	rm -f gccRelease_gl/*.d
	rm -f gccRelease_gl/*.a
	rm -f gccRelease_gl/*.so
	rm -f gccRelease_gl/*.dll
	rm -f gccRelease_gl/*.exe

