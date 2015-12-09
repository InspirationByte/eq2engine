# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../public/platform" -I"../../src_dependency/wx/include" -I"../../src_dependency/wx/include/msvc" -I"../../src_dependency/wx/include/msw" -I"../../src_dependency/bullet/src" -I"../../src_dependency/bullet/src/BulletSoftBody" -I"../eq_physgen" -I"../../src_dependency/sdl2/include" 
Release_Include_Path=-I"../../FileSystem" -I"../../Engine" -I"../../public" -I"../../public/core" -I"../../shared_engine" -I"../../public/platform" -I"../../src_dependency/wx/include" -I"../../src_dependency/wx/include/msvc" -I"../../src_dependency/wx/include/msw" -I"../../src_dependency/bullet/src" -I"../../src_dependency/bullet/src/BulletSoftBody" -I"../eq_physgen" -I"../../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=-L"../wx/lib/gccvc_lib" 
Release_Library_Path=-L"../../src_dependency/wx/lib/gccvc_lib" 

# Additional libraries...
Debug_Libraries=-Wl,--start-group -luser32 -lcomctl32 -lwinmm  -Wl,--end-group
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D NO_GAME 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D NO_GAME 

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
Debug: create_folders gccDebug/../../public/Math/math_util.o gccDebug/../../public/Utils/GeomTools.o gccDebug/../../public/anim_activity.o gccDebug/../../public/anim_events.o gccDebug/../../public/BoneSetup.o gccDebug/../../public/ragdoll.o gccDebug/CAnimatedModel.o gccDebug/EgfMan.o gccDebug/../../Engine/Physics/DkBulletObject.o gccDebug/../../Engine/Physics/dkbulletphysics.o gccDebug/../../Engine/Physics/DkPhysicsJoint.o gccDebug/../../Engine/Physics/DkPhysicsRope.o gccDebug/../../shared_engine/physics/BulletShapeCache.o gccDebug/../../public/Font.o gccDebug/../../public/ViewParams.o gccDebug/../../public/RenderList.o gccDebug/../../public/BaseRenderableObject.o gccDebug/../../Engine/DebugOverlay.o gccDebug/../../Engine/studio_egf.o gccDebug/../../Engine/modelloader_shared.o 
	g++ gccDebug/../../public/Math/math_util.o gccDebug/../../public/Utils/GeomTools.o gccDebug/../../public/anim_activity.o gccDebug/../../public/anim_events.o gccDebug/../../public/BoneSetup.o gccDebug/../../public/ragdoll.o gccDebug/CAnimatedModel.o gccDebug/EgfMan.o gccDebug/../../Engine/Physics/DkBulletObject.o gccDebug/../../Engine/Physics/dkbulletphysics.o gccDebug/../../Engine/Physics/DkPhysicsJoint.o gccDebug/../../Engine/Physics/DkPhysicsRope.o gccDebug/../../shared_engine/physics/BulletShapeCache.o gccDebug/../../public/Font.o gccDebug/../../public/ViewParams.o gccDebug/../../public/RenderList.o gccDebug/../../public/BaseRenderableObject.o gccDebug/../../Engine/DebugOverlay.o gccDebug/../../Engine/studio_egf.o gccDebug/../../Engine/modelloader_shared.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/egfman.exe

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

# Compiles file ../../public/anim_activity.cpp for the Debug configuration...
-include gccDebug/../../public/anim_activity.d
gccDebug/../../public/anim_activity.o: ../../public/anim_activity.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/anim_activity.cpp $(Debug_Include_Path) -o gccDebug/../../public/anim_activity.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/anim_activity.cpp $(Debug_Include_Path) > gccDebug/../../public/anim_activity.d

# Compiles file ../../public/anim_events.cpp for the Debug configuration...
-include gccDebug/../../public/anim_events.d
gccDebug/../../public/anim_events.o: ../../public/anim_events.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/anim_events.cpp $(Debug_Include_Path) -o gccDebug/../../public/anim_events.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/anim_events.cpp $(Debug_Include_Path) > gccDebug/../../public/anim_events.d

# Compiles file ../../public/BoneSetup.cpp for the Debug configuration...
-include gccDebug/../../public/BoneSetup.d
gccDebug/../../public/BoneSetup.o: ../../public/BoneSetup.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/BoneSetup.cpp $(Debug_Include_Path) -o gccDebug/../../public/BoneSetup.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/BoneSetup.cpp $(Debug_Include_Path) > gccDebug/../../public/BoneSetup.d

# Compiles file ../../public/ragdoll.cpp for the Debug configuration...
-include gccDebug/../../public/ragdoll.d
gccDebug/../../public/ragdoll.o: ../../public/ragdoll.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ragdoll.cpp $(Debug_Include_Path) -o gccDebug/../../public/ragdoll.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ragdoll.cpp $(Debug_Include_Path) > gccDebug/../../public/ragdoll.d

# Compiles file CAnimatedModel.cpp for the Debug configuration...
-include gccDebug/CAnimatedModel.d
gccDebug/CAnimatedModel.o: CAnimatedModel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c CAnimatedModel.cpp $(Debug_Include_Path) -o gccDebug/CAnimatedModel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM CAnimatedModel.cpp $(Debug_Include_Path) > gccDebug/CAnimatedModel.d

# Compiles file EgfMan.cpp for the Debug configuration...
-include gccDebug/EgfMan.d
gccDebug/EgfMan.o: EgfMan.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EgfMan.cpp $(Debug_Include_Path) -o gccDebug/EgfMan.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EgfMan.cpp $(Debug_Include_Path) > gccDebug/EgfMan.d

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

# Compiles file ../../shared_engine/physics/BulletShapeCache.cpp for the Debug configuration...
-include gccDebug/../../shared_engine/physics/BulletShapeCache.d
gccDebug/../../shared_engine/physics/BulletShapeCache.o: ../../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) -o gccDebug/../../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) > gccDebug/../../shared_engine/physics/BulletShapeCache.d

# Compiles file ../../public/Font.cpp for the Debug configuration...
-include gccDebug/../../public/Font.d
gccDebug/../../public/Font.o: ../../public/Font.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/Font.cpp $(Debug_Include_Path) -o gccDebug/../../public/Font.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/Font.cpp $(Debug_Include_Path) > gccDebug/../../public/Font.d

# Compiles file ../../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../../public/ViewParams.d
gccDebug/../../public/ViewParams.o: ../../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../../public/ViewParams.d

# Compiles file ../../public/RenderList.cpp for the Debug configuration...
-include gccDebug/../../public/RenderList.d
gccDebug/../../public/RenderList.o: ../../public/RenderList.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/RenderList.cpp $(Debug_Include_Path) -o gccDebug/../../public/RenderList.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/RenderList.cpp $(Debug_Include_Path) > gccDebug/../../public/RenderList.d

# Compiles file ../../public/BaseRenderableObject.cpp for the Debug configuration...
-include gccDebug/../../public/BaseRenderableObject.d
gccDebug/../../public/BaseRenderableObject.o: ../../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../public/BaseRenderableObject.cpp $(Debug_Include_Path) -o gccDebug/../../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../public/BaseRenderableObject.cpp $(Debug_Include_Path) > gccDebug/../../public/BaseRenderableObject.d

# Compiles file ../../Engine/DebugOverlay.cpp for the Debug configuration...
-include gccDebug/../../Engine/DebugOverlay.d
gccDebug/../../Engine/DebugOverlay.o: ../../Engine/DebugOverlay.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/DebugOverlay.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/DebugOverlay.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/DebugOverlay.cpp $(Debug_Include_Path) > gccDebug/../../Engine/DebugOverlay.d

# Compiles file ../../Engine/studio_egf.cpp for the Debug configuration...
-include gccDebug/../../Engine/studio_egf.d
gccDebug/../../Engine/studio_egf.o: ../../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/studio_egf.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/studio_egf.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/studio_egf.cpp $(Debug_Include_Path) > gccDebug/../../Engine/studio_egf.d

# Compiles file ../../Engine/modelloader_shared.cpp for the Debug configuration...
-include gccDebug/../../Engine/modelloader_shared.d
gccDebug/../../Engine/modelloader_shared.o: ../../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../../Engine/modelloader_shared.cpp $(Debug_Include_Path) -o gccDebug/../../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../../Engine/modelloader_shared.cpp $(Debug_Include_Path) > gccDebug/../../Engine/modelloader_shared.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../../public/Math/math_util.o gccRelease/../../public/Utils/GeomTools.o gccRelease/../../public/anim_activity.o gccRelease/../../public/anim_events.o gccRelease/../../public/BoneSetup.o gccRelease/../../public/ragdoll.o gccRelease/CAnimatedModel.o gccRelease/EgfMan.o gccRelease/../../Engine/Physics/DkBulletObject.o gccRelease/../../Engine/Physics/dkbulletphysics.o gccRelease/../../Engine/Physics/DkPhysicsJoint.o gccRelease/../../Engine/Physics/DkPhysicsRope.o gccRelease/../../shared_engine/physics/BulletShapeCache.o gccRelease/../../public/Font.o gccRelease/../../public/ViewParams.o gccRelease/../../public/RenderList.o gccRelease/../../public/BaseRenderableObject.o gccRelease/../../Engine/DebugOverlay.o gccRelease/../../Engine/studio_egf.o gccRelease/../../Engine/modelloader_shared.o 
	g++ gccRelease/../../public/Math/math_util.o gccRelease/../../public/Utils/GeomTools.o gccRelease/../../public/anim_activity.o gccRelease/../../public/anim_events.o gccRelease/../../public/BoneSetup.o gccRelease/../../public/ragdoll.o gccRelease/CAnimatedModel.o gccRelease/EgfMan.o gccRelease/../../Engine/Physics/DkBulletObject.o gccRelease/../../Engine/Physics/dkbulletphysics.o gccRelease/../../Engine/Physics/DkPhysicsJoint.o gccRelease/../../Engine/Physics/DkPhysicsRope.o gccRelease/../../shared_engine/physics/BulletShapeCache.o gccRelease/../../public/Font.o gccRelease/../../public/ViewParams.o gccRelease/../../public/RenderList.o gccRelease/../../public/BaseRenderableObject.o gccRelease/../../Engine/DebugOverlay.o gccRelease/../../Engine/studio_egf.o gccRelease/../../Engine/modelloader_shared.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/egfman.exe

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

# Compiles file ../../public/anim_activity.cpp for the Release configuration...
-include gccRelease/../../public/anim_activity.d
gccRelease/../../public/anim_activity.o: ../../public/anim_activity.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/anim_activity.cpp $(Release_Include_Path) -o gccRelease/../../public/anim_activity.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/anim_activity.cpp $(Release_Include_Path) > gccRelease/../../public/anim_activity.d

# Compiles file ../../public/anim_events.cpp for the Release configuration...
-include gccRelease/../../public/anim_events.d
gccRelease/../../public/anim_events.o: ../../public/anim_events.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/anim_events.cpp $(Release_Include_Path) -o gccRelease/../../public/anim_events.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/anim_events.cpp $(Release_Include_Path) > gccRelease/../../public/anim_events.d

# Compiles file ../../public/BoneSetup.cpp for the Release configuration...
-include gccRelease/../../public/BoneSetup.d
gccRelease/../../public/BoneSetup.o: ../../public/BoneSetup.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/BoneSetup.cpp $(Release_Include_Path) -o gccRelease/../../public/BoneSetup.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/BoneSetup.cpp $(Release_Include_Path) > gccRelease/../../public/BoneSetup.d

# Compiles file ../../public/ragdoll.cpp for the Release configuration...
-include gccRelease/../../public/ragdoll.d
gccRelease/../../public/ragdoll.o: ../../public/ragdoll.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ragdoll.cpp $(Release_Include_Path) -o gccRelease/../../public/ragdoll.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ragdoll.cpp $(Release_Include_Path) > gccRelease/../../public/ragdoll.d

# Compiles file CAnimatedModel.cpp for the Release configuration...
-include gccRelease/CAnimatedModel.d
gccRelease/CAnimatedModel.o: CAnimatedModel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c CAnimatedModel.cpp $(Release_Include_Path) -o gccRelease/CAnimatedModel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM CAnimatedModel.cpp $(Release_Include_Path) > gccRelease/CAnimatedModel.d

# Compiles file EgfMan.cpp for the Release configuration...
-include gccRelease/EgfMan.d
gccRelease/EgfMan.o: EgfMan.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EgfMan.cpp $(Release_Include_Path) -o gccRelease/EgfMan.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EgfMan.cpp $(Release_Include_Path) > gccRelease/EgfMan.d

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

# Compiles file ../../shared_engine/physics/BulletShapeCache.cpp for the Release configuration...
-include gccRelease/../../shared_engine/physics/BulletShapeCache.d
gccRelease/../../shared_engine/physics/BulletShapeCache.o: ../../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) -o gccRelease/../../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) > gccRelease/../../shared_engine/physics/BulletShapeCache.d

# Compiles file ../../public/Font.cpp for the Release configuration...
-include gccRelease/../../public/Font.d
gccRelease/../../public/Font.o: ../../public/Font.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/Font.cpp $(Release_Include_Path) -o gccRelease/../../public/Font.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/Font.cpp $(Release_Include_Path) > gccRelease/../../public/Font.d

# Compiles file ../../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../../public/ViewParams.d
gccRelease/../../public/ViewParams.o: ../../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../../public/ViewParams.d

# Compiles file ../../public/RenderList.cpp for the Release configuration...
-include gccRelease/../../public/RenderList.d
gccRelease/../../public/RenderList.o: ../../public/RenderList.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/RenderList.cpp $(Release_Include_Path) -o gccRelease/../../public/RenderList.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/RenderList.cpp $(Release_Include_Path) > gccRelease/../../public/RenderList.d

# Compiles file ../../public/BaseRenderableObject.cpp for the Release configuration...
-include gccRelease/../../public/BaseRenderableObject.d
gccRelease/../../public/BaseRenderableObject.o: ../../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../public/BaseRenderableObject.cpp $(Release_Include_Path) -o gccRelease/../../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../public/BaseRenderableObject.cpp $(Release_Include_Path) > gccRelease/../../public/BaseRenderableObject.d

# Compiles file ../../Engine/DebugOverlay.cpp for the Release configuration...
-include gccRelease/../../Engine/DebugOverlay.d
gccRelease/../../Engine/DebugOverlay.o: ../../Engine/DebugOverlay.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/DebugOverlay.cpp $(Release_Include_Path) -o gccRelease/../../Engine/DebugOverlay.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/DebugOverlay.cpp $(Release_Include_Path) > gccRelease/../../Engine/DebugOverlay.d

# Compiles file ../../Engine/studio_egf.cpp for the Release configuration...
-include gccRelease/../../Engine/studio_egf.d
gccRelease/../../Engine/studio_egf.o: ../../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/studio_egf.cpp $(Release_Include_Path) -o gccRelease/../../Engine/studio_egf.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/studio_egf.cpp $(Release_Include_Path) > gccRelease/../../Engine/studio_egf.d

# Compiles file ../../Engine/modelloader_shared.cpp for the Release configuration...
-include gccRelease/../../Engine/modelloader_shared.d
gccRelease/../../Engine/modelloader_shared.o: ../../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../../Engine/modelloader_shared.cpp $(Release_Include_Path) -o gccRelease/../../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../../Engine/modelloader_shared.cpp $(Release_Include_Path) > gccRelease/../../Engine/modelloader_shared.d

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

