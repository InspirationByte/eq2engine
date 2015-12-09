# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../FileSystem" -I"../../core" -I"../../public" -I"../../public/core" -I"../../public/Platform" -I"../../Engine" -I"../../src_dependency/bullet/src" -I"../../src_dependency/bullet/src/BulletSoftBody" -I"../../src_dependency/sdl2/include" 
Release_Include_Path=-I"../../FileSystem" -I"../../core" -I"../../public" -I"../../public/core" -I"../../public/Platform" -I"../../Engine" -I"../../src_dependency/bullet/src" -I"../../src_dependency/bullet/src/BulletSoftBody" -I"../../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lgdi32 -lshell32 -lwinmm  -Wl,--end-group
Release_Libraries=-Wl,--start-group -lodbc32 -lodbccp32 -ladvapi32 -luser32 -lkernel32 -lgdi32 -lshell32 -lwinmm  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE -D _TOOLS -D EQLC -D NOENGINE -D USE_SINGLE_CUBEMAPRENDER 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _CONSOLE -D _TOOLS -D EQLC -D NOENGINE -D USE_SINGLE_CUBEMAPRENDER 

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
Debug: create_folders gccDebug/../../Engine/EqGameLevel.o gccDebug/eqlc_main.o gccDebug/lighting.o gccDebug/../../public/BaseShader.o gccDebug/EQLCDepthWrite.o gccDebug/EQLCLightmapFilter.o gccDebug/EQLCPointLight.o gccDebug/EQLCSpotLight.o gccDebug/../../public/Math/math_util.o gccDebug/../../public/Utils/GeomTools.o gccDebug/../../Engine/ViewRenderer.o gccDebug/../../public/BaseRenderableObject.o gccDebug/../../public/RenderList.o gccDebug/../../public/ViewParams.o gccDebug/../../Engine/modelloader_shared.o gccDebug/../../Engine/studio_egf.o gccDebug/../../Engine/Physics/DkBulletObject.o gccDebug/../../Engine/Physics/dkbulletphysics.o gccDebug/../../Engine/Physics/DkPhysicsJoint.o gccDebug/../../Engine/Physics/DkPhysicsRope.o 
	g++ gccDebug/../../Engine/EqGameLevel.o gccDebug/eqlc_main.o gccDebug/lighting.o gccDebug/../../public/BaseShader.o gccDebug/EQLCDepthWrite.o gccDebug/EQLCLightmapFilter.o gccDebug/EQLCPointLight.o gccDebug/EQLCSpotLight.o gccDebug/../../public/Math/math_util.o gccDebug/../../public/Utils/GeomTools.o gccDebug/../../Engine/ViewRenderer.o gccDebug/../../public/BaseRenderableObject.o gccDebug/../../public/RenderList.o gccDebug/../../public/ViewParams.o gccDebug/../../Engine/modelloader_shared.o gccDebug/../../Engine/studio_egf.o gccDebug/../../Engine/Physics/DkBulletObject.o gccDebug/../../Engine/Physics/dkbulletphysics.o gccDebug/../../Engine/Physics/DkPhysicsJoint.o gccDebug/../../Engine/Physics/DkPhysicsRope.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/eqlc.exe

# Compiles file ../../Engine/EqGameLevel.cpp for the Debug configuration...
-include gccDebug/../../Engine/EqGameLevel.d
gccDebug/../../Engine/EqGameLevel.o: ../../Engine/EqGameLevel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/EqGameLevel.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/EqGameLevel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/EqGameLevel.cpp $(Debug_Include_Path) > gccDebug/../../Engine/EqGameLevel.d

# Compiles file eqlc_main.cpp for the Debug configuration...
-include gccDebug/eqlc_main.d
gccDebug/eqlc_main.o: eqlc_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c eqlc_main.cpp $(Debug_Include_Path) -o gccDebug/eqlc_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM eqlc_main.cpp $(Debug_Include_Path) > gccDebug/eqlc_main.d

# Compiles file lighting.cpp for the Debug configuration...
-include gccDebug/lighting.d
gccDebug/lighting.o: lighting.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c lighting.cpp $(Debug_Include_Path) -o gccDebug/lighting.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM lighting.cpp $(Debug_Include_Path) > gccDebug/lighting.d

# Compiles file ../../public/BaseShader.cpp for the Debug configuration...
-include gccDebug/../../public/BaseShader.d
gccDebug/../../public/BaseShader.o: ../../public/BaseShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/BaseShader.cpp $(Debug_Include_Path) -o gccDebug/../../public/BaseShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/BaseShader.cpp $(Debug_Include_Path) > gccDebug/../../public/BaseShader.d

# Compiles file EQLCDepthWrite.cpp for the Debug configuration...
-include gccDebug/EQLCDepthWrite.d
gccDebug/EQLCDepthWrite.o: EQLCDepthWrite.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EQLCDepthWrite.cpp $(Debug_Include_Path) -o gccDebug/EQLCDepthWrite.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EQLCDepthWrite.cpp $(Debug_Include_Path) > gccDebug/EQLCDepthWrite.d

# Compiles file EQLCLightmapFilter.cpp for the Debug configuration...
-include gccDebug/EQLCLightmapFilter.d
gccDebug/EQLCLightmapFilter.o: EQLCLightmapFilter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EQLCLightmapFilter.cpp $(Debug_Include_Path) -o gccDebug/EQLCLightmapFilter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EQLCLightmapFilter.cpp $(Debug_Include_Path) > gccDebug/EQLCLightmapFilter.d

# Compiles file EQLCPointLight.cpp for the Debug configuration...
-include gccDebug/EQLCPointLight.d
gccDebug/EQLCPointLight.o: EQLCPointLight.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EQLCPointLight.cpp $(Debug_Include_Path) -o gccDebug/EQLCPointLight.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EQLCPointLight.cpp $(Debug_Include_Path) > gccDebug/EQLCPointLight.d

# Compiles file EQLCSpotLight.cpp for the Debug configuration...
-include gccDebug/EQLCSpotLight.d
gccDebug/EQLCSpotLight.o: EQLCSpotLight.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EQLCSpotLight.cpp $(Debug_Include_Path) -o gccDebug/EQLCSpotLight.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EQLCSpotLight.cpp $(Debug_Include_Path) > gccDebug/EQLCSpotLight.d

# Compiles file ../../public/Math/math_util.cpp for the Debug configuration...
-include gccDebug/../../public/Math/math_util.d
gccDebug/../../public/Math/math_util.o: ../../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/Math/math_util.cpp $(Debug_Include_Path) -o gccDebug/../../public/Math/math_util.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/Math/math_util.cpp $(Debug_Include_Path) > gccDebug/../../public/Math/math_util.d

# Compiles file ../../public/Utils/GeomTools.cpp for the Debug configuration...
-include gccDebug/../../public/Utils/GeomTools.d
gccDebug/../../public/Utils/GeomTools.o: ../../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/Utils/GeomTools.cpp $(Debug_Include_Path) -o gccDebug/../../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/Utils/GeomTools.cpp $(Debug_Include_Path) > gccDebug/../../public/Utils/GeomTools.d

# Compiles file ../../Engine/ViewRenderer.cpp for the Debug configuration...
-include gccDebug/../../Engine/ViewRenderer.d
gccDebug/../../Engine/ViewRenderer.o: ../../Engine/ViewRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/ViewRenderer.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/ViewRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/ViewRenderer.cpp $(Debug_Include_Path) > gccDebug/../../Engine/ViewRenderer.d

# Compiles file ../../public/BaseRenderableObject.cpp for the Debug configuration...
-include gccDebug/../../public/BaseRenderableObject.d
gccDebug/../../public/BaseRenderableObject.o: ../../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/BaseRenderableObject.cpp $(Debug_Include_Path) -o gccDebug/../../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/BaseRenderableObject.cpp $(Debug_Include_Path) > gccDebug/../../public/BaseRenderableObject.d

# Compiles file ../../public/RenderList.cpp for the Debug configuration...
-include gccDebug/../../public/RenderList.d
gccDebug/../../public/RenderList.o: ../../public/RenderList.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/RenderList.cpp $(Debug_Include_Path) -o gccDebug/../../public/RenderList.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/RenderList.cpp $(Debug_Include_Path) > gccDebug/../../public/RenderList.d

# Compiles file ../../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../../public/ViewParams.d
gccDebug/../../public/ViewParams.o: ../../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../../public/ViewParams.d

# Compiles file ../../Engine/modelloader_shared.cpp for the Debug configuration...
-include gccDebug/../../Engine/modelloader_shared.d
gccDebug/../../Engine/modelloader_shared.o: ../../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/modelloader_shared.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/modelloader_shared.cpp $(Debug_Include_Path) > gccDebug/../../Engine/modelloader_shared.d

# Compiles file ../../Engine/studio_egf.cpp for the Debug configuration...
-include gccDebug/../../Engine/studio_egf.d
gccDebug/../../Engine/studio_egf.o: ../../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/studio_egf.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/studio_egf.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/studio_egf.cpp $(Debug_Include_Path) > gccDebug/../../Engine/studio_egf.d

# Compiles file ../../Engine/Physics/DkBulletObject.cpp for the Debug configuration...
-include gccDebug/../../Engine/Physics/DkBulletObject.d
gccDebug/../../Engine/Physics/DkBulletObject.o: ../../Engine/Physics/DkBulletObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/Physics/DkBulletObject.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/Physics/DkBulletObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/Physics/DkBulletObject.cpp $(Debug_Include_Path) > gccDebug/../../Engine/Physics/DkBulletObject.d

# Compiles file ../../Engine/Physics/dkbulletphysics.cpp for the Debug configuration...
-include gccDebug/../../Engine/Physics/dkbulletphysics.d
gccDebug/../../Engine/Physics/dkbulletphysics.o: ../../Engine/Physics/dkbulletphysics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/Physics/dkbulletphysics.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/Physics/dkbulletphysics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/Physics/dkbulletphysics.cpp $(Debug_Include_Path) > gccDebug/../../Engine/Physics/dkbulletphysics.d

# Compiles file ../../Engine/Physics/DkPhysicsJoint.cpp for the Debug configuration...
-include gccDebug/../../Engine/Physics/DkPhysicsJoint.d
gccDebug/../../Engine/Physics/DkPhysicsJoint.o: ../../Engine/Physics/DkPhysicsJoint.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/Physics/DkPhysicsJoint.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/Physics/DkPhysicsJoint.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/Physics/DkPhysicsJoint.cpp $(Debug_Include_Path) > gccDebug/../../Engine/Physics/DkPhysicsJoint.d

# Compiles file ../../Engine/Physics/DkPhysicsRope.cpp for the Debug configuration...
-include gccDebug/../../Engine/Physics/DkPhysicsRope.d
gccDebug/../../Engine/Physics/DkPhysicsRope.o: ../../Engine/Physics/DkPhysicsRope.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/Physics/DkPhysicsRope.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/Physics/DkPhysicsRope.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/Physics/DkPhysicsRope.cpp $(Debug_Include_Path) > gccDebug/../../Engine/Physics/DkPhysicsRope.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../Engine/EqGameLevel.o gccRelease/eqlc_main.o gccRelease/lighting.o gccRelease/../../public/BaseShader.o gccRelease/EQLCDepthWrite.o gccRelease/EQLCLightmapFilter.o gccRelease/EQLCPointLight.o gccRelease/EQLCSpotLight.o gccRelease/../../public/Math/math_util.o gccRelease/../../public/Utils/GeomTools.o gccRelease/../../Engine/ViewRenderer.o gccRelease/../../public/BaseRenderableObject.o gccRelease/../../public/RenderList.o gccRelease/../../public/ViewParams.o gccRelease/../../Engine/modelloader_shared.o gccRelease/../../Engine/studio_egf.o gccRelease/../../Engine/Physics/DkBulletObject.o gccRelease/../../Engine/Physics/dkbulletphysics.o gccRelease/../../Engine/Physics/DkPhysicsJoint.o gccRelease/../../Engine/Physics/DkPhysicsRope.o 
	g++ gccRelease/../../Engine/EqGameLevel.o gccRelease/eqlc_main.o gccRelease/lighting.o gccRelease/../../public/BaseShader.o gccRelease/EQLCDepthWrite.o gccRelease/EQLCLightmapFilter.o gccRelease/EQLCPointLight.o gccRelease/EQLCSpotLight.o gccRelease/../../public/Math/math_util.o gccRelease/../../public/Utils/GeomTools.o gccRelease/../../Engine/ViewRenderer.o gccRelease/../../public/BaseRenderableObject.o gccRelease/../../public/RenderList.o gccRelease/../../public/ViewParams.o gccRelease/../../Engine/modelloader_shared.o gccRelease/../../Engine/studio_egf.o gccRelease/../../Engine/Physics/DkBulletObject.o gccRelease/../../Engine/Physics/dkbulletphysics.o gccRelease/../../Engine/Physics/DkPhysicsJoint.o gccRelease/../../Engine/Physics/DkPhysicsRope.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/eqlc.exe

# Compiles file ../../Engine/EqGameLevel.cpp for the Release configuration...
-include gccRelease/../../Engine/EqGameLevel.d
gccRelease/../../Engine/EqGameLevel.o: ../../Engine/EqGameLevel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/EqGameLevel.cpp $(Release_Include_Path) -o gccRelease/../../Engine/EqGameLevel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/EqGameLevel.cpp $(Release_Include_Path) > gccRelease/../../Engine/EqGameLevel.d

# Compiles file eqlc_main.cpp for the Release configuration...
-include gccRelease/eqlc_main.d
gccRelease/eqlc_main.o: eqlc_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c eqlc_main.cpp $(Release_Include_Path) -o gccRelease/eqlc_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM eqlc_main.cpp $(Release_Include_Path) > gccRelease/eqlc_main.d

# Compiles file lighting.cpp for the Release configuration...
-include gccRelease/lighting.d
gccRelease/lighting.o: lighting.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c lighting.cpp $(Release_Include_Path) -o gccRelease/lighting.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM lighting.cpp $(Release_Include_Path) > gccRelease/lighting.d

# Compiles file ../../public/BaseShader.cpp for the Release configuration...
-include gccRelease/../../public/BaseShader.d
gccRelease/../../public/BaseShader.o: ../../public/BaseShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/BaseShader.cpp $(Release_Include_Path) -o gccRelease/../../public/BaseShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/BaseShader.cpp $(Release_Include_Path) > gccRelease/../../public/BaseShader.d

# Compiles file EQLCDepthWrite.cpp for the Release configuration...
-include gccRelease/EQLCDepthWrite.d
gccRelease/EQLCDepthWrite.o: EQLCDepthWrite.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EQLCDepthWrite.cpp $(Release_Include_Path) -o gccRelease/EQLCDepthWrite.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EQLCDepthWrite.cpp $(Release_Include_Path) > gccRelease/EQLCDepthWrite.d

# Compiles file EQLCLightmapFilter.cpp for the Release configuration...
-include gccRelease/EQLCLightmapFilter.d
gccRelease/EQLCLightmapFilter.o: EQLCLightmapFilter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EQLCLightmapFilter.cpp $(Release_Include_Path) -o gccRelease/EQLCLightmapFilter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EQLCLightmapFilter.cpp $(Release_Include_Path) > gccRelease/EQLCLightmapFilter.d

# Compiles file EQLCPointLight.cpp for the Release configuration...
-include gccRelease/EQLCPointLight.d
gccRelease/EQLCPointLight.o: EQLCPointLight.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EQLCPointLight.cpp $(Release_Include_Path) -o gccRelease/EQLCPointLight.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EQLCPointLight.cpp $(Release_Include_Path) > gccRelease/EQLCPointLight.d

# Compiles file EQLCSpotLight.cpp for the Release configuration...
-include gccRelease/EQLCSpotLight.d
gccRelease/EQLCSpotLight.o: EQLCSpotLight.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EQLCSpotLight.cpp $(Release_Include_Path) -o gccRelease/EQLCSpotLight.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EQLCSpotLight.cpp $(Release_Include_Path) > gccRelease/EQLCSpotLight.d

# Compiles file ../../public/Math/math_util.cpp for the Release configuration...
-include gccRelease/../../public/Math/math_util.d
gccRelease/../../public/Math/math_util.o: ../../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/Math/math_util.cpp $(Release_Include_Path) -o gccRelease/../../public/Math/math_util.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/Math/math_util.cpp $(Release_Include_Path) > gccRelease/../../public/Math/math_util.d

# Compiles file ../../public/Utils/GeomTools.cpp for the Release configuration...
-include gccRelease/../../public/Utils/GeomTools.d
gccRelease/../../public/Utils/GeomTools.o: ../../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/Utils/GeomTools.cpp $(Release_Include_Path) -o gccRelease/../../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/Utils/GeomTools.cpp $(Release_Include_Path) > gccRelease/../../public/Utils/GeomTools.d

# Compiles file ../../Engine/ViewRenderer.cpp for the Release configuration...
-include gccRelease/../../Engine/ViewRenderer.d
gccRelease/../../Engine/ViewRenderer.o: ../../Engine/ViewRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/ViewRenderer.cpp $(Release_Include_Path) -o gccRelease/../../Engine/ViewRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/ViewRenderer.cpp $(Release_Include_Path) > gccRelease/../../Engine/ViewRenderer.d

# Compiles file ../../public/BaseRenderableObject.cpp for the Release configuration...
-include gccRelease/../../public/BaseRenderableObject.d
gccRelease/../../public/BaseRenderableObject.o: ../../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/BaseRenderableObject.cpp $(Release_Include_Path) -o gccRelease/../../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/BaseRenderableObject.cpp $(Release_Include_Path) > gccRelease/../../public/BaseRenderableObject.d

# Compiles file ../../public/RenderList.cpp for the Release configuration...
-include gccRelease/../../public/RenderList.d
gccRelease/../../public/RenderList.o: ../../public/RenderList.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/RenderList.cpp $(Release_Include_Path) -o gccRelease/../../public/RenderList.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/RenderList.cpp $(Release_Include_Path) > gccRelease/../../public/RenderList.d

# Compiles file ../../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../../public/ViewParams.d
gccRelease/../../public/ViewParams.o: ../../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../../public/ViewParams.d

# Compiles file ../../Engine/modelloader_shared.cpp for the Release configuration...
-include gccRelease/../../Engine/modelloader_shared.d
gccRelease/../../Engine/modelloader_shared.o: ../../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/modelloader_shared.cpp $(Release_Include_Path) -o gccRelease/../../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/modelloader_shared.cpp $(Release_Include_Path) > gccRelease/../../Engine/modelloader_shared.d

# Compiles file ../../Engine/studio_egf.cpp for the Release configuration...
-include gccRelease/../../Engine/studio_egf.d
gccRelease/../../Engine/studio_egf.o: ../../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/studio_egf.cpp $(Release_Include_Path) -o gccRelease/../../Engine/studio_egf.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/studio_egf.cpp $(Release_Include_Path) > gccRelease/../../Engine/studio_egf.d

# Compiles file ../../Engine/Physics/DkBulletObject.cpp for the Release configuration...
-include gccRelease/../../Engine/Physics/DkBulletObject.d
gccRelease/../../Engine/Physics/DkBulletObject.o: ../../Engine/Physics/DkBulletObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/Physics/DkBulletObject.cpp $(Release_Include_Path) -o gccRelease/../../Engine/Physics/DkBulletObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/Physics/DkBulletObject.cpp $(Release_Include_Path) > gccRelease/../../Engine/Physics/DkBulletObject.d

# Compiles file ../../Engine/Physics/dkbulletphysics.cpp for the Release configuration...
-include gccRelease/../../Engine/Physics/dkbulletphysics.d
gccRelease/../../Engine/Physics/dkbulletphysics.o: ../../Engine/Physics/dkbulletphysics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/Physics/dkbulletphysics.cpp $(Release_Include_Path) -o gccRelease/../../Engine/Physics/dkbulletphysics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/Physics/dkbulletphysics.cpp $(Release_Include_Path) > gccRelease/../../Engine/Physics/dkbulletphysics.d

# Compiles file ../../Engine/Physics/DkPhysicsJoint.cpp for the Release configuration...
-include gccRelease/../../Engine/Physics/DkPhysicsJoint.d
gccRelease/../../Engine/Physics/DkPhysicsJoint.o: ../../Engine/Physics/DkPhysicsJoint.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/Physics/DkPhysicsJoint.cpp $(Release_Include_Path) -o gccRelease/../../Engine/Physics/DkPhysicsJoint.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/Physics/DkPhysicsJoint.cpp $(Release_Include_Path) > gccRelease/../../Engine/Physics/DkPhysicsJoint.d

# Compiles file ../../Engine/Physics/DkPhysicsRope.cpp for the Release configuration...
-include gccRelease/../../Engine/Physics/DkPhysicsRope.d
gccRelease/../../Engine/Physics/DkPhysicsRope.o: ../../Engine/Physics/DkPhysicsRope.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/Physics/DkPhysicsRope.cpp $(Release_Include_Path) -o gccRelease/../../Engine/Physics/DkPhysicsRope.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/Physics/DkPhysicsRope.cpp $(Release_Include_Path) > gccRelease/../../Engine/Physics/DkPhysicsRope.d

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

