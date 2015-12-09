# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../public/materialsystem/renderers" -I"../src_dependency/bullet/src/BulletSoftBody" -I"../src_dependency/bullet/Extras/Serialization/BulletWorldImporter" -I"../src_dependency/bullet/src" -I"../src_dependency/sdl2/include" 
Release_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../shared_engine" -I"../public/platform" -I"../src_dependency/sdl2/include" -I"../src_dependency/bullet/src" -I"../src_dependency/bullet/Extras/Serialization/BulletWorldImporter" -I"../src_dependency/bullet/src/BulletSoftBody" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -lwsock32 -lwinmm  -Wl,--end-group
Release_Libraries=-Wl,--start-group -lwsock32 -lwinmm  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D USE_SIMD 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D USE_SIMD -D EQ_USE_SDL 

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
Debug: create_folders gccDebug/../public/Font.o gccDebug/UFont.o gccDebug/../public/mtriangle_framework.o gccDebug/../public/Utils/GeomTools.o gccDebug/Audio/soundzero.o gccDebug/Audio/alsnd_ambient.o gccDebug/Audio/alsnd_emitter.o gccDebug/Audio/alsnd_sample.o gccDebug/Audio/alsound_local.o gccDebug/Audio/EqSoundSystemA.o gccDebug/KeyBinding/Keys.o gccDebug/../FileSystem/cfgloader.o gccDebug/EqGameLevel.o gccDebug/CEngineGame.o gccDebug/../public/GlobalVarsBase.o gccDebug/EntityFactory.o gccDebug/../public/Math/math_util.o gccDebug/studio_egf.o gccDebug/modelloader_shared.o gccDebug/Network/NETThread.o gccDebug/../shared_engine/physics/BulletShapeCache.o gccDebug/Physics/DkBulletObject.o gccDebug/Physics/DkBulletPhysics.o gccDebug/Physics/DkPhysicsJoint.o gccDebug/Physics/DkPhysicsRope.o gccDebug/../public/BaseRenderableObject.o gccDebug/../public/Decals.o gccDebug/../public/ViewParams.o gccDebug/BaseViewRenderer.o gccDebug/ViewRenderer.o gccDebug/DebugOverlay.o gccDebug/EngineSpew.o gccDebug/EngineVersion.o gccDebug/Modules.o gccDebug/sys_console.o gccDebug/sys_enginehost.o gccDebug/sys_main.o gccDebug/sys_mainwind.o 
	g++ -fPIC -shared -Wl,-soname,libEqEngine.so -o gccDebug/libEqEngine.so gccDebug/../public/Font.o gccDebug/UFont.o gccDebug/../public/mtriangle_framework.o gccDebug/../public/Utils/GeomTools.o gccDebug/Audio/soundzero.o gccDebug/Audio/alsnd_ambient.o gccDebug/Audio/alsnd_emitter.o gccDebug/Audio/alsnd_sample.o gccDebug/Audio/alsound_local.o gccDebug/Audio/EqSoundSystemA.o gccDebug/KeyBinding/Keys.o gccDebug/../FileSystem/cfgloader.o gccDebug/EqGameLevel.o gccDebug/CEngineGame.o gccDebug/../public/GlobalVarsBase.o gccDebug/EntityFactory.o gccDebug/../public/Math/math_util.o gccDebug/studio_egf.o gccDebug/modelloader_shared.o gccDebug/Network/NETThread.o gccDebug/../shared_engine/physics/BulletShapeCache.o gccDebug/Physics/DkBulletObject.o gccDebug/Physics/DkBulletPhysics.o gccDebug/Physics/DkPhysicsJoint.o gccDebug/Physics/DkPhysicsRope.o gccDebug/../public/BaseRenderableObject.o gccDebug/../public/Decals.o gccDebug/../public/ViewParams.o gccDebug/BaseViewRenderer.o gccDebug/ViewRenderer.o gccDebug/DebugOverlay.o gccDebug/EngineSpew.o gccDebug/EngineVersion.o gccDebug/Modules.o gccDebug/sys_console.o gccDebug/sys_enginehost.o gccDebug/sys_main.o gccDebug/sys_mainwind.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file ../public/Font.cpp for the Debug configuration...
-include gccDebug/../public/Font.d
gccDebug/../public/Font.o: ../public/Font.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Font.cpp $(Debug_Include_Path) -o gccDebug/../public/Font.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Font.cpp $(Debug_Include_Path) > gccDebug/../public/Font.d

# Compiles file UFont.cpp for the Debug configuration...
-include gccDebug/UFont.d
gccDebug/UFont.o: UFont.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c UFont.cpp $(Debug_Include_Path) -o gccDebug/UFont.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM UFont.cpp $(Debug_Include_Path) > gccDebug/UFont.d

# Compiles file ../public/mtriangle_framework.cpp for the Debug configuration...
-include gccDebug/../public/mtriangle_framework.d
gccDebug/../public/mtriangle_framework.o: ../public/mtriangle_framework.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/mtriangle_framework.cpp $(Debug_Include_Path) -o gccDebug/../public/mtriangle_framework.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/mtriangle_framework.cpp $(Debug_Include_Path) > gccDebug/../public/mtriangle_framework.d

# Compiles file ../public/Utils/GeomTools.cpp for the Debug configuration...
-include gccDebug/../public/Utils/GeomTools.d
gccDebug/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(Debug_Include_Path) -o gccDebug/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(Debug_Include_Path) > gccDebug/../public/Utils/GeomTools.d

# Compiles file Audio/soundzero.cpp for the Debug configuration...
-include gccDebug/Audio/soundzero.d
gccDebug/Audio/soundzero.o: Audio/soundzero.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Audio/soundzero.cpp $(Debug_Include_Path) -o gccDebug/Audio/soundzero.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Audio/soundzero.cpp $(Debug_Include_Path) > gccDebug/Audio/soundzero.d

# Compiles file Audio/alsnd_ambient.cpp for the Debug configuration...
-include gccDebug/Audio/alsnd_ambient.d
gccDebug/Audio/alsnd_ambient.o: Audio/alsnd_ambient.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Audio/alsnd_ambient.cpp $(Debug_Include_Path) -o gccDebug/Audio/alsnd_ambient.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Audio/alsnd_ambient.cpp $(Debug_Include_Path) > gccDebug/Audio/alsnd_ambient.d

# Compiles file Audio/alsnd_emitter.cpp for the Debug configuration...
-include gccDebug/Audio/alsnd_emitter.d
gccDebug/Audio/alsnd_emitter.o: Audio/alsnd_emitter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Audio/alsnd_emitter.cpp $(Debug_Include_Path) -o gccDebug/Audio/alsnd_emitter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Audio/alsnd_emitter.cpp $(Debug_Include_Path) > gccDebug/Audio/alsnd_emitter.d

# Compiles file Audio/alsnd_sample.cpp for the Debug configuration...
-include gccDebug/Audio/alsnd_sample.d
gccDebug/Audio/alsnd_sample.o: Audio/alsnd_sample.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Audio/alsnd_sample.cpp $(Debug_Include_Path) -o gccDebug/Audio/alsnd_sample.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Audio/alsnd_sample.cpp $(Debug_Include_Path) > gccDebug/Audio/alsnd_sample.d

# Compiles file Audio/alsound_local.cpp for the Debug configuration...
-include gccDebug/Audio/alsound_local.d
gccDebug/Audio/alsound_local.o: Audio/alsound_local.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Audio/alsound_local.cpp $(Debug_Include_Path) -o gccDebug/Audio/alsound_local.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Audio/alsound_local.cpp $(Debug_Include_Path) > gccDebug/Audio/alsound_local.d

# Compiles file Audio/EqSoundSystemA.cpp for the Debug configuration...
-include gccDebug/Audio/EqSoundSystemA.d
gccDebug/Audio/EqSoundSystemA.o: Audio/EqSoundSystemA.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Audio/EqSoundSystemA.cpp $(Debug_Include_Path) -o gccDebug/Audio/EqSoundSystemA.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Audio/EqSoundSystemA.cpp $(Debug_Include_Path) > gccDebug/Audio/EqSoundSystemA.d

# Compiles file KeyBinding/Keys.cpp for the Debug configuration...
-include gccDebug/KeyBinding/Keys.d
gccDebug/KeyBinding/Keys.o: KeyBinding/Keys.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c KeyBinding/Keys.cpp $(Debug_Include_Path) -o gccDebug/KeyBinding/Keys.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM KeyBinding/Keys.cpp $(Debug_Include_Path) > gccDebug/KeyBinding/Keys.d

# Compiles file ../FileSystem/cfgloader.cpp for the Debug configuration...
-include gccDebug/../FileSystem/cfgloader.d
gccDebug/../FileSystem/cfgloader.o: ../FileSystem/cfgloader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../FileSystem/cfgloader.cpp $(Debug_Include_Path) -o gccDebug/../FileSystem/cfgloader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../FileSystem/cfgloader.cpp $(Debug_Include_Path) > gccDebug/../FileSystem/cfgloader.d

# Compiles file EqGameLevel.cpp for the Debug configuration...
-include gccDebug/EqGameLevel.d
gccDebug/EqGameLevel.o: EqGameLevel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EqGameLevel.cpp $(Debug_Include_Path) -o gccDebug/EqGameLevel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EqGameLevel.cpp $(Debug_Include_Path) > gccDebug/EqGameLevel.d

# Compiles file CEngineGame.cpp for the Debug configuration...
-include gccDebug/CEngineGame.d
gccDebug/CEngineGame.o: CEngineGame.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CEngineGame.cpp $(Debug_Include_Path) -o gccDebug/CEngineGame.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CEngineGame.cpp $(Debug_Include_Path) > gccDebug/CEngineGame.d

# Compiles file ../public/GlobalVarsBase.cpp for the Debug configuration...
-include gccDebug/../public/GlobalVarsBase.d
gccDebug/../public/GlobalVarsBase.o: ../public/GlobalVarsBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/GlobalVarsBase.cpp $(Debug_Include_Path) -o gccDebug/../public/GlobalVarsBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/GlobalVarsBase.cpp $(Debug_Include_Path) > gccDebug/../public/GlobalVarsBase.d

# Compiles file EntityFactory.cpp for the Debug configuration...
-include gccDebug/EntityFactory.d
gccDebug/EntityFactory.o: EntityFactory.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityFactory.cpp $(Debug_Include_Path) -o gccDebug/EntityFactory.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityFactory.cpp $(Debug_Include_Path) > gccDebug/EntityFactory.d

# Compiles file ../public/Math/math_util.cpp for the Debug configuration...
-include gccDebug/../public/Math/math_util.d
gccDebug/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Math/math_util.cpp $(Debug_Include_Path) -o gccDebug/../public/Math/math_util.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Math/math_util.cpp $(Debug_Include_Path) > gccDebug/../public/Math/math_util.d

# Compiles file studio_egf.cpp for the Debug configuration...
-include gccDebug/studio_egf.d
gccDebug/studio_egf.o: studio_egf.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c studio_egf.cpp $(Debug_Include_Path) -o gccDebug/studio_egf.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM studio_egf.cpp $(Debug_Include_Path) > gccDebug/studio_egf.d

# Compiles file modelloader_shared.cpp for the Debug configuration...
-include gccDebug/modelloader_shared.d
gccDebug/modelloader_shared.o: modelloader_shared.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c modelloader_shared.cpp $(Debug_Include_Path) -o gccDebug/modelloader_shared.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM modelloader_shared.cpp $(Debug_Include_Path) > gccDebug/modelloader_shared.d

# Compiles file Network/NETThread.cpp for the Debug configuration...
-include gccDebug/Network/NETThread.d
gccDebug/Network/NETThread.o: Network/NETThread.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Network/NETThread.cpp $(Debug_Include_Path) -o gccDebug/Network/NETThread.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Network/NETThread.cpp $(Debug_Include_Path) > gccDebug/Network/NETThread.d

# Compiles file ../shared_engine/physics/BulletShapeCache.cpp for the Debug configuration...
-include gccDebug/../shared_engine/physics/BulletShapeCache.d
gccDebug/../shared_engine/physics/BulletShapeCache.o: ../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) -o gccDebug/../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) > gccDebug/../shared_engine/physics/BulletShapeCache.d

# Compiles file Physics/DkBulletObject.cpp for the Debug configuration...
-include gccDebug/Physics/DkBulletObject.d
gccDebug/Physics/DkBulletObject.o: Physics/DkBulletObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Physics/DkBulletObject.cpp $(Debug_Include_Path) -o gccDebug/Physics/DkBulletObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Physics/DkBulletObject.cpp $(Debug_Include_Path) > gccDebug/Physics/DkBulletObject.d

# Compiles file Physics/DkBulletPhysics.cpp for the Debug configuration...
-include gccDebug/Physics/DkBulletPhysics.d
gccDebug/Physics/DkBulletPhysics.o: Physics/DkBulletPhysics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Physics/DkBulletPhysics.cpp $(Debug_Include_Path) -o gccDebug/Physics/DkBulletPhysics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Physics/DkBulletPhysics.cpp $(Debug_Include_Path) > gccDebug/Physics/DkBulletPhysics.d

# Compiles file Physics/DkPhysicsJoint.cpp for the Debug configuration...
-include gccDebug/Physics/DkPhysicsJoint.d
gccDebug/Physics/DkPhysicsJoint.o: Physics/DkPhysicsJoint.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Physics/DkPhysicsJoint.cpp $(Debug_Include_Path) -o gccDebug/Physics/DkPhysicsJoint.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Physics/DkPhysicsJoint.cpp $(Debug_Include_Path) > gccDebug/Physics/DkPhysicsJoint.d

# Compiles file Physics/DkPhysicsRope.cpp for the Debug configuration...
-include gccDebug/Physics/DkPhysicsRope.d
gccDebug/Physics/DkPhysicsRope.o: Physics/DkPhysicsRope.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Physics/DkPhysicsRope.cpp $(Debug_Include_Path) -o gccDebug/Physics/DkPhysicsRope.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Physics/DkPhysicsRope.cpp $(Debug_Include_Path) > gccDebug/Physics/DkPhysicsRope.d

# Compiles file ../public/BaseRenderableObject.cpp for the Debug configuration...
-include gccDebug/../public/BaseRenderableObject.d
gccDebug/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(Debug_Include_Path) -o gccDebug/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(Debug_Include_Path) > gccDebug/../public/BaseRenderableObject.d

# Compiles file ../public/Decals.cpp for the Debug configuration...
-include gccDebug/../public/Decals.d
gccDebug/../public/Decals.o: ../public/Decals.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Decals.cpp $(Debug_Include_Path) -o gccDebug/../public/Decals.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Decals.cpp $(Debug_Include_Path) > gccDebug/../public/Decals.d

# Compiles file ../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../public/ViewParams.d
gccDebug/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../public/ViewParams.d

# Compiles file BaseViewRenderer.cpp for the Debug configuration...
-include gccDebug/BaseViewRenderer.d
gccDebug/BaseViewRenderer.o: BaseViewRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseViewRenderer.cpp $(Debug_Include_Path) -o gccDebug/BaseViewRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseViewRenderer.cpp $(Debug_Include_Path) > gccDebug/BaseViewRenderer.d

# Compiles file ViewRenderer.cpp for the Debug configuration...
-include gccDebug/ViewRenderer.d
gccDebug/ViewRenderer.o: ViewRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ViewRenderer.cpp $(Debug_Include_Path) -o gccDebug/ViewRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ViewRenderer.cpp $(Debug_Include_Path) > gccDebug/ViewRenderer.d

# Compiles file DebugOverlay.cpp for the Debug configuration...
-include gccDebug/DebugOverlay.d
gccDebug/DebugOverlay.o: DebugOverlay.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DebugOverlay.cpp $(Debug_Include_Path) -o gccDebug/DebugOverlay.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DebugOverlay.cpp $(Debug_Include_Path) > gccDebug/DebugOverlay.d

# Compiles file EngineSpew.cpp for the Debug configuration...
-include gccDebug/EngineSpew.d
gccDebug/EngineSpew.o: EngineSpew.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EngineSpew.cpp $(Debug_Include_Path) -o gccDebug/EngineSpew.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EngineSpew.cpp $(Debug_Include_Path) > gccDebug/EngineSpew.d

# Compiles file EngineVersion.cpp for the Debug configuration...
-include gccDebug/EngineVersion.d
gccDebug/EngineVersion.o: EngineVersion.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EngineVersion.cpp $(Debug_Include_Path) -o gccDebug/EngineVersion.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EngineVersion.cpp $(Debug_Include_Path) > gccDebug/EngineVersion.d

# Compiles file Modules.cpp for the Debug configuration...
-include gccDebug/Modules.d
gccDebug/Modules.o: Modules.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Modules.cpp $(Debug_Include_Path) -o gccDebug/Modules.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Modules.cpp $(Debug_Include_Path) > gccDebug/Modules.d

# Compiles file sys_console.cpp for the Debug configuration...
-include gccDebug/sys_console.d
gccDebug/sys_console.o: sys_console.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c sys_console.cpp $(Debug_Include_Path) -o gccDebug/sys_console.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM sys_console.cpp $(Debug_Include_Path) > gccDebug/sys_console.d

# Compiles file sys_enginehost.cpp for the Debug configuration...
-include gccDebug/sys_enginehost.d
gccDebug/sys_enginehost.o: sys_enginehost.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c sys_enginehost.cpp $(Debug_Include_Path) -o gccDebug/sys_enginehost.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM sys_enginehost.cpp $(Debug_Include_Path) > gccDebug/sys_enginehost.d

# Compiles file sys_main.cpp for the Debug configuration...
-include gccDebug/sys_main.d
gccDebug/sys_main.o: sys_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c sys_main.cpp $(Debug_Include_Path) -o gccDebug/sys_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM sys_main.cpp $(Debug_Include_Path) > gccDebug/sys_main.d

# Compiles file sys_mainwind.cpp for the Debug configuration...
-include gccDebug/sys_mainwind.d
gccDebug/sys_mainwind.o: sys_mainwind.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c sys_mainwind.cpp $(Debug_Include_Path) -o gccDebug/sys_mainwind.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM sys_mainwind.cpp $(Debug_Include_Path) > gccDebug/sys_mainwind.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../public/Font.o gccRelease/UFont.o gccRelease/../public/mtriangle_framework.o gccRelease/../public/Utils/GeomTools.o gccRelease/Audio/soundzero.o gccRelease/Audio/alsnd_ambient.o gccRelease/Audio/alsnd_emitter.o gccRelease/Audio/alsnd_sample.o gccRelease/Audio/alsound_local.o gccRelease/Audio/EqSoundSystemA.o gccRelease/KeyBinding/Keys.o gccRelease/../FileSystem/cfgloader.o gccRelease/EqGameLevel.o gccRelease/CEngineGame.o gccRelease/../public/GlobalVarsBase.o gccRelease/EntityFactory.o gccRelease/../public/Math/math_util.o gccRelease/studio_egf.o gccRelease/modelloader_shared.o gccRelease/Network/NETThread.o gccRelease/../shared_engine/physics/BulletShapeCache.o gccRelease/Physics/DkBulletObject.o gccRelease/Physics/DkBulletPhysics.o gccRelease/Physics/DkPhysicsJoint.o gccRelease/Physics/DkPhysicsRope.o gccRelease/../public/BaseRenderableObject.o gccRelease/../public/Decals.o gccRelease/../public/ViewParams.o gccRelease/BaseViewRenderer.o gccRelease/ViewRenderer.o gccRelease/DebugOverlay.o gccRelease/EngineSpew.o gccRelease/EngineVersion.o gccRelease/Modules.o gccRelease/sys_console.o gccRelease/sys_enginehost.o gccRelease/sys_main.o gccRelease/sys_mainwind.o 
	g++ -fPIC -shared -Wl,-soname,libEqEngine.so -o gccRelease/libEqEngine.so gccRelease/../public/Font.o gccRelease/UFont.o gccRelease/../public/mtriangle_framework.o gccRelease/../public/Utils/GeomTools.o gccRelease/Audio/soundzero.o gccRelease/Audio/alsnd_ambient.o gccRelease/Audio/alsnd_emitter.o gccRelease/Audio/alsnd_sample.o gccRelease/Audio/alsound_local.o gccRelease/Audio/EqSoundSystemA.o gccRelease/KeyBinding/Keys.o gccRelease/../FileSystem/cfgloader.o gccRelease/EqGameLevel.o gccRelease/CEngineGame.o gccRelease/../public/GlobalVarsBase.o gccRelease/EntityFactory.o gccRelease/../public/Math/math_util.o gccRelease/studio_egf.o gccRelease/modelloader_shared.o gccRelease/Network/NETThread.o gccRelease/../shared_engine/physics/BulletShapeCache.o gccRelease/Physics/DkBulletObject.o gccRelease/Physics/DkBulletPhysics.o gccRelease/Physics/DkPhysicsJoint.o gccRelease/Physics/DkPhysicsRope.o gccRelease/../public/BaseRenderableObject.o gccRelease/../public/Decals.o gccRelease/../public/ViewParams.o gccRelease/BaseViewRenderer.o gccRelease/ViewRenderer.o gccRelease/DebugOverlay.o gccRelease/EngineSpew.o gccRelease/EngineVersion.o gccRelease/Modules.o gccRelease/sys_console.o gccRelease/sys_enginehost.o gccRelease/sys_main.o gccRelease/sys_mainwind.o  $(Release_Implicitly_Linked_Objects)

# Compiles file ../public/Font.cpp for the Release configuration...
-include gccRelease/../public/Font.d
gccRelease/../public/Font.o: ../public/Font.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Font.cpp $(Release_Include_Path) -o gccRelease/../public/Font.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Font.cpp $(Release_Include_Path) > gccRelease/../public/Font.d

# Compiles file UFont.cpp for the Release configuration...
-include gccRelease/UFont.d
gccRelease/UFont.o: UFont.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c UFont.cpp $(Release_Include_Path) -o gccRelease/UFont.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM UFont.cpp $(Release_Include_Path) > gccRelease/UFont.d

# Compiles file ../public/mtriangle_framework.cpp for the Release configuration...
-include gccRelease/../public/mtriangle_framework.d
gccRelease/../public/mtriangle_framework.o: ../public/mtriangle_framework.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/mtriangle_framework.cpp $(Release_Include_Path) -o gccRelease/../public/mtriangle_framework.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/mtriangle_framework.cpp $(Release_Include_Path) > gccRelease/../public/mtriangle_framework.d

# Compiles file ../public/Utils/GeomTools.cpp for the Release configuration...
-include gccRelease/../public/Utils/GeomTools.d
gccRelease/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(Release_Include_Path) -o gccRelease/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(Release_Include_Path) > gccRelease/../public/Utils/GeomTools.d

# Compiles file Audio/soundzero.cpp for the Release configuration...
-include gccRelease/Audio/soundzero.d
gccRelease/Audio/soundzero.o: Audio/soundzero.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Audio/soundzero.cpp $(Release_Include_Path) -o gccRelease/Audio/soundzero.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Audio/soundzero.cpp $(Release_Include_Path) > gccRelease/Audio/soundzero.d

# Compiles file Audio/alsnd_ambient.cpp for the Release configuration...
-include gccRelease/Audio/alsnd_ambient.d
gccRelease/Audio/alsnd_ambient.o: Audio/alsnd_ambient.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Audio/alsnd_ambient.cpp $(Release_Include_Path) -o gccRelease/Audio/alsnd_ambient.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Audio/alsnd_ambient.cpp $(Release_Include_Path) > gccRelease/Audio/alsnd_ambient.d

# Compiles file Audio/alsnd_emitter.cpp for the Release configuration...
-include gccRelease/Audio/alsnd_emitter.d
gccRelease/Audio/alsnd_emitter.o: Audio/alsnd_emitter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Audio/alsnd_emitter.cpp $(Release_Include_Path) -o gccRelease/Audio/alsnd_emitter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Audio/alsnd_emitter.cpp $(Release_Include_Path) > gccRelease/Audio/alsnd_emitter.d

# Compiles file Audio/alsnd_sample.cpp for the Release configuration...
-include gccRelease/Audio/alsnd_sample.d
gccRelease/Audio/alsnd_sample.o: Audio/alsnd_sample.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Audio/alsnd_sample.cpp $(Release_Include_Path) -o gccRelease/Audio/alsnd_sample.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Audio/alsnd_sample.cpp $(Release_Include_Path) > gccRelease/Audio/alsnd_sample.d

# Compiles file Audio/alsound_local.cpp for the Release configuration...
-include gccRelease/Audio/alsound_local.d
gccRelease/Audio/alsound_local.o: Audio/alsound_local.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Audio/alsound_local.cpp $(Release_Include_Path) -o gccRelease/Audio/alsound_local.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Audio/alsound_local.cpp $(Release_Include_Path) > gccRelease/Audio/alsound_local.d

# Compiles file Audio/EqSoundSystemA.cpp for the Release configuration...
-include gccRelease/Audio/EqSoundSystemA.d
gccRelease/Audio/EqSoundSystemA.o: Audio/EqSoundSystemA.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Audio/EqSoundSystemA.cpp $(Release_Include_Path) -o gccRelease/Audio/EqSoundSystemA.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Audio/EqSoundSystemA.cpp $(Release_Include_Path) > gccRelease/Audio/EqSoundSystemA.d

# Compiles file KeyBinding/Keys.cpp for the Release configuration...
-include gccRelease/KeyBinding/Keys.d
gccRelease/KeyBinding/Keys.o: KeyBinding/Keys.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c KeyBinding/Keys.cpp $(Release_Include_Path) -o gccRelease/KeyBinding/Keys.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM KeyBinding/Keys.cpp $(Release_Include_Path) > gccRelease/KeyBinding/Keys.d

# Compiles file ../FileSystem/cfgloader.cpp for the Release configuration...
-include gccRelease/../FileSystem/cfgloader.d
gccRelease/../FileSystem/cfgloader.o: ../FileSystem/cfgloader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../FileSystem/cfgloader.cpp $(Release_Include_Path) -o gccRelease/../FileSystem/cfgloader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../FileSystem/cfgloader.cpp $(Release_Include_Path) > gccRelease/../FileSystem/cfgloader.d

# Compiles file EqGameLevel.cpp for the Release configuration...
-include gccRelease/EqGameLevel.d
gccRelease/EqGameLevel.o: EqGameLevel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EqGameLevel.cpp $(Release_Include_Path) -o gccRelease/EqGameLevel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EqGameLevel.cpp $(Release_Include_Path) > gccRelease/EqGameLevel.d

# Compiles file CEngineGame.cpp for the Release configuration...
-include gccRelease/CEngineGame.d
gccRelease/CEngineGame.o: CEngineGame.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CEngineGame.cpp $(Release_Include_Path) -o gccRelease/CEngineGame.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CEngineGame.cpp $(Release_Include_Path) > gccRelease/CEngineGame.d

# Compiles file ../public/GlobalVarsBase.cpp for the Release configuration...
-include gccRelease/../public/GlobalVarsBase.d
gccRelease/../public/GlobalVarsBase.o: ../public/GlobalVarsBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/GlobalVarsBase.cpp $(Release_Include_Path) -o gccRelease/../public/GlobalVarsBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/GlobalVarsBase.cpp $(Release_Include_Path) > gccRelease/../public/GlobalVarsBase.d

# Compiles file EntityFactory.cpp for the Release configuration...
-include gccRelease/EntityFactory.d
gccRelease/EntityFactory.o: EntityFactory.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityFactory.cpp $(Release_Include_Path) -o gccRelease/EntityFactory.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityFactory.cpp $(Release_Include_Path) > gccRelease/EntityFactory.d

# Compiles file ../public/Math/math_util.cpp for the Release configuration...
-include gccRelease/../public/Math/math_util.d
gccRelease/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Math/math_util.cpp $(Release_Include_Path) -o gccRelease/../public/Math/math_util.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Math/math_util.cpp $(Release_Include_Path) > gccRelease/../public/Math/math_util.d

# Compiles file studio_egf.cpp for the Release configuration...
-include gccRelease/studio_egf.d
gccRelease/studio_egf.o: studio_egf.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c studio_egf.cpp $(Release_Include_Path) -o gccRelease/studio_egf.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM studio_egf.cpp $(Release_Include_Path) > gccRelease/studio_egf.d

# Compiles file modelloader_shared.cpp for the Release configuration...
-include gccRelease/modelloader_shared.d
gccRelease/modelloader_shared.o: modelloader_shared.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c modelloader_shared.cpp $(Release_Include_Path) -o gccRelease/modelloader_shared.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM modelloader_shared.cpp $(Release_Include_Path) > gccRelease/modelloader_shared.d

# Compiles file Network/NETThread.cpp for the Release configuration...
-include gccRelease/Network/NETThread.d
gccRelease/Network/NETThread.o: Network/NETThread.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Network/NETThread.cpp $(Release_Include_Path) -o gccRelease/Network/NETThread.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Network/NETThread.cpp $(Release_Include_Path) > gccRelease/Network/NETThread.d

# Compiles file ../shared_engine/physics/BulletShapeCache.cpp for the Release configuration...
-include gccRelease/../shared_engine/physics/BulletShapeCache.d
gccRelease/../shared_engine/physics/BulletShapeCache.o: ../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) -o gccRelease/../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) > gccRelease/../shared_engine/physics/BulletShapeCache.d

# Compiles file Physics/DkBulletObject.cpp for the Release configuration...
-include gccRelease/Physics/DkBulletObject.d
gccRelease/Physics/DkBulletObject.o: Physics/DkBulletObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Physics/DkBulletObject.cpp $(Release_Include_Path) -o gccRelease/Physics/DkBulletObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Physics/DkBulletObject.cpp $(Release_Include_Path) > gccRelease/Physics/DkBulletObject.d

# Compiles file Physics/DkBulletPhysics.cpp for the Release configuration...
-include gccRelease/Physics/DkBulletPhysics.d
gccRelease/Physics/DkBulletPhysics.o: Physics/DkBulletPhysics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Physics/DkBulletPhysics.cpp $(Release_Include_Path) -o gccRelease/Physics/DkBulletPhysics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Physics/DkBulletPhysics.cpp $(Release_Include_Path) > gccRelease/Physics/DkBulletPhysics.d

# Compiles file Physics/DkPhysicsJoint.cpp for the Release configuration...
-include gccRelease/Physics/DkPhysicsJoint.d
gccRelease/Physics/DkPhysicsJoint.o: Physics/DkPhysicsJoint.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Physics/DkPhysicsJoint.cpp $(Release_Include_Path) -o gccRelease/Physics/DkPhysicsJoint.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Physics/DkPhysicsJoint.cpp $(Release_Include_Path) > gccRelease/Physics/DkPhysicsJoint.d

# Compiles file Physics/DkPhysicsRope.cpp for the Release configuration...
-include gccRelease/Physics/DkPhysicsRope.d
gccRelease/Physics/DkPhysicsRope.o: Physics/DkPhysicsRope.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Physics/DkPhysicsRope.cpp $(Release_Include_Path) -o gccRelease/Physics/DkPhysicsRope.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Physics/DkPhysicsRope.cpp $(Release_Include_Path) > gccRelease/Physics/DkPhysicsRope.d

# Compiles file ../public/BaseRenderableObject.cpp for the Release configuration...
-include gccRelease/../public/BaseRenderableObject.d
gccRelease/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(Release_Include_Path) -o gccRelease/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(Release_Include_Path) > gccRelease/../public/BaseRenderableObject.d

# Compiles file ../public/Decals.cpp for the Release configuration...
-include gccRelease/../public/Decals.d
gccRelease/../public/Decals.o: ../public/Decals.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Decals.cpp $(Release_Include_Path) -o gccRelease/../public/Decals.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Decals.cpp $(Release_Include_Path) > gccRelease/../public/Decals.d

# Compiles file ../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../public/ViewParams.d
gccRelease/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../public/ViewParams.d

# Compiles file BaseViewRenderer.cpp for the Release configuration...
-include gccRelease/BaseViewRenderer.d
gccRelease/BaseViewRenderer.o: BaseViewRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseViewRenderer.cpp $(Release_Include_Path) -o gccRelease/BaseViewRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseViewRenderer.cpp $(Release_Include_Path) > gccRelease/BaseViewRenderer.d

# Compiles file ViewRenderer.cpp for the Release configuration...
-include gccRelease/ViewRenderer.d
gccRelease/ViewRenderer.o: ViewRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ViewRenderer.cpp $(Release_Include_Path) -o gccRelease/ViewRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ViewRenderer.cpp $(Release_Include_Path) > gccRelease/ViewRenderer.d

# Compiles file DebugOverlay.cpp for the Release configuration...
-include gccRelease/DebugOverlay.d
gccRelease/DebugOverlay.o: DebugOverlay.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DebugOverlay.cpp $(Release_Include_Path) -o gccRelease/DebugOverlay.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DebugOverlay.cpp $(Release_Include_Path) > gccRelease/DebugOverlay.d

# Compiles file EngineSpew.cpp for the Release configuration...
-include gccRelease/EngineSpew.d
gccRelease/EngineSpew.o: EngineSpew.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EngineSpew.cpp $(Release_Include_Path) -o gccRelease/EngineSpew.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EngineSpew.cpp $(Release_Include_Path) > gccRelease/EngineSpew.d

# Compiles file EngineVersion.cpp for the Release configuration...
-include gccRelease/EngineVersion.d
gccRelease/EngineVersion.o: EngineVersion.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EngineVersion.cpp $(Release_Include_Path) -o gccRelease/EngineVersion.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EngineVersion.cpp $(Release_Include_Path) > gccRelease/EngineVersion.d

# Compiles file Modules.cpp for the Release configuration...
-include gccRelease/Modules.d
gccRelease/Modules.o: Modules.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Modules.cpp $(Release_Include_Path) -o gccRelease/Modules.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Modules.cpp $(Release_Include_Path) > gccRelease/Modules.d

# Compiles file sys_console.cpp for the Release configuration...
-include gccRelease/sys_console.d
gccRelease/sys_console.o: sys_console.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c sys_console.cpp $(Release_Include_Path) -o gccRelease/sys_console.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM sys_console.cpp $(Release_Include_Path) > gccRelease/sys_console.d

# Compiles file sys_enginehost.cpp for the Release configuration...
-include gccRelease/sys_enginehost.d
gccRelease/sys_enginehost.o: sys_enginehost.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c sys_enginehost.cpp $(Release_Include_Path) -o gccRelease/sys_enginehost.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM sys_enginehost.cpp $(Release_Include_Path) > gccRelease/sys_enginehost.d

# Compiles file sys_main.cpp for the Release configuration...
-include gccRelease/sys_main.d
gccRelease/sys_main.o: sys_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c sys_main.cpp $(Release_Include_Path) -o gccRelease/sys_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM sys_main.cpp $(Release_Include_Path) > gccRelease/sys_main.d

# Compiles file sys_mainwind.cpp for the Release configuration...
-include gccRelease/sys_mainwind.d
gccRelease/sys_mainwind.o: sys_mainwind.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c sys_mainwind.cpp $(Release_Include_Path) -o gccRelease/sys_mainwind.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM sys_mainwind.cpp $(Release_Include_Path) > gccRelease/sys_mainwind.d

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

