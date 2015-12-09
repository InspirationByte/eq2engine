# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../Game" -I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../public/materialsystem/renderers" -I"../Physics/bullet/src" -I"../Physics/bullet/src/BulletSoftBody" -I"../utils/eq_physgen" -I"../utils/wx/include" -I"../utils/wx/include/msvc" -I"../src_dependency/sdl2/include" -I"../src_dependency/bullet/src" 
Release_Include_Path=-I"../Game" -I"../FileSystem" -I"../Engine" -I"../public" -I"../public/core" -I"../public/platform" -I"../public/materialsystem/renderers" -I"../Physics/bullet/src" -I"../Physics/bullet/src/BulletSoftBody" -I"../utils/eq_physgen" -I"../utils/wx/include" -I"../utils/wx/include/msvc" -I"../src_dependency/sdl2/include" -I"../src_dependency/bullet/src" 

# Library paths...
Debug_Library_Path=
Release_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -lwsock32  -Wl,--end-group
Release_Libraries=-Wl,--start-group -lwsock32  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D NO_ENGINE -D NOENGINE -D GAME_DRIVERS -D EQ_USE_SDL 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D NO_ENGINE -D NOENGINE -D GAME_DRIVERS -D EQ_USE_SDL 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-O0 -g 
Release_Compiler_Flags=-O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug Release 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/main.o gccDebug/system.o gccDebug/window.o gccDebug/../Engine/Audio/alsnd_ambient.o gccDebug/../Engine/Audio/alsnd_emitter.o gccDebug/../Engine/Audio/alsnd_sample.o gccDebug/../Engine/Audio/alsound_local.o gccDebug/../Engine/Audio/soundzero.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/../public/Math/math_util.o gccDebug/../public/Utils/GeomTools.o gccDebug/../Engine/EngineSpew.o gccDebug/../Engine/sys_console.o gccDebug/replay.o gccDebug/ui_main.o gccDebug/input.o gccDebug/game_net.o gccDebug/NetPlayer.o gccDebug/state_game.o gccDebug/state.o gccDebug/car.o gccDebug/object_debris.o gccDebug/GameObject.o gccDebug/object_physics.o gccDebug/../Engine/Network/NETThread.o gccDebug/eqphysics/eqBulletIndexedMesh.o gccDebug/eqphysics/eqCollision_Object.o gccDebug/eqphysics/eqCollision_ObjectGrid.o gccDebug/eqPhysics/eqPhysics.o gccDebug/eqPhysics/eqPhysics_Body.o gccDebug/physics.o gccDebug/../shared_engine/physics/BulletShapeCache.o gccDebug/heightfield.o gccDebug/level.o gccDebug/world.o gccDebug/../Engine/EngineVersion.o gccDebug/../Engine/KeyBinding/Keys.o gccDebug/../Engine/modelloader_shared.o gccDebug/../Engine/studio_egf.o gccDebug/../FileSystem/cfgloader.o gccDebug/../public/Font.o gccDebug/../public/ViewParams.o gccDebug/../Engine/DebugOverlay.o gccDebug/../Game/EffectRender.o gccDebug/../Game/EqParticles_Render.o gccDebug/../public/TextureAtlas.o gccDebug/../public/BaseShader.o gccDebug/VehicleBodyShader.o 
	g++ gccDebug/main.o gccDebug/system.o gccDebug/window.o gccDebug/../Engine/Audio/alsnd_ambient.o gccDebug/../Engine/Audio/alsnd_emitter.o gccDebug/../Engine/Audio/alsnd_sample.o gccDebug/../Engine/Audio/alsound_local.o gccDebug/../Engine/Audio/soundzero.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/../public/Math/math_util.o gccDebug/../public/Utils/GeomTools.o gccDebug/../Engine/EngineSpew.o gccDebug/../Engine/sys_console.o gccDebug/replay.o gccDebug/ui_main.o gccDebug/input.o gccDebug/game_net.o gccDebug/NetPlayer.o gccDebug/state_game.o gccDebug/state.o gccDebug/car.o gccDebug/object_debris.o gccDebug/GameObject.o gccDebug/object_physics.o gccDebug/../Engine/Network/NETThread.o gccDebug/eqphysics/eqBulletIndexedMesh.o gccDebug/eqphysics/eqCollision_Object.o gccDebug/eqphysics/eqCollision_ObjectGrid.o gccDebug/eqPhysics/eqPhysics.o gccDebug/eqPhysics/eqPhysics_Body.o gccDebug/physics.o gccDebug/../shared_engine/physics/BulletShapeCache.o gccDebug/heightfield.o gccDebug/level.o gccDebug/world.o gccDebug/../Engine/EngineVersion.o gccDebug/../Engine/KeyBinding/Keys.o gccDebug/../Engine/modelloader_shared.o gccDebug/../Engine/studio_egf.o gccDebug/../FileSystem/cfgloader.o gccDebug/../public/Font.o gccDebug/../public/ViewParams.o gccDebug/../Engine/DebugOverlay.o gccDebug/../Game/EffectRender.o gccDebug/../Game/EqParticles_Render.o gccDebug/../public/TextureAtlas.o gccDebug/../public/BaseShader.o gccDebug/VehicleBodyShader.o  $(Debug_Library_Path) $(Debug_Libraries) -Wl,-rpath,./ -o gccDebug/DriversGame.exe

# Compiles file main.cpp for the Debug configuration...
-include gccDebug/main.d
gccDebug/main.o: main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c main.cpp $(Debug_Include_Path) -o gccDebug/main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM main.cpp $(Debug_Include_Path) > gccDebug/main.d

# Compiles file system.cpp for the Debug configuration...
-include gccDebug/system.d
gccDebug/system.o: system.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c system.cpp $(Debug_Include_Path) -o gccDebug/system.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM system.cpp $(Debug_Include_Path) > gccDebug/system.d

# Compiles file window.cpp for the Debug configuration...
-include gccDebug/window.d
gccDebug/window.o: window.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c window.cpp $(Debug_Include_Path) -o gccDebug/window.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM window.cpp $(Debug_Include_Path) > gccDebug/window.d

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

# Compiles file ../Engine/EngineSpew.cpp for the Debug configuration...
-include gccDebug/../Engine/EngineSpew.d
gccDebug/../Engine/EngineSpew.o: ../Engine/EngineSpew.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/EngineSpew.cpp $(Debug_Include_Path) -o gccDebug/../Engine/EngineSpew.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/EngineSpew.cpp $(Debug_Include_Path) > gccDebug/../Engine/EngineSpew.d

# Compiles file ../Engine/sys_console.cpp for the Debug configuration...
-include gccDebug/../Engine/sys_console.d
gccDebug/../Engine/sys_console.o: ../Engine/sys_console.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/sys_console.cpp $(Debug_Include_Path) -o gccDebug/../Engine/sys_console.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/sys_console.cpp $(Debug_Include_Path) > gccDebug/../Engine/sys_console.d

# Compiles file replay.cpp for the Debug configuration...
-include gccDebug/replay.d
gccDebug/replay.o: replay.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c replay.cpp $(Debug_Include_Path) -o gccDebug/replay.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM replay.cpp $(Debug_Include_Path) > gccDebug/replay.d

# Compiles file ui_main.cpp for the Debug configuration...
-include gccDebug/ui_main.d
gccDebug/ui_main.o: ui_main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ui_main.cpp $(Debug_Include_Path) -o gccDebug/ui_main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ui_main.cpp $(Debug_Include_Path) > gccDebug/ui_main.d

# Compiles file input.cpp for the Debug configuration...
-include gccDebug/input.d
gccDebug/input.o: input.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c input.cpp $(Debug_Include_Path) -o gccDebug/input.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM input.cpp $(Debug_Include_Path) > gccDebug/input.d

# Compiles file game_net.cpp for the Debug configuration...
-include gccDebug/game_net.d
gccDebug/game_net.o: game_net.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c game_net.cpp $(Debug_Include_Path) -o gccDebug/game_net.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM game_net.cpp $(Debug_Include_Path) > gccDebug/game_net.d

# Compiles file NetPlayer.cpp for the Debug configuration...
-include gccDebug/NetPlayer.d
gccDebug/NetPlayer.o: NetPlayer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c NetPlayer.cpp $(Debug_Include_Path) -o gccDebug/NetPlayer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM NetPlayer.cpp $(Debug_Include_Path) > gccDebug/NetPlayer.d

# Compiles file state_game.cpp for the Debug configuration...
-include gccDebug/state_game.d
gccDebug/state_game.o: state_game.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c state_game.cpp $(Debug_Include_Path) -o gccDebug/state_game.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM state_game.cpp $(Debug_Include_Path) > gccDebug/state_game.d

# Compiles file state.cpp for the Debug configuration...
-include gccDebug/state.d
gccDebug/state.o: state.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c state.cpp $(Debug_Include_Path) -o gccDebug/state.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM state.cpp $(Debug_Include_Path) > gccDebug/state.d

# Compiles file car.cpp for the Debug configuration...
-include gccDebug/car.d
gccDebug/car.o: car.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c car.cpp $(Debug_Include_Path) -o gccDebug/car.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM car.cpp $(Debug_Include_Path) > gccDebug/car.d

# Compiles file object_debris.cpp for the Debug configuration...
-include gccDebug/object_debris.d
gccDebug/object_debris.o: object_debris.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c object_debris.cpp $(Debug_Include_Path) -o gccDebug/object_debris.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM object_debris.cpp $(Debug_Include_Path) > gccDebug/object_debris.d

# Compiles file GameObject.cpp for the Debug configuration...
-include gccDebug/GameObject.d
gccDebug/GameObject.o: GameObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameObject.cpp $(Debug_Include_Path) -o gccDebug/GameObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameObject.cpp $(Debug_Include_Path) > gccDebug/GameObject.d

# Compiles file object_physics.cpp for the Debug configuration...
-include gccDebug/object_physics.d
gccDebug/object_physics.o: object_physics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c object_physics.cpp $(Debug_Include_Path) -o gccDebug/object_physics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM object_physics.cpp $(Debug_Include_Path) > gccDebug/object_physics.d

# Compiles file ../Engine/Network/NETThread.cpp for the Debug configuration...
-include gccDebug/../Engine/Network/NETThread.d
gccDebug/../Engine/Network/NETThread.o: ../Engine/Network/NETThread.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/Network/NETThread.cpp $(Debug_Include_Path) -o gccDebug/../Engine/Network/NETThread.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/Network/NETThread.cpp $(Debug_Include_Path) > gccDebug/../Engine/Network/NETThread.d

# Compiles file eqphysics/eqBulletIndexedMesh.cpp for the Debug configuration...
-include gccDebug/eqphysics/eqBulletIndexedMesh.d
gccDebug/eqphysics/eqBulletIndexedMesh.o: eqphysics/eqBulletIndexedMesh.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c eqphysics/eqBulletIndexedMesh.cpp $(Debug_Include_Path) -o gccDebug/eqphysics/eqBulletIndexedMesh.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM eqphysics/eqBulletIndexedMesh.cpp $(Debug_Include_Path) > gccDebug/eqphysics/eqBulletIndexedMesh.d

# Compiles file eqphysics/eqCollision_Object.cpp for the Debug configuration...
-include gccDebug/eqphysics/eqCollision_Object.d
gccDebug/eqphysics/eqCollision_Object.o: eqphysics/eqCollision_Object.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c eqphysics/eqCollision_Object.cpp $(Debug_Include_Path) -o gccDebug/eqphysics/eqCollision_Object.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM eqphysics/eqCollision_Object.cpp $(Debug_Include_Path) > gccDebug/eqphysics/eqCollision_Object.d

# Compiles file eqphysics/eqCollision_ObjectGrid.cpp for the Debug configuration...
-include gccDebug/eqphysics/eqCollision_ObjectGrid.d
gccDebug/eqphysics/eqCollision_ObjectGrid.o: eqphysics/eqCollision_ObjectGrid.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c eqphysics/eqCollision_ObjectGrid.cpp $(Debug_Include_Path) -o gccDebug/eqphysics/eqCollision_ObjectGrid.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM eqphysics/eqCollision_ObjectGrid.cpp $(Debug_Include_Path) > gccDebug/eqphysics/eqCollision_ObjectGrid.d

# Compiles file eqPhysics/eqPhysics.cpp for the Debug configuration...
-include gccDebug/eqPhysics/eqPhysics.d
gccDebug/eqPhysics/eqPhysics.o: eqPhysics/eqPhysics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c eqPhysics/eqPhysics.cpp $(Debug_Include_Path) -o gccDebug/eqPhysics/eqPhysics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM eqPhysics/eqPhysics.cpp $(Debug_Include_Path) > gccDebug/eqPhysics/eqPhysics.d

# Compiles file eqPhysics/eqPhysics_Body.cpp for the Debug configuration...
-include gccDebug/eqPhysics/eqPhysics_Body.d
gccDebug/eqPhysics/eqPhysics_Body.o: eqPhysics/eqPhysics_Body.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c eqPhysics/eqPhysics_Body.cpp $(Debug_Include_Path) -o gccDebug/eqPhysics/eqPhysics_Body.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM eqPhysics/eqPhysics_Body.cpp $(Debug_Include_Path) > gccDebug/eqPhysics/eqPhysics_Body.d

# Compiles file physics.cpp for the Debug configuration...
-include gccDebug/physics.d
gccDebug/physics.o: physics.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c physics.cpp $(Debug_Include_Path) -o gccDebug/physics.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM physics.cpp $(Debug_Include_Path) > gccDebug/physics.d

# Compiles file ../shared_engine/physics/BulletShapeCache.cpp for the Debug configuration...
-include gccDebug/../shared_engine/physics/BulletShapeCache.d
gccDebug/../shared_engine/physics/BulletShapeCache.o: ../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) -o gccDebug/../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../shared_engine/physics/BulletShapeCache.cpp $(Debug_Include_Path) > gccDebug/../shared_engine/physics/BulletShapeCache.d

# Compiles file heightfield.cpp for the Debug configuration...
-include gccDebug/heightfield.d
gccDebug/heightfield.o: heightfield.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c heightfield.cpp $(Debug_Include_Path) -o gccDebug/heightfield.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM heightfield.cpp $(Debug_Include_Path) > gccDebug/heightfield.d

# Compiles file level.cpp for the Debug configuration...
-include gccDebug/level.d
gccDebug/level.o: level.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c level.cpp $(Debug_Include_Path) -o gccDebug/level.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM level.cpp $(Debug_Include_Path) > gccDebug/level.d

# Compiles file world.cpp for the Debug configuration...
-include gccDebug/world.d
gccDebug/world.o: world.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c world.cpp $(Debug_Include_Path) -o gccDebug/world.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM world.cpp $(Debug_Include_Path) > gccDebug/world.d

# Compiles file ../Engine/EngineVersion.cpp for the Debug configuration...
-include gccDebug/../Engine/EngineVersion.d
gccDebug/../Engine/EngineVersion.o: ../Engine/EngineVersion.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/EngineVersion.cpp $(Debug_Include_Path) -o gccDebug/../Engine/EngineVersion.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/EngineVersion.cpp $(Debug_Include_Path) > gccDebug/../Engine/EngineVersion.d

# Compiles file ../Engine/KeyBinding/Keys.cpp for the Debug configuration...
-include gccDebug/../Engine/KeyBinding/Keys.d
gccDebug/../Engine/KeyBinding/Keys.o: ../Engine/KeyBinding/Keys.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/KeyBinding/Keys.cpp $(Debug_Include_Path) -o gccDebug/../Engine/KeyBinding/Keys.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/KeyBinding/Keys.cpp $(Debug_Include_Path) > gccDebug/../Engine/KeyBinding/Keys.d

# Compiles file ../Engine/modelloader_shared.cpp for the Debug configuration...
-include gccDebug/../Engine/modelloader_shared.d
gccDebug/../Engine/modelloader_shared.o: ../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/modelloader_shared.cpp $(Debug_Include_Path) -o gccDebug/../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/modelloader_shared.cpp $(Debug_Include_Path) > gccDebug/../Engine/modelloader_shared.d

# Compiles file ../Engine/studio_egf.cpp for the Debug configuration...
-include gccDebug/../Engine/studio_egf.d
gccDebug/../Engine/studio_egf.o: ../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/studio_egf.cpp $(Debug_Include_Path) -o gccDebug/../Engine/studio_egf.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/studio_egf.cpp $(Debug_Include_Path) > gccDebug/../Engine/studio_egf.d

# Compiles file ../FileSystem/cfgloader.cpp for the Debug configuration...
-include gccDebug/../FileSystem/cfgloader.d
gccDebug/../FileSystem/cfgloader.o: ../FileSystem/cfgloader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../FileSystem/cfgloader.cpp $(Debug_Include_Path) -o gccDebug/../FileSystem/cfgloader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../FileSystem/cfgloader.cpp $(Debug_Include_Path) > gccDebug/../FileSystem/cfgloader.d

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

# Compiles file ../public/BaseShader.cpp for the Debug configuration...
-include gccDebug/../public/BaseShader.d
gccDebug/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BaseShader.cpp $(Debug_Include_Path) -o gccDebug/../public/BaseShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BaseShader.cpp $(Debug_Include_Path) > gccDebug/../public/BaseShader.d

# Compiles file VehicleBodyShader.cpp for the Debug configuration...
-include gccDebug/VehicleBodyShader.d
gccDebug/VehicleBodyShader.o: VehicleBodyShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c VehicleBodyShader.cpp $(Debug_Include_Path) -o gccDebug/VehicleBodyShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM VehicleBodyShader.cpp $(Debug_Include_Path) > gccDebug/VehicleBodyShader.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/main.o gccRelease/system.o gccRelease/window.o gccRelease/../Engine/Audio/alsnd_ambient.o gccRelease/../Engine/Audio/alsnd_emitter.o gccRelease/../Engine/Audio/alsnd_sample.o gccRelease/../Engine/Audio/alsound_local.o gccRelease/../Engine/Audio/soundzero.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/../public/Math/math_util.o gccRelease/../public/Utils/GeomTools.o gccRelease/../Engine/EngineSpew.o gccRelease/../Engine/sys_console.o gccRelease/replay.o gccRelease/ui_main.o gccRelease/input.o gccRelease/game_net.o gccRelease/NetPlayer.o gccRelease/state_game.o gccRelease/state.o gccRelease/car.o gccRelease/object_debris.o gccRelease/GameObject.o gccRelease/object_physics.o gccRelease/../Engine/Network/NETThread.o gccRelease/eqphysics/eqBulletIndexedMesh.o gccRelease/eqphysics/eqCollision_Object.o gccRelease/eqphysics/eqCollision_ObjectGrid.o gccRelease/eqPhysics/eqPhysics.o gccRelease/eqPhysics/eqPhysics_Body.o gccRelease/physics.o gccRelease/../shared_engine/physics/BulletShapeCache.o gccRelease/heightfield.o gccRelease/level.o gccRelease/world.o gccRelease/../Engine/EngineVersion.o gccRelease/../Engine/KeyBinding/Keys.o gccRelease/../Engine/modelloader_shared.o gccRelease/../Engine/studio_egf.o gccRelease/../FileSystem/cfgloader.o gccRelease/../public/Font.o gccRelease/../public/ViewParams.o gccRelease/../Engine/DebugOverlay.o gccRelease/../Game/EffectRender.o gccRelease/../Game/EqParticles_Render.o gccRelease/../public/TextureAtlas.o gccRelease/../public/BaseShader.o gccRelease/VehicleBodyShader.o 
	g++ gccRelease/main.o gccRelease/system.o gccRelease/window.o gccRelease/../Engine/Audio/alsnd_ambient.o gccRelease/../Engine/Audio/alsnd_emitter.o gccRelease/../Engine/Audio/alsnd_sample.o gccRelease/../Engine/Audio/alsound_local.o gccRelease/../Engine/Audio/soundzero.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/../public/Math/math_util.o gccRelease/../public/Utils/GeomTools.o gccRelease/../Engine/EngineSpew.o gccRelease/../Engine/sys_console.o gccRelease/replay.o gccRelease/ui_main.o gccRelease/input.o gccRelease/game_net.o gccRelease/NetPlayer.o gccRelease/state_game.o gccRelease/state.o gccRelease/car.o gccRelease/object_debris.o gccRelease/GameObject.o gccRelease/object_physics.o gccRelease/../Engine/Network/NETThread.o gccRelease/eqphysics/eqBulletIndexedMesh.o gccRelease/eqphysics/eqCollision_Object.o gccRelease/eqphysics/eqCollision_ObjectGrid.o gccRelease/eqPhysics/eqPhysics.o gccRelease/eqPhysics/eqPhysics_Body.o gccRelease/physics.o gccRelease/../shared_engine/physics/BulletShapeCache.o gccRelease/heightfield.o gccRelease/level.o gccRelease/world.o gccRelease/../Engine/EngineVersion.o gccRelease/../Engine/KeyBinding/Keys.o gccRelease/../Engine/modelloader_shared.o gccRelease/../Engine/studio_egf.o gccRelease/../FileSystem/cfgloader.o gccRelease/../public/Font.o gccRelease/../public/ViewParams.o gccRelease/../Engine/DebugOverlay.o gccRelease/../Game/EffectRender.o gccRelease/../Game/EqParticles_Render.o gccRelease/../public/TextureAtlas.o gccRelease/../public/BaseShader.o gccRelease/VehicleBodyShader.o  $(Release_Library_Path) $(Release_Libraries) -Wl,-rpath,./ -o gccRelease/DriversGame.exe

# Compiles file main.cpp for the Release configuration...
-include gccRelease/main.d
gccRelease/main.o: main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c main.cpp $(Release_Include_Path) -o gccRelease/main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM main.cpp $(Release_Include_Path) > gccRelease/main.d

# Compiles file system.cpp for the Release configuration...
-include gccRelease/system.d
gccRelease/system.o: system.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c system.cpp $(Release_Include_Path) -o gccRelease/system.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM system.cpp $(Release_Include_Path) > gccRelease/system.d

# Compiles file window.cpp for the Release configuration...
-include gccRelease/window.d
gccRelease/window.o: window.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c window.cpp $(Release_Include_Path) -o gccRelease/window.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM window.cpp $(Release_Include_Path) > gccRelease/window.d

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

# Compiles file ../Engine/EngineSpew.cpp for the Release configuration...
-include gccRelease/../Engine/EngineSpew.d
gccRelease/../Engine/EngineSpew.o: ../Engine/EngineSpew.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/EngineSpew.cpp $(Release_Include_Path) -o gccRelease/../Engine/EngineSpew.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/EngineSpew.cpp $(Release_Include_Path) > gccRelease/../Engine/EngineSpew.d

# Compiles file ../Engine/sys_console.cpp for the Release configuration...
-include gccRelease/../Engine/sys_console.d
gccRelease/../Engine/sys_console.o: ../Engine/sys_console.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/sys_console.cpp $(Release_Include_Path) -o gccRelease/../Engine/sys_console.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/sys_console.cpp $(Release_Include_Path) > gccRelease/../Engine/sys_console.d

# Compiles file replay.cpp for the Release configuration...
-include gccRelease/replay.d
gccRelease/replay.o: replay.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c replay.cpp $(Release_Include_Path) -o gccRelease/replay.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM replay.cpp $(Release_Include_Path) > gccRelease/replay.d

# Compiles file ui_main.cpp for the Release configuration...
-include gccRelease/ui_main.d
gccRelease/ui_main.o: ui_main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ui_main.cpp $(Release_Include_Path) -o gccRelease/ui_main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ui_main.cpp $(Release_Include_Path) > gccRelease/ui_main.d

# Compiles file input.cpp for the Release configuration...
-include gccRelease/input.d
gccRelease/input.o: input.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c input.cpp $(Release_Include_Path) -o gccRelease/input.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM input.cpp $(Release_Include_Path) > gccRelease/input.d

# Compiles file game_net.cpp for the Release configuration...
-include gccRelease/game_net.d
gccRelease/game_net.o: game_net.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c game_net.cpp $(Release_Include_Path) -o gccRelease/game_net.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM game_net.cpp $(Release_Include_Path) > gccRelease/game_net.d

# Compiles file NetPlayer.cpp for the Release configuration...
-include gccRelease/NetPlayer.d
gccRelease/NetPlayer.o: NetPlayer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c NetPlayer.cpp $(Release_Include_Path) -o gccRelease/NetPlayer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM NetPlayer.cpp $(Release_Include_Path) > gccRelease/NetPlayer.d

# Compiles file state_game.cpp for the Release configuration...
-include gccRelease/state_game.d
gccRelease/state_game.o: state_game.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c state_game.cpp $(Release_Include_Path) -o gccRelease/state_game.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM state_game.cpp $(Release_Include_Path) > gccRelease/state_game.d

# Compiles file state.cpp for the Release configuration...
-include gccRelease/state.d
gccRelease/state.o: state.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c state.cpp $(Release_Include_Path) -o gccRelease/state.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM state.cpp $(Release_Include_Path) > gccRelease/state.d

# Compiles file car.cpp for the Release configuration...
-include gccRelease/car.d
gccRelease/car.o: car.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c car.cpp $(Release_Include_Path) -o gccRelease/car.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM car.cpp $(Release_Include_Path) > gccRelease/car.d

# Compiles file object_debris.cpp for the Release configuration...
-include gccRelease/object_debris.d
gccRelease/object_debris.o: object_debris.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c object_debris.cpp $(Release_Include_Path) -o gccRelease/object_debris.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM object_debris.cpp $(Release_Include_Path) > gccRelease/object_debris.d

# Compiles file GameObject.cpp for the Release configuration...
-include gccRelease/GameObject.d
gccRelease/GameObject.o: GameObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameObject.cpp $(Release_Include_Path) -o gccRelease/GameObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameObject.cpp $(Release_Include_Path) > gccRelease/GameObject.d

# Compiles file object_physics.cpp for the Release configuration...
-include gccRelease/object_physics.d
gccRelease/object_physics.o: object_physics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c object_physics.cpp $(Release_Include_Path) -o gccRelease/object_physics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM object_physics.cpp $(Release_Include_Path) > gccRelease/object_physics.d

# Compiles file ../Engine/Network/NETThread.cpp for the Release configuration...
-include gccRelease/../Engine/Network/NETThread.d
gccRelease/../Engine/Network/NETThread.o: ../Engine/Network/NETThread.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/Network/NETThread.cpp $(Release_Include_Path) -o gccRelease/../Engine/Network/NETThread.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/Network/NETThread.cpp $(Release_Include_Path) > gccRelease/../Engine/Network/NETThread.d

# Compiles file eqphysics/eqBulletIndexedMesh.cpp for the Release configuration...
-include gccRelease/eqphysics/eqBulletIndexedMesh.d
gccRelease/eqphysics/eqBulletIndexedMesh.o: eqphysics/eqBulletIndexedMesh.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c eqphysics/eqBulletIndexedMesh.cpp $(Release_Include_Path) -o gccRelease/eqphysics/eqBulletIndexedMesh.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM eqphysics/eqBulletIndexedMesh.cpp $(Release_Include_Path) > gccRelease/eqphysics/eqBulletIndexedMesh.d

# Compiles file eqphysics/eqCollision_Object.cpp for the Release configuration...
-include gccRelease/eqphysics/eqCollision_Object.d
gccRelease/eqphysics/eqCollision_Object.o: eqphysics/eqCollision_Object.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c eqphysics/eqCollision_Object.cpp $(Release_Include_Path) -o gccRelease/eqphysics/eqCollision_Object.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM eqphysics/eqCollision_Object.cpp $(Release_Include_Path) > gccRelease/eqphysics/eqCollision_Object.d

# Compiles file eqphysics/eqCollision_ObjectGrid.cpp for the Release configuration...
-include gccRelease/eqphysics/eqCollision_ObjectGrid.d
gccRelease/eqphysics/eqCollision_ObjectGrid.o: eqphysics/eqCollision_ObjectGrid.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c eqphysics/eqCollision_ObjectGrid.cpp $(Release_Include_Path) -o gccRelease/eqphysics/eqCollision_ObjectGrid.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM eqphysics/eqCollision_ObjectGrid.cpp $(Release_Include_Path) > gccRelease/eqphysics/eqCollision_ObjectGrid.d

# Compiles file eqPhysics/eqPhysics.cpp for the Release configuration...
-include gccRelease/eqPhysics/eqPhysics.d
gccRelease/eqPhysics/eqPhysics.o: eqPhysics/eqPhysics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c eqPhysics/eqPhysics.cpp $(Release_Include_Path) -o gccRelease/eqPhysics/eqPhysics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM eqPhysics/eqPhysics.cpp $(Release_Include_Path) > gccRelease/eqPhysics/eqPhysics.d

# Compiles file eqPhysics/eqPhysics_Body.cpp for the Release configuration...
-include gccRelease/eqPhysics/eqPhysics_Body.d
gccRelease/eqPhysics/eqPhysics_Body.o: eqPhysics/eqPhysics_Body.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c eqPhysics/eqPhysics_Body.cpp $(Release_Include_Path) -o gccRelease/eqPhysics/eqPhysics_Body.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM eqPhysics/eqPhysics_Body.cpp $(Release_Include_Path) > gccRelease/eqPhysics/eqPhysics_Body.d

# Compiles file physics.cpp for the Release configuration...
-include gccRelease/physics.d
gccRelease/physics.o: physics.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c physics.cpp $(Release_Include_Path) -o gccRelease/physics.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM physics.cpp $(Release_Include_Path) > gccRelease/physics.d

# Compiles file ../shared_engine/physics/BulletShapeCache.cpp for the Release configuration...
-include gccRelease/../shared_engine/physics/BulletShapeCache.d
gccRelease/../shared_engine/physics/BulletShapeCache.o: ../shared_engine/physics/BulletShapeCache.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) -o gccRelease/../shared_engine/physics/BulletShapeCache.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../shared_engine/physics/BulletShapeCache.cpp $(Release_Include_Path) > gccRelease/../shared_engine/physics/BulletShapeCache.d

# Compiles file heightfield.cpp for the Release configuration...
-include gccRelease/heightfield.d
gccRelease/heightfield.o: heightfield.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c heightfield.cpp $(Release_Include_Path) -o gccRelease/heightfield.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM heightfield.cpp $(Release_Include_Path) > gccRelease/heightfield.d

# Compiles file level.cpp for the Release configuration...
-include gccRelease/level.d
gccRelease/level.o: level.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c level.cpp $(Release_Include_Path) -o gccRelease/level.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM level.cpp $(Release_Include_Path) > gccRelease/level.d

# Compiles file world.cpp for the Release configuration...
-include gccRelease/world.d
gccRelease/world.o: world.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c world.cpp $(Release_Include_Path) -o gccRelease/world.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM world.cpp $(Release_Include_Path) > gccRelease/world.d

# Compiles file ../Engine/EngineVersion.cpp for the Release configuration...
-include gccRelease/../Engine/EngineVersion.d
gccRelease/../Engine/EngineVersion.o: ../Engine/EngineVersion.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/EngineVersion.cpp $(Release_Include_Path) -o gccRelease/../Engine/EngineVersion.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/EngineVersion.cpp $(Release_Include_Path) > gccRelease/../Engine/EngineVersion.d

# Compiles file ../Engine/KeyBinding/Keys.cpp for the Release configuration...
-include gccRelease/../Engine/KeyBinding/Keys.d
gccRelease/../Engine/KeyBinding/Keys.o: ../Engine/KeyBinding/Keys.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/KeyBinding/Keys.cpp $(Release_Include_Path) -o gccRelease/../Engine/KeyBinding/Keys.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/KeyBinding/Keys.cpp $(Release_Include_Path) > gccRelease/../Engine/KeyBinding/Keys.d

# Compiles file ../Engine/modelloader_shared.cpp for the Release configuration...
-include gccRelease/../Engine/modelloader_shared.d
gccRelease/../Engine/modelloader_shared.o: ../Engine/modelloader_shared.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/modelloader_shared.cpp $(Release_Include_Path) -o gccRelease/../Engine/modelloader_shared.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/modelloader_shared.cpp $(Release_Include_Path) > gccRelease/../Engine/modelloader_shared.d

# Compiles file ../Engine/studio_egf.cpp for the Release configuration...
-include gccRelease/../Engine/studio_egf.d
gccRelease/../Engine/studio_egf.o: ../Engine/studio_egf.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/studio_egf.cpp $(Release_Include_Path) -o gccRelease/../Engine/studio_egf.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/studio_egf.cpp $(Release_Include_Path) > gccRelease/../Engine/studio_egf.d

# Compiles file ../FileSystem/cfgloader.cpp for the Release configuration...
-include gccRelease/../FileSystem/cfgloader.d
gccRelease/../FileSystem/cfgloader.o: ../FileSystem/cfgloader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../FileSystem/cfgloader.cpp $(Release_Include_Path) -o gccRelease/../FileSystem/cfgloader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../FileSystem/cfgloader.cpp $(Release_Include_Path) > gccRelease/../FileSystem/cfgloader.d

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

# Compiles file ../public/BaseShader.cpp for the Release configuration...
-include gccRelease/../public/BaseShader.d
gccRelease/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BaseShader.cpp $(Release_Include_Path) -o gccRelease/../public/BaseShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BaseShader.cpp $(Release_Include_Path) > gccRelease/../public/BaseShader.d

# Compiles file VehicleBodyShader.cpp for the Release configuration...
-include gccRelease/VehicleBodyShader.d
gccRelease/VehicleBodyShader.o: VehicleBodyShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c VehicleBodyShader.cpp $(Release_Include_Path) -o gccRelease/VehicleBodyShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM VehicleBodyShader.cpp $(Release_Include_Path) > gccRelease/VehicleBodyShader.d

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

