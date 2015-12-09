# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/platform" -I"../utils/wx/include" -I"../utils/wx/include/msvc" -I"../utils/wx/include/msw" -I"../DriversGame" -I"../DriversGame/TA" -I"../Game" 
Release_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../src_dependency/wx/include" -I"../src_dependency/wx/include/msvc" -I"../src_dependency/wx/include/msw" -I"../DriversGame" -I"../src_dependency/bullet/src" -I"../Game" -I"../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=-L"../utils/wx/lib/gccvc_lib" 
Release_Library_Path=-L"../src_dependency/wx/lib/gccvc_lib" 

# Additional libraries...
Debug_Libraries=-Wl,--start-group -luser32 -lcomctl32 -lwinmm  -Wl,--end-group
Release_Libraries=

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D NO_GAME -D NO_ENGINE -D NOENGINE 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D NO_GAME -D NO_ENGINE -D NOENGINE -D EDITOR -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-Wall -O0 -g 
Release_Compiler_Flags=-Wall -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../public/Math/math_util.o gccDebug/../public/Utils/GeomTools.o gccDebug/../public/dsm_esm_loader.o gccDebug/../public/dsm_loader.o gccDebug/../public/dsm_obj_loader.o gccDebug/../public/Font.o gccDebug/../public/ViewParams.o gccDebug/../Engine/DebugOverlay.o gccDebug/../Engine/studio_egf.o gccDebug/../Engine/modelloader_shared.o gccDebug/../Engine/Audio/alsnd_ambient.o gccDebug/../Engine/Audio/alsnd_emitter.o gccDebug/../Engine/Audio/alsnd_sample.o gccDebug/../Engine/Audio/alsound_local.o gccDebug/../Engine/Audio/soundzero.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/../DriversGame/eqPhysics/eqBulletIndexedMesh.o gccDebug/../DriversGame/eqPhysics/eqCollision_Object.o gccDebug/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o gccDebug/../DriversGame/eqPhysics/eqPhysics.o gccDebug/../DriversGame/eqPhysics/eqPhysics_Body.o gccDebug/../DriversGame/physics.o gccDebug/../shared_engine/physics/BulletShapeCache.o gccDebug/../Game/EffectRender.o gccDebug/../Game/EqParticles_Render.o gccDebug/../public/TextureAtlas.o gccDebug/../DriversGame/car.o gccDebug/../DriversGame/GameObject.o gccDebug/../DriversGame/object_debris.o gccDebug/../DriversGame/object_physics.o gccDebug/BaseTilebasedEditor.o gccDebug/EditorMain.o gccDebug/LoadLevDialog.o gccDebug/UI_HeightEdit.o gccDebug/UI_LevelModels.o gccDebug/../DriversGame/heightfield.o gccDebug/../DriversGame/level.o gccDebug/../DriversGame/world.o 
	g++ gccDebug/../public/Math/math_util.o gccDebug/../public/Utils/GeomTools.o gccDebug/../public/dsm_esm_loader.o gccDebug/../public/dsm_loader.o gccDebug/../public/dsm_obj_loader.o gccDebug/../public/Font.o gccDebug/../public/ViewParams.o gccDebug/../Engine/DebugOverlay.o gccDebug/../Engine/studio_egf.o gccDebug/../Engine/modelloader_shared.o gccDebug/../Engine/Audio/alsnd_ambient.o gccDebug/../Engine/Audio/alsnd_emitter.o gccDebug/../Engine/Audio/alsnd_sample.o gccDebug/../Engine/Audio/alsound_local.o gccDebug/../Engine/Audio/soundzero.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/../DriversGame/eqPhysics/eqBulletIndexedMesh.o gccDebug/../DriversGame/eqPhysics/eqCollision_Object.o gccDebug/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o gccDebug/../DriversGame/eqPhysics/eqPhysics.o gccDebug/../DriversGame/eqPhysics/eqPhysics_Body.o gccDebug/../DriversGame/physics.o gccDebug/../shared_engine/physics/BulletShapeCache.o gccDebug/../Game/EffectRender.o gccDebug/../Game/EqParticles_Render.o gccDebug/../public/TextureAtlas.o gccDebug/../DriversGame/car.o gccDebug/../DriversGame/GameObject.o gccDebug/../DriversGame/object_debris.o gccDebug/../DriversGame/object_physics.o gccDebug/BaseTilebasedEditor.o gccDebug/EditorMain.o gccDebug/LoadLevDialog.o gccDebug/UI_HeightEdit.o gccDebug/UI_LevelModels.o gccDebug/../DriversGame/heightfield.o gccDebug/../DriversGame/level.o gccDebug/../DriversGame/world.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/DriversEditor.exe

# Compiles file ../public/Math/math_util.cpp for the Debug configuration...
-include gccDebug/../public/Math/math_util.d
gccDebug/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Math/math_util.cpp $(Debug_Include_Path) -o gccDebug/../public/Math/math_util.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Math/math_util.cpp $(Debug_Include_Path) > gccDebug/../public/Math/math_util.d

# Compiles file ../public/Utils/GeomTools.cpp for the Debug configuration...
-include gccDebug/../public/Utils/GeomTools.d
gccDebug/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(Debug_Include_Path) -o gccDebug/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(Debug_Include_Path) > gccDebug/../public/Utils/GeomTools.d

# Compiles file ../public/dsm_esm_loader.cpp for the Debug configuration...
-include gccDebug/../public/dsm_esm_loader.d
gccDebug/../public/dsm_esm_loader.o: ../public/dsm_esm_loader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/dsm_esm_loader.cpp $(Debug_Include_Path) -o gccDebug/../public/dsm_esm_loader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/dsm_esm_loader.cpp $(Debug_Include_Path) > gccDebug/../public/dsm_esm_loader.d

# Compiles file ../public/dsm_loader.cpp for the Debug configuration...
-include gccDebug/../public/dsm_loader.d
gccDebug/../public/dsm_loader.o: ../public/dsm_loader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/dsm_loader.cpp $(Debug_Include_Path) -o gccDebug/../public/dsm_loader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/dsm_loader.cpp $(Debug_Include_Path) > gccDebug/../public/dsm_loader.d

# Compiles file ../public/dsm_obj_loader.cpp for the Debug configuration...
-include gccDebug/../public/dsm_obj_loader.d
gccDebug/../public/dsm_obj_loader.o: ../public/dsm_obj_loader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/dsm_obj_loader.cpp $(Debug_Include_Path) -o gccDebug/../public/dsm_obj_loader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/dsm_obj_loader.cpp $(Debug_Include_Path) > gccDebug/../public/dsm_obj_loader.d

# Compiles file ../public/Font.cpp for the Debug configuration...
-include gccDebug/../public/Font.d
gccDebug/../public/Font.o: ../public/Font.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Font.cpp $(Debug_Include_Path) -o gccDebug/../public/Font.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Font.cpp $(Debug_Include_Path) > gccDebug/../public/Font.d

# Compiles file ../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../public/ViewParams.d
gccDebug/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../public/ViewParams.d

# Compiles file ../Engine/DebugOverlay.cpp for the Debug configuration...
-include gccDebug/../Engine/DebugOverlay.d
gccDebug/../Engine/DebugOverlay.o: ../Engine/DebugOverlay.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/DebugOverlay.cpp $(Debug_Include_Path) -o gccDebug/../Engine/DebugOverlay.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/DebugOverlay.cpp $(Debug_Include_Path) > gccDebug/../Engine/DebugOverlay.d

# Compiles file ../Engine/studio_egf.cpp for the Debug configuration...
-include gccDebug/../Engine/studio_egf.d
gccDebug/../Engine/studio_egf.o: ../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/studio_egf.cpp $(Debug_Include_Path) -o gccDebug/../Engine/studio_egf.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/studio_egf.cpp $(Debug_Include_Path) > gccDebug/../Engine/studio_egf.d

# Compiles file ../Engine/modelloader_shared.cpp for the Debug configuration...
-include gccDebug/../Engine/modelloader_shared.d
gccDebug/../Engine/modelloader_shared.o: ../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/modelloader_shared.cpp $(Debug_Include_Path) -o gccDebug/../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/modelloader_shared.cpp $(Debug_Include_Path) > gccDebug/../Engine/modelloader_shared.d

# Compiles file ../Engine/Audio/alsnd_ambient.cpp for the Debug configuration...
-include gccDebug/../Engine/Audio/alsnd_ambient.d
gccDebug/../Engine/Audio/alsnd_ambient.o: ../Engine/Audio/alsnd_ambient.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Audio/alsnd_ambient.cpp $(Debug_Include_Path) -o gccDebug/../Engine/Audio/alsnd_ambient.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Audio/alsnd_ambient.cpp $(Debug_Include_Path) > gccDebug/../Engine/Audio/alsnd_ambient.d

# Compiles file ../Engine/Audio/alsnd_emitter.cpp for the Debug configuration...
-include gccDebug/../Engine/Audio/alsnd_emitter.d
gccDebug/../Engine/Audio/alsnd_emitter.o: ../Engine/Audio/alsnd_emitter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Audio/alsnd_emitter.cpp $(Debug_Include_Path) -o gccDebug/../Engine/Audio/alsnd_emitter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Audio/alsnd_emitter.cpp $(Debug_Include_Path) > gccDebug/../Engine/Audio/alsnd_emitter.d

# Compiles file ../Engine/Audio/alsnd_sample.cpp for the Debug configuration...
-include gccDebug/../Engine/Audio/alsnd_sample.d
gccDebug/../Engine/Audio/alsnd_sample.o: ../Engine/Audio/alsnd_sample.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Audio/alsnd_sample.cpp $(Debug_Include_Path) -o gccDebug/../Engine/Audio/alsnd_sample.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Audio/alsnd_sample.cpp $(Debug_Include_Path) > gccDebug/../Engine/Audio/alsnd_sample.d

# Compiles file ../Engine/Audio/alsound_local.cpp for the Debug configuration...
-include gccDebug/../Engine/Audio/alsound_local.d
gccDebug/../Engine/Audio/alsound_local.o: ../Engine/Audio/alsound_local.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Audio/alsound_local.cpp $(Debug_Include_Path) -o gccDebug/../Engine/Audio/alsound_local.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Audio/alsound_local.cpp $(Debug_Include_Path) > gccDebug/../Engine/Audio/alsound_local.d

# Compiles file ../Engine/Audio/soundzero.cpp for the Debug configuration...
-include gccDebug/../Engine/Audio/soundzero.d
gccDebug/../Engine/Audio/soundzero.o: ../Engine/Audio/soundzero.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Audio/soundzero.cpp $(Debug_Include_Path) -o gccDebug/../Engine/Audio/soundzero.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Audio/soundzero.cpp $(Debug_Include_Path) > gccDebug/../Engine/Audio/soundzero.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the Debug configuration...
-include gccDebug/../public/GameSoundEmitterSystem.d
gccDebug/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(Debug_Include_Path) -o gccDebug/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(Debug_Include_Path) > gccDebug/../public/GameSoundEmitterSystem.d

# Compiles file ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp for the Debug configuration...
-include gccDebug/../DriversGame/eqPhysics/eqBulletIndexedMesh.d
gccDebug/../DriversGame/eqPhysics/eqBulletIndexedMesh.o: ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/eqPhysics/eqBulletIndexedMesh.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/eqPhysics/eqBulletIndexedMesh.d

# Compiles file ../DriversGame/eqPhysics/eqCollision_Object.cpp for the Debug configuration...
-include gccDebug/../DriversGame/eqPhysics/eqCollision_Object.d
gccDebug/../DriversGame/eqPhysics/eqCollision_Object.o: ../DriversGame/eqPhysics/eqCollision_Object.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/eqPhysics/eqCollision_Object.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/eqPhysics/eqCollision_Object.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqCollision_Object.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/eqPhysics/eqCollision_Object.d

# Compiles file ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp for the Debug configuration...
-include gccDebug/../DriversGame/eqPhysics/eqCollision_ObjectGrid.d
gccDebug/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o: ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/eqPhysics/eqCollision_ObjectGrid.d

# Compiles file ../DriversGame/eqPhysics/eqPhysics.cpp for the Debug configuration...
-include gccDebug/../DriversGame/eqPhysics/eqPhysics.d
gccDebug/../DriversGame/eqPhysics/eqPhysics.o: ../DriversGame/eqPhysics/eqPhysics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/eqPhysics/eqPhysics.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/eqPhysics/eqPhysics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqPhysics.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/eqPhysics/eqPhysics.d

# Compiles file ../DriversGame/eqPhysics/eqPhysics_Body.cpp for the Debug configuration...
-include gccDebug/../DriversGame/eqPhysics/eqPhysics_Body.d
gccDebug/../DriversGame/eqPhysics/eqPhysics_Body.o: ../DriversGame/eqPhysics/eqPhysics_Body.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/eqPhysics/eqPhysics_Body.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/eqPhysics/eqPhysics_Body.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqPhysics_Body.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/eqPhysics/eqPhysics_Body.d

# Compiles file ../DriversGame/physics.cpp for the Debug configuration...
-include gccDebug/../DriversGame/physics.d
gccDebug/../DriversGame/physics.o: ../DriversGame/physics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/physics.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/physics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/physics.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/physics.d

# Compiles file ../shared_engine/physics/BulletShapeCache.cpp for the Debug configuration...
-include gccDebug/../shared_engine/physics/BulletShapeCache.d
gccDebug/../shared_engine/physics/BulletShapeCache.o: ../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) -o gccDebug/../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) > gccDebug/../shared_engine/physics/BulletShapeCache.d

# Compiles file ../Game/EffectRender.cpp for the Debug configuration...
-include gccDebug/../Game/EffectRender.d
gccDebug/../Game/EffectRender.o: ../Game/EffectRender.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Game/EffectRender.cpp $(Debug_Include_Path) -o gccDebug/../Game/EffectRender.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Game/EffectRender.cpp $(Debug_Include_Path) > gccDebug/../Game/EffectRender.d

# Compiles file ../Game/EqParticles_Render.cpp for the Debug configuration...
-include gccDebug/../Game/EqParticles_Render.d
gccDebug/../Game/EqParticles_Render.o: ../Game/EqParticles_Render.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Game/EqParticles_Render.cpp $(Debug_Include_Path) -o gccDebug/../Game/EqParticles_Render.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Game/EqParticles_Render.cpp $(Debug_Include_Path) > gccDebug/../Game/EqParticles_Render.d

# Compiles file ../public/TextureAtlas.cpp for the Debug configuration...
-include gccDebug/../public/TextureAtlas.d
gccDebug/../public/TextureAtlas.o: ../public/TextureAtlas.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/TextureAtlas.cpp $(Debug_Include_Path) -o gccDebug/../public/TextureAtlas.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/TextureAtlas.cpp $(Debug_Include_Path) > gccDebug/../public/TextureAtlas.d

# Compiles file ../DriversGame/car.cpp for the Debug configuration...
-include gccDebug/../DriversGame/car.d
gccDebug/../DriversGame/car.o: ../DriversGame/car.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/car.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/car.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/car.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/car.d

# Compiles file ../DriversGame/GameObject.cpp for the Debug configuration...
-include gccDebug/../DriversGame/GameObject.d
gccDebug/../DriversGame/GameObject.o: ../DriversGame/GameObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/GameObject.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/GameObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/GameObject.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/GameObject.d

# Compiles file ../DriversGame/object_debris.cpp for the Debug configuration...
-include gccDebug/../DriversGame/object_debris.d
gccDebug/../DriversGame/object_debris.o: ../DriversGame/object_debris.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/object_debris.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/object_debris.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/object_debris.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/object_debris.d

# Compiles file ../DriversGame/object_physics.cpp for the Debug configuration...
-include gccDebug/../DriversGame/object_physics.d
gccDebug/../DriversGame/object_physics.o: ../DriversGame/object_physics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/object_physics.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/object_physics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/object_physics.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/object_physics.d

# Compiles file BaseTilebasedEditor.cpp for the Debug configuration...
-include gccDebug/BaseTilebasedEditor.d
gccDebug/BaseTilebasedEditor.o: BaseTilebasedEditor.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseTilebasedEditor.cpp $(Debug_Include_Path) -o gccDebug/BaseTilebasedEditor.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseTilebasedEditor.cpp $(Debug_Include_Path) > gccDebug/BaseTilebasedEditor.d

# Compiles file EditorMain.cpp for the Debug configuration...
-include gccDebug/EditorMain.d
gccDebug/EditorMain.o: EditorMain.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditorMain.cpp $(Debug_Include_Path) -o gccDebug/EditorMain.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditorMain.cpp $(Debug_Include_Path) > gccDebug/EditorMain.d

# Compiles file LoadLevDialog.cpp for the Debug configuration...
-include gccDebug/LoadLevDialog.d
gccDebug/LoadLevDialog.o: LoadLevDialog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c LoadLevDialog.cpp $(Debug_Include_Path) -o gccDebug/LoadLevDialog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM LoadLevDialog.cpp $(Debug_Include_Path) > gccDebug/LoadLevDialog.d

# Compiles file UI_HeightEdit.cpp for the Debug configuration...
-include gccDebug/UI_HeightEdit.d
gccDebug/UI_HeightEdit.o: UI_HeightEdit.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c UI_HeightEdit.cpp $(Debug_Include_Path) -o gccDebug/UI_HeightEdit.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM UI_HeightEdit.cpp $(Debug_Include_Path) > gccDebug/UI_HeightEdit.d

# Compiles file UI_LevelModels.cpp for the Debug configuration...
-include gccDebug/UI_LevelModels.d
gccDebug/UI_LevelModels.o: UI_LevelModels.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c UI_LevelModels.cpp $(Debug_Include_Path) -o gccDebug/UI_LevelModels.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM UI_LevelModels.cpp $(Debug_Include_Path) > gccDebug/UI_LevelModels.d

# Compiles file ../DriversGame/heightfield.cpp for the Debug configuration...
-include gccDebug/../DriversGame/heightfield.d
gccDebug/../DriversGame/heightfield.o: ../DriversGame/heightfield.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/heightfield.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/heightfield.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/heightfield.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/heightfield.d

# Compiles file ../DriversGame/level.cpp for the Debug configuration...
-include gccDebug/../DriversGame/level.d
gccDebug/../DriversGame/level.o: ../DriversGame/level.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/level.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/level.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/level.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/level.d

# Compiles file ../DriversGame/world.cpp for the Debug configuration...
-include gccDebug/../DriversGame/world.d
gccDebug/../DriversGame/world.o: ../DriversGame/world.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../DriversGame/world.cpp $(Debug_Include_Path) -o gccDebug/../DriversGame/world.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../DriversGame/world.cpp $(Debug_Include_Path) > gccDebug/../DriversGame/world.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../public/Math/math_util.o gccRelease/../public/Utils/GeomTools.o gccRelease/../public/dsm_esm_loader.o gccRelease/../public/dsm_loader.o gccRelease/../public/dsm_obj_loader.o gccRelease/../public/Font.o gccRelease/../public/ViewParams.o gccRelease/../Engine/DebugOverlay.o gccRelease/../Engine/studio_egf.o gccRelease/../Engine/modelloader_shared.o gccRelease/../Engine/Audio/alsnd_ambient.o gccRelease/../Engine/Audio/alsnd_emitter.o gccRelease/../Engine/Audio/alsnd_sample.o gccRelease/../Engine/Audio/alsound_local.o gccRelease/../Engine/Audio/soundzero.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/../DriversGame/eqPhysics/eqBulletIndexedMesh.o gccRelease/../DriversGame/eqPhysics/eqCollision_Object.o gccRelease/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o gccRelease/../DriversGame/eqPhysics/eqPhysics.o gccRelease/../DriversGame/eqPhysics/eqPhysics_Body.o gccRelease/../DriversGame/physics.o gccRelease/../shared_engine/physics/BulletShapeCache.o gccRelease/../Game/EffectRender.o gccRelease/../Game/EqParticles_Render.o gccRelease/../public/TextureAtlas.o gccRelease/../DriversGame/car.o gccRelease/../DriversGame/GameObject.o gccRelease/../DriversGame/object_debris.o gccRelease/../DriversGame/object_physics.o gccRelease/BaseTilebasedEditor.o gccRelease/EditorMain.o gccRelease/LoadLevDialog.o gccRelease/UI_HeightEdit.o gccRelease/UI_LevelModels.o gccRelease/../DriversGame/heightfield.o gccRelease/../DriversGame/level.o gccRelease/../DriversGame/world.o 
	g++ gccRelease/../public/Math/math_util.o gccRelease/../public/Utils/GeomTools.o gccRelease/../public/dsm_esm_loader.o gccRelease/../public/dsm_loader.o gccRelease/../public/dsm_obj_loader.o gccRelease/../public/Font.o gccRelease/../public/ViewParams.o gccRelease/../Engine/DebugOverlay.o gccRelease/../Engine/studio_egf.o gccRelease/../Engine/modelloader_shared.o gccRelease/../Engine/Audio/alsnd_ambient.o gccRelease/../Engine/Audio/alsnd_emitter.o gccRelease/../Engine/Audio/alsnd_sample.o gccRelease/../Engine/Audio/alsound_local.o gccRelease/../Engine/Audio/soundzero.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/../DriversGame/eqPhysics/eqBulletIndexedMesh.o gccRelease/../DriversGame/eqPhysics/eqCollision_Object.o gccRelease/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o gccRelease/../DriversGame/eqPhysics/eqPhysics.o gccRelease/../DriversGame/eqPhysics/eqPhysics_Body.o gccRelease/../DriversGame/physics.o gccRelease/../shared_engine/physics/BulletShapeCache.o gccRelease/../Game/EffectRender.o gccRelease/../Game/EqParticles_Render.o gccRelease/../public/TextureAtlas.o gccRelease/../DriversGame/car.o gccRelease/../DriversGame/GameObject.o gccRelease/../DriversGame/object_debris.o gccRelease/../DriversGame/object_physics.o gccRelease/BaseTilebasedEditor.o gccRelease/EditorMain.o gccRelease/LoadLevDialog.o gccRelease/UI_HeightEdit.o gccRelease/UI_LevelModels.o gccRelease/../DriversGame/heightfield.o gccRelease/../DriversGame/level.o gccRelease/../DriversGame/world.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/DriversEditor.exe

# Compiles file ../public/Math/math_util.cpp for the Release configuration...
-include gccRelease/../public/Math/math_util.d
gccRelease/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Math/math_util.cpp $(Release_Include_Path) -o gccRelease/../public/Math/math_util.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Math/math_util.cpp $(Release_Include_Path) > gccRelease/../public/Math/math_util.d

# Compiles file ../public/Utils/GeomTools.cpp for the Release configuration...
-include gccRelease/../public/Utils/GeomTools.d
gccRelease/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(Release_Include_Path) -o gccRelease/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(Release_Include_Path) > gccRelease/../public/Utils/GeomTools.d

# Compiles file ../public/dsm_esm_loader.cpp for the Release configuration...
-include gccRelease/../public/dsm_esm_loader.d
gccRelease/../public/dsm_esm_loader.o: ../public/dsm_esm_loader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/dsm_esm_loader.cpp $(Release_Include_Path) -o gccRelease/../public/dsm_esm_loader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/dsm_esm_loader.cpp $(Release_Include_Path) > gccRelease/../public/dsm_esm_loader.d

# Compiles file ../public/dsm_loader.cpp for the Release configuration...
-include gccRelease/../public/dsm_loader.d
gccRelease/../public/dsm_loader.o: ../public/dsm_loader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/dsm_loader.cpp $(Release_Include_Path) -o gccRelease/../public/dsm_loader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/dsm_loader.cpp $(Release_Include_Path) > gccRelease/../public/dsm_loader.d

# Compiles file ../public/dsm_obj_loader.cpp for the Release configuration...
-include gccRelease/../public/dsm_obj_loader.d
gccRelease/../public/dsm_obj_loader.o: ../public/dsm_obj_loader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/dsm_obj_loader.cpp $(Release_Include_Path) -o gccRelease/../public/dsm_obj_loader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/dsm_obj_loader.cpp $(Release_Include_Path) > gccRelease/../public/dsm_obj_loader.d

# Compiles file ../public/Font.cpp for the Release configuration...
-include gccRelease/../public/Font.d
gccRelease/../public/Font.o: ../public/Font.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Font.cpp $(Release_Include_Path) -o gccRelease/../public/Font.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Font.cpp $(Release_Include_Path) > gccRelease/../public/Font.d

# Compiles file ../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../public/ViewParams.d
gccRelease/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../public/ViewParams.d

# Compiles file ../Engine/DebugOverlay.cpp for the Release configuration...
-include gccRelease/../Engine/DebugOverlay.d
gccRelease/../Engine/DebugOverlay.o: ../Engine/DebugOverlay.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/DebugOverlay.cpp $(Release_Include_Path) -o gccRelease/../Engine/DebugOverlay.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/DebugOverlay.cpp $(Release_Include_Path) > gccRelease/../Engine/DebugOverlay.d

# Compiles file ../Engine/studio_egf.cpp for the Release configuration...
-include gccRelease/../Engine/studio_egf.d
gccRelease/../Engine/studio_egf.o: ../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/studio_egf.cpp $(Release_Include_Path) -o gccRelease/../Engine/studio_egf.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/studio_egf.cpp $(Release_Include_Path) > gccRelease/../Engine/studio_egf.d

# Compiles file ../Engine/modelloader_shared.cpp for the Release configuration...
-include gccRelease/../Engine/modelloader_shared.d
gccRelease/../Engine/modelloader_shared.o: ../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/modelloader_shared.cpp $(Release_Include_Path) -o gccRelease/../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/modelloader_shared.cpp $(Release_Include_Path) > gccRelease/../Engine/modelloader_shared.d

# Compiles file ../Engine/Audio/alsnd_ambient.cpp for the Release configuration...
-include gccRelease/../Engine/Audio/alsnd_ambient.d
gccRelease/../Engine/Audio/alsnd_ambient.o: ../Engine/Audio/alsnd_ambient.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Audio/alsnd_ambient.cpp $(Release_Include_Path) -o gccRelease/../Engine/Audio/alsnd_ambient.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Audio/alsnd_ambient.cpp $(Release_Include_Path) > gccRelease/../Engine/Audio/alsnd_ambient.d

# Compiles file ../Engine/Audio/alsnd_emitter.cpp for the Release configuration...
-include gccRelease/../Engine/Audio/alsnd_emitter.d
gccRelease/../Engine/Audio/alsnd_emitter.o: ../Engine/Audio/alsnd_emitter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Audio/alsnd_emitter.cpp $(Release_Include_Path) -o gccRelease/../Engine/Audio/alsnd_emitter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Audio/alsnd_emitter.cpp $(Release_Include_Path) > gccRelease/../Engine/Audio/alsnd_emitter.d

# Compiles file ../Engine/Audio/alsnd_sample.cpp for the Release configuration...
-include gccRelease/../Engine/Audio/alsnd_sample.d
gccRelease/../Engine/Audio/alsnd_sample.o: ../Engine/Audio/alsnd_sample.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Audio/alsnd_sample.cpp $(Release_Include_Path) -o gccRelease/../Engine/Audio/alsnd_sample.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Audio/alsnd_sample.cpp $(Release_Include_Path) > gccRelease/../Engine/Audio/alsnd_sample.d

# Compiles file ../Engine/Audio/alsound_local.cpp for the Release configuration...
-include gccRelease/../Engine/Audio/alsound_local.d
gccRelease/../Engine/Audio/alsound_local.o: ../Engine/Audio/alsound_local.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Audio/alsound_local.cpp $(Release_Include_Path) -o gccRelease/../Engine/Audio/alsound_local.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Audio/alsound_local.cpp $(Release_Include_Path) > gccRelease/../Engine/Audio/alsound_local.d

# Compiles file ../Engine/Audio/soundzero.cpp for the Release configuration...
-include gccRelease/../Engine/Audio/soundzero.d
gccRelease/../Engine/Audio/soundzero.o: ../Engine/Audio/soundzero.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Audio/soundzero.cpp $(Release_Include_Path) -o gccRelease/../Engine/Audio/soundzero.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Audio/soundzero.cpp $(Release_Include_Path) > gccRelease/../Engine/Audio/soundzero.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the Release configuration...
-include gccRelease/../public/GameSoundEmitterSystem.d
gccRelease/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(Release_Include_Path) -o gccRelease/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(Release_Include_Path) > gccRelease/../public/GameSoundEmitterSystem.d

# Compiles file ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp for the Release configuration...
-include gccRelease/../DriversGame/eqPhysics/eqBulletIndexedMesh.d
gccRelease/../DriversGame/eqPhysics/eqBulletIndexedMesh.o: ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/eqPhysics/eqBulletIndexedMesh.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqBulletIndexedMesh.cpp $(Release_Include_Path) > gccRelease/../DriversGame/eqPhysics/eqBulletIndexedMesh.d

# Compiles file ../DriversGame/eqPhysics/eqCollision_Object.cpp for the Release configuration...
-include gccRelease/../DriversGame/eqPhysics/eqCollision_Object.d
gccRelease/../DriversGame/eqPhysics/eqCollision_Object.o: ../DriversGame/eqPhysics/eqCollision_Object.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/eqPhysics/eqCollision_Object.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/eqPhysics/eqCollision_Object.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqCollision_Object.cpp $(Release_Include_Path) > gccRelease/../DriversGame/eqPhysics/eqCollision_Object.d

# Compiles file ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp for the Release configuration...
-include gccRelease/../DriversGame/eqPhysics/eqCollision_ObjectGrid.d
gccRelease/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o: ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/eqPhysics/eqCollision_ObjectGrid.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp $(Release_Include_Path) > gccRelease/../DriversGame/eqPhysics/eqCollision_ObjectGrid.d

# Compiles file ../DriversGame/eqPhysics/eqPhysics.cpp for the Release configuration...
-include gccRelease/../DriversGame/eqPhysics/eqPhysics.d
gccRelease/../DriversGame/eqPhysics/eqPhysics.o: ../DriversGame/eqPhysics/eqPhysics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/eqPhysics/eqPhysics.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/eqPhysics/eqPhysics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqPhysics.cpp $(Release_Include_Path) > gccRelease/../DriversGame/eqPhysics/eqPhysics.d

# Compiles file ../DriversGame/eqPhysics/eqPhysics_Body.cpp for the Release configuration...
-include gccRelease/../DriversGame/eqPhysics/eqPhysics_Body.d
gccRelease/../DriversGame/eqPhysics/eqPhysics_Body.o: ../DriversGame/eqPhysics/eqPhysics_Body.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/eqPhysics/eqPhysics_Body.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/eqPhysics/eqPhysics_Body.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/eqPhysics/eqPhysics_Body.cpp $(Release_Include_Path) > gccRelease/../DriversGame/eqPhysics/eqPhysics_Body.d

# Compiles file ../DriversGame/physics.cpp for the Release configuration...
-include gccRelease/../DriversGame/physics.d
gccRelease/../DriversGame/physics.o: ../DriversGame/physics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/physics.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/physics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/physics.cpp $(Release_Include_Path) > gccRelease/../DriversGame/physics.d

# Compiles file ../shared_engine/physics/BulletShapeCache.cpp for the Release configuration...
-include gccRelease/../shared_engine/physics/BulletShapeCache.d
gccRelease/../shared_engine/physics/BulletShapeCache.o: ../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) -o gccRelease/../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) > gccRelease/../shared_engine/physics/BulletShapeCache.d

# Compiles file ../Game/EffectRender.cpp for the Release configuration...
-include gccRelease/../Game/EffectRender.d
gccRelease/../Game/EffectRender.o: ../Game/EffectRender.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Game/EffectRender.cpp $(Release_Include_Path) -o gccRelease/../Game/EffectRender.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Game/EffectRender.cpp $(Release_Include_Path) > gccRelease/../Game/EffectRender.d

# Compiles file ../Game/EqParticles_Render.cpp for the Release configuration...
-include gccRelease/../Game/EqParticles_Render.d
gccRelease/../Game/EqParticles_Render.o: ../Game/EqParticles_Render.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Game/EqParticles_Render.cpp $(Release_Include_Path) -o gccRelease/../Game/EqParticles_Render.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Game/EqParticles_Render.cpp $(Release_Include_Path) > gccRelease/../Game/EqParticles_Render.d

# Compiles file ../public/TextureAtlas.cpp for the Release configuration...
-include gccRelease/../public/TextureAtlas.d
gccRelease/../public/TextureAtlas.o: ../public/TextureAtlas.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/TextureAtlas.cpp $(Release_Include_Path) -o gccRelease/../public/TextureAtlas.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/TextureAtlas.cpp $(Release_Include_Path) > gccRelease/../public/TextureAtlas.d

# Compiles file ../DriversGame/car.cpp for the Release configuration...
-include gccRelease/../DriversGame/car.d
gccRelease/../DriversGame/car.o: ../DriversGame/car.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/car.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/car.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/car.cpp $(Release_Include_Path) > gccRelease/../DriversGame/car.d

# Compiles file ../DriversGame/GameObject.cpp for the Release configuration...
-include gccRelease/../DriversGame/GameObject.d
gccRelease/../DriversGame/GameObject.o: ../DriversGame/GameObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/GameObject.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/GameObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/GameObject.cpp $(Release_Include_Path) > gccRelease/../DriversGame/GameObject.d

# Compiles file ../DriversGame/object_debris.cpp for the Release configuration...
-include gccRelease/../DriversGame/object_debris.d
gccRelease/../DriversGame/object_debris.o: ../DriversGame/object_debris.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/object_debris.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/object_debris.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/object_debris.cpp $(Release_Include_Path) > gccRelease/../DriversGame/object_debris.d

# Compiles file ../DriversGame/object_physics.cpp for the Release configuration...
-include gccRelease/../DriversGame/object_physics.d
gccRelease/../DriversGame/object_physics.o: ../DriversGame/object_physics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/object_physics.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/object_physics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/object_physics.cpp $(Release_Include_Path) > gccRelease/../DriversGame/object_physics.d

# Compiles file BaseTilebasedEditor.cpp for the Release configuration...
-include gccRelease/BaseTilebasedEditor.d
gccRelease/BaseTilebasedEditor.o: BaseTilebasedEditor.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseTilebasedEditor.cpp $(Release_Include_Path) -o gccRelease/BaseTilebasedEditor.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseTilebasedEditor.cpp $(Release_Include_Path) > gccRelease/BaseTilebasedEditor.d

# Compiles file EditorMain.cpp for the Release configuration...
-include gccRelease/EditorMain.d
gccRelease/EditorMain.o: EditorMain.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditorMain.cpp $(Release_Include_Path) -o gccRelease/EditorMain.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditorMain.cpp $(Release_Include_Path) > gccRelease/EditorMain.d

# Compiles file LoadLevDialog.cpp for the Release configuration...
-include gccRelease/LoadLevDialog.d
gccRelease/LoadLevDialog.o: LoadLevDialog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c LoadLevDialog.cpp $(Release_Include_Path) -o gccRelease/LoadLevDialog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM LoadLevDialog.cpp $(Release_Include_Path) > gccRelease/LoadLevDialog.d

# Compiles file UI_HeightEdit.cpp for the Release configuration...
-include gccRelease/UI_HeightEdit.d
gccRelease/UI_HeightEdit.o: UI_HeightEdit.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c UI_HeightEdit.cpp $(Release_Include_Path) -o gccRelease/UI_HeightEdit.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM UI_HeightEdit.cpp $(Release_Include_Path) > gccRelease/UI_HeightEdit.d

# Compiles file UI_LevelModels.cpp for the Release configuration...
-include gccRelease/UI_LevelModels.d
gccRelease/UI_LevelModels.o: UI_LevelModels.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c UI_LevelModels.cpp $(Release_Include_Path) -o gccRelease/UI_LevelModels.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM UI_LevelModels.cpp $(Release_Include_Path) > gccRelease/UI_LevelModels.d

# Compiles file ../DriversGame/heightfield.cpp for the Release configuration...
-include gccRelease/../DriversGame/heightfield.d
gccRelease/../DriversGame/heightfield.o: ../DriversGame/heightfield.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/heightfield.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/heightfield.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/heightfield.cpp $(Release_Include_Path) > gccRelease/../DriversGame/heightfield.d

# Compiles file ../DriversGame/level.cpp for the Release configuration...
-include gccRelease/../DriversGame/level.d
gccRelease/../DriversGame/level.o: ../DriversGame/level.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/level.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/level.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/level.cpp $(Release_Include_Path) > gccRelease/../DriversGame/level.d

# Compiles file ../DriversGame/world.cpp for the Release configuration...
-include gccRelease/../DriversGame/world.d
gccRelease/../DriversGame/world.o: ../DriversGame/world.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../DriversGame/world.cpp $(Release_Include_Path) -o gccRelease/../DriversGame/world.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../DriversGame/world.cpp $(Release_Include_Path) > gccRelease/../DriversGame/world.d

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

