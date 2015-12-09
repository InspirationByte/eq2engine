# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../Game/" -I"../public/platform" -I"AI/Recast/Include" -I"AI/Detour/Include" -I"gmScript/binds/" -I"gmScript/platform/win32msvc" -I"gmScript/gm/" -I"../src_dependency/sdl2/include" 
PigeonGameRelease_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../Game/" -I"../public/platform" -I"AI/Recast/Include" -I"AI/Detour/Include" -I"gmScript/binds/" -I"gmScript/platform/win32msvc" -I"gmScript/gm/" 
Release_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../Game/" -I"../public/platform" -I"AI/Recast/Include" -I"AI/Detour/Include" 
STDGameRelease_Include_Path=-I"../FileSystem" -I"../Engine" -I"../public" -I"../Game/" -I"../public/core" -I"../public/platform" -I"../public/materialsystem/renderers" -I"AI/Recast/Include" -I"AI/Detour/Include" -I"gmScript/binds/" -I"gmScript/platform/win32msvc" -I"gmScript/gm/" -I"../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=
PigeonGameRelease_Library_Path=
Release_Library_Path=
STDGameRelease_Library_Path=

# Additional libraries...
Debug_Libraries=-Wl,--start-group -luser32  -Wl,--end-group
PigeonGameRelease_Libraries=-Wl,--start-group -luser32  -Wl,--end-group
Release_Libraries=-Wl,--start-group -luser32  -Wl,--end-group
STDGameRelease_Libraries=-Wl,--start-group -luser32  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D GAME_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D STDGAME 
PigeonGameRelease_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D GAME_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D PIGEONGAME 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D GAME_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D WCGAME 
STDGameRelease_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D GAME_EXPORT -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE -D STDGAME 

# Implictly linked object files...
Debug_Implicitly_Linked_Objects=
PigeonGameRelease_Implicitly_Linked_Objects=
Release_Implicitly_Linked_Objects=
STDGameRelease_Implicitly_Linked_Objects=

# Compiler flags...
Debug_Compiler_Flags=-fPIC -Wall -O0 -g 
PigeonGameRelease_Compiler_Flags=-fPIC -Wall -O2 
Release_Compiler_Flags=-fPIC -Wall -O2 
STDGameRelease_Compiler_Flags=-fPIC -Wall -O2 

# Builds all configurations for this project...
.PHONY: build_all_configurations
build_all_configurations: Debug PigeonGameRelease Release STDGameRelease 

# Builds the Debug configuration...
.PHONY: Debug
Debug: create_folders gccDebug/../public/Utils/GeomTools.o gccDebug/../public/Math/math_util.o gccDebug/../public/BaseShader.o gccDebug/../public/BaseRenderableObject.o gccDebug/../public/RenderList.o gccDebug/../public/ViewParams.o gccDebug/../public/dsm_esm_loader.o gccDebug/../public/dsm_loader.o gccDebug/../public/dsm_obj_loader.o gccDebug/GameLibrary.o gccDebug/GameState.o gccDebug/AI/AIBaseNPC.o gccDebug/AI/AINode.o gccDebug/AI/AITaskTypes.o gccDebug/AI/AI_Idle.o gccDebug/AI/AI_MovementTarget.o gccDebug/AI/ai_navigator.o gccDebug/AI/Detour/Source/DetourAlloc.o gccDebug/AI/Detour/Source/DetourCommon.o gccDebug/AI/Detour/Source/DetourNavMesh.o gccDebug/AI/Detour/Source/DetourNavMeshBuilder.o gccDebug/AI/Detour/Source/DetourNavMeshQuery.o gccDebug/AI/Detour/Source/DetourNode.o gccDebug/AI/Recast/Source/Recast.o gccDebug/AI/Recast/Source/RecastAlloc.o gccDebug/AI/Recast/Source/RecastArea.o gccDebug/AI/Recast/Source/RecastContour.o gccDebug/AI/Recast/Source/RecastFilter.o gccDebug/AI/Recast/Source/RecastLayers.o gccDebug/AI/Recast/Source/RecastMesh.o gccDebug/AI/Recast/Source/RecastMeshDetail.o gccDebug/AI/Recast/Source/RecastRasterization.o gccDebug/AI/Recast/Source/RecastRegion.o gccDebug/AI/AIMemoryBase.o gccDebug/AI/AIScriptTask.o gccDebug/AI/AITaskActionBase.o gccDebug/AmmoType.o gccDebug/BaseEntity.o gccDebug/InfoCamera.o gccDebug/Ladder.o gccDebug/LogicEntities.o gccDebug/BulletSimulator.o gccDebug/BaseActor.o gccDebug/BaseAnimating.o gccDebug/BaseWeapon.o gccDebug/EffectEntities.o gccDebug/EngineEntities.o gccDebug/PlayerSpawn.o gccDebug/ModelEntities.o gccDebug/SoundEntities.o gccDebug/Triggers.o gccDebug/../public/anim_activity.o gccDebug/../public/anim_events.o gccDebug/../public/BoneSetup.o gccDebug/../public/ragdoll.o gccDebug/EntityQueue.o gccDebug/EntityDataField.o gccDebug/GameTrace.o gccDebug/Effects.o gccDebug/GameInput.o gccDebug/GlobalVarsGame.o gccDebug/MaterialProxy.o gccDebug/Rain.o gccDebug/Snow.o gccDebug/SaveGame_Events.o gccDebug/SaveRestoreManager.o gccDebug/EffectRender.o gccDebug/FilterPipelineBase.o gccDebug/GameRenderer.o gccDebug/ParticleRenderer.o gccDebug/RenderDefs.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/PigeonGame/BasePigeon.o gccDebug/PigeonGame/GameRules_Pigeon.o gccDebug/PigeonGame/Pigeon_Player.o gccDebug/GameRules_WhiteCage.o gccDebug/npc_cyborg.o gccDebug/npc_soldier.o gccDebug/Player.o gccDebug/viewmodel.o gccDebug/weapon_ak74.o gccDebug/weapon_deagle.o gccDebug/weapon_f1.o gccDebug/weapon_wiremine.o gccDebug/EGUI/EQUI_Manager.o gccDebug/EGUI/EqUI_Panel.o gccDebug/EntityBind.o gccDebug/EqGMS.o gccDebug/EngineScriptBind.o gccDebug/gmScript/gm/gmArraySimple.o gccDebug/gmScript/gm/gmByteCode.o gccDebug/gmScript/gm/gmByteCodeGen.o gccDebug/gmScript/gm/gmCodeGen.o gccDebug/gmScript/gm/gmCodeGenHooks.o gccDebug/gmScript/gm/gmCodeTree.o gccDebug/gmScript/gm/gmCrc.o gccDebug/gmScript/gm/gmDebug.o gccDebug/gmScript/gm/gmFunctionObject.o gccDebug/gmScript/gm/gmHash.o gccDebug/gmScript/gm/gmIncGC.o gccDebug/gmScript/gm/gmLibHooks.o gccDebug/gmScript/gm/gmListDouble.o gccDebug/gmScript/gm/gmLog.o gccDebug/gmScript/gm/gmMachine.o gccDebug/gmScript/gm/gmMachineLib.o gccDebug/gmScript/gm/gmMem.o gccDebug/gmScript/gm/gmMemChain.o gccDebug/gmScript/gm/gmMemFixed.o gccDebug/gmScript/gm/gmMemFixedSet.o gccDebug/gmScript/gm/gmOperators.o gccDebug/gmScript/gm/gmParser.o gccDebug/gmScript/gm/gmScanner.o gccDebug/gmScript/gm/gmStream.o gccDebug/gmScript/gm/gmStreamBuffer.o gccDebug/gmScript/gm/gmStringObject.o gccDebug/gmScript/gm/gmTableObject.o gccDebug/gmScript/gm/gmThread.o gccDebug/gmScript/gm/gmUserObject.o gccDebug/gmScript/gm/gmUtil.o gccDebug/gmScript/gm/gmVariable.o gccDebug/gmScript/binds/gmArrayLib.o gccDebug/gmScript/binds/gmCall.o gccDebug/gmScript/binds/gmGCRoot.o gccDebug/gmScript/binds/gmGCRootUtil.o gccDebug/gmScript/binds/gmHelpers.o gccDebug/gmScript/binds/gmMathLib.o gccDebug/gmScript/binds/gmStringLib.o gccDebug/gmScript/binds/gmSystemLib.o gccDebug/gmScript/binds/gmVector3Lib.o 
	g++ -fPIC -shared -Wl,-soname,libGameDLL.so -o gccDebug/libGameDLL.so gccDebug/../public/Utils/GeomTools.o gccDebug/../public/Math/math_util.o gccDebug/../public/BaseShader.o gccDebug/../public/BaseRenderableObject.o gccDebug/../public/RenderList.o gccDebug/../public/ViewParams.o gccDebug/../public/dsm_esm_loader.o gccDebug/../public/dsm_loader.o gccDebug/../public/dsm_obj_loader.o gccDebug/GameLibrary.o gccDebug/GameState.o gccDebug/AI/AIBaseNPC.o gccDebug/AI/AINode.o gccDebug/AI/AITaskTypes.o gccDebug/AI/AI_Idle.o gccDebug/AI/AI_MovementTarget.o gccDebug/AI/ai_navigator.o gccDebug/AI/Detour/Source/DetourAlloc.o gccDebug/AI/Detour/Source/DetourCommon.o gccDebug/AI/Detour/Source/DetourNavMesh.o gccDebug/AI/Detour/Source/DetourNavMeshBuilder.o gccDebug/AI/Detour/Source/DetourNavMeshQuery.o gccDebug/AI/Detour/Source/DetourNode.o gccDebug/AI/Recast/Source/Recast.o gccDebug/AI/Recast/Source/RecastAlloc.o gccDebug/AI/Recast/Source/RecastArea.o gccDebug/AI/Recast/Source/RecastContour.o gccDebug/AI/Recast/Source/RecastFilter.o gccDebug/AI/Recast/Source/RecastLayers.o gccDebug/AI/Recast/Source/RecastMesh.o gccDebug/AI/Recast/Source/RecastMeshDetail.o gccDebug/AI/Recast/Source/RecastRasterization.o gccDebug/AI/Recast/Source/RecastRegion.o gccDebug/AI/AIMemoryBase.o gccDebug/AI/AIScriptTask.o gccDebug/AI/AITaskActionBase.o gccDebug/AmmoType.o gccDebug/BaseEntity.o gccDebug/InfoCamera.o gccDebug/Ladder.o gccDebug/LogicEntities.o gccDebug/BulletSimulator.o gccDebug/BaseActor.o gccDebug/BaseAnimating.o gccDebug/BaseWeapon.o gccDebug/EffectEntities.o gccDebug/EngineEntities.o gccDebug/PlayerSpawn.o gccDebug/ModelEntities.o gccDebug/SoundEntities.o gccDebug/Triggers.o gccDebug/../public/anim_activity.o gccDebug/../public/anim_events.o gccDebug/../public/BoneSetup.o gccDebug/../public/ragdoll.o gccDebug/EntityQueue.o gccDebug/EntityDataField.o gccDebug/GameTrace.o gccDebug/Effects.o gccDebug/GameInput.o gccDebug/GlobalVarsGame.o gccDebug/MaterialProxy.o gccDebug/Rain.o gccDebug/Snow.o gccDebug/SaveGame_Events.o gccDebug/SaveRestoreManager.o gccDebug/EffectRender.o gccDebug/FilterPipelineBase.o gccDebug/GameRenderer.o gccDebug/ParticleRenderer.o gccDebug/RenderDefs.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/PigeonGame/BasePigeon.o gccDebug/PigeonGame/GameRules_Pigeon.o gccDebug/PigeonGame/Pigeon_Player.o gccDebug/GameRules_WhiteCage.o gccDebug/npc_cyborg.o gccDebug/npc_soldier.o gccDebug/Player.o gccDebug/viewmodel.o gccDebug/weapon_ak74.o gccDebug/weapon_deagle.o gccDebug/weapon_f1.o gccDebug/weapon_wiremine.o gccDebug/EGUI/EQUI_Manager.o gccDebug/EGUI/EqUI_Panel.o gccDebug/EntityBind.o gccDebug/EqGMS.o gccDebug/EngineScriptBind.o gccDebug/gmScript/gm/gmArraySimple.o gccDebug/gmScript/gm/gmByteCode.o gccDebug/gmScript/gm/gmByteCodeGen.o gccDebug/gmScript/gm/gmCodeGen.o gccDebug/gmScript/gm/gmCodeGenHooks.o gccDebug/gmScript/gm/gmCodeTree.o gccDebug/gmScript/gm/gmCrc.o gccDebug/gmScript/gm/gmDebug.o gccDebug/gmScript/gm/gmFunctionObject.o gccDebug/gmScript/gm/gmHash.o gccDebug/gmScript/gm/gmIncGC.o gccDebug/gmScript/gm/gmLibHooks.o gccDebug/gmScript/gm/gmListDouble.o gccDebug/gmScript/gm/gmLog.o gccDebug/gmScript/gm/gmMachine.o gccDebug/gmScript/gm/gmMachineLib.o gccDebug/gmScript/gm/gmMem.o gccDebug/gmScript/gm/gmMemChain.o gccDebug/gmScript/gm/gmMemFixed.o gccDebug/gmScript/gm/gmMemFixedSet.o gccDebug/gmScript/gm/gmOperators.o gccDebug/gmScript/gm/gmParser.o gccDebug/gmScript/gm/gmScanner.o gccDebug/gmScript/gm/gmStream.o gccDebug/gmScript/gm/gmStreamBuffer.o gccDebug/gmScript/gm/gmStringObject.o gccDebug/gmScript/gm/gmTableObject.o gccDebug/gmScript/gm/gmThread.o gccDebug/gmScript/gm/gmUserObject.o gccDebug/gmScript/gm/gmUtil.o gccDebug/gmScript/gm/gmVariable.o gccDebug/gmScript/binds/gmArrayLib.o gccDebug/gmScript/binds/gmCall.o gccDebug/gmScript/binds/gmGCRoot.o gccDebug/gmScript/binds/gmGCRootUtil.o gccDebug/gmScript/binds/gmHelpers.o gccDebug/gmScript/binds/gmMathLib.o gccDebug/gmScript/binds/gmStringLib.o gccDebug/gmScript/binds/gmSystemLib.o gccDebug/gmScript/binds/gmVector3Lib.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file ../public/Utils/GeomTools.cpp for the Debug configuration...
-include gccDebug/../public/Utils/GeomTools.d
gccDebug/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(Debug_Include_Path) -o gccDebug/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(Debug_Include_Path) > gccDebug/../public/Utils/GeomTools.d

# Compiles file ../public/Math/math_util.cpp for the Debug configuration...
-include gccDebug/../public/Math/math_util.d
gccDebug/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Math/math_util.cpp $(Debug_Include_Path) -o gccDebug/../public/Math/math_util.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Math/math_util.cpp $(Debug_Include_Path) > gccDebug/../public/Math/math_util.d

# Compiles file ../public/BaseShader.cpp for the Debug configuration...
-include gccDebug/../public/BaseShader.d
gccDebug/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BaseShader.cpp $(Debug_Include_Path) -o gccDebug/../public/BaseShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BaseShader.cpp $(Debug_Include_Path) > gccDebug/../public/BaseShader.d

# Compiles file ../public/BaseRenderableObject.cpp for the Debug configuration...
-include gccDebug/../public/BaseRenderableObject.d
gccDebug/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(Debug_Include_Path) -o gccDebug/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(Debug_Include_Path) > gccDebug/../public/BaseRenderableObject.d

# Compiles file ../public/RenderList.cpp for the Debug configuration...
-include gccDebug/../public/RenderList.d
gccDebug/../public/RenderList.o: ../public/RenderList.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/RenderList.cpp $(Debug_Include_Path) -o gccDebug/../public/RenderList.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/RenderList.cpp $(Debug_Include_Path) > gccDebug/../public/RenderList.d

# Compiles file ../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../public/ViewParams.d
gccDebug/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../public/ViewParams.d

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

# Compiles file GameLibrary.cpp for the Debug configuration...
-include gccDebug/GameLibrary.d
gccDebug/GameLibrary.o: GameLibrary.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameLibrary.cpp $(Debug_Include_Path) -o gccDebug/GameLibrary.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameLibrary.cpp $(Debug_Include_Path) > gccDebug/GameLibrary.d

# Compiles file GameState.cpp for the Debug configuration...
-include gccDebug/GameState.d
gccDebug/GameState.o: GameState.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameState.cpp $(Debug_Include_Path) -o gccDebug/GameState.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameState.cpp $(Debug_Include_Path) > gccDebug/GameState.d

# Compiles file AI/AIBaseNPC.cpp for the Debug configuration...
-include gccDebug/AI/AIBaseNPC.d
gccDebug/AI/AIBaseNPC.o: AI/AIBaseNPC.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AIBaseNPC.cpp $(Debug_Include_Path) -o gccDebug/AI/AIBaseNPC.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AIBaseNPC.cpp $(Debug_Include_Path) > gccDebug/AI/AIBaseNPC.d

# Compiles file AI/AINode.cpp for the Debug configuration...
-include gccDebug/AI/AINode.d
gccDebug/AI/AINode.o: AI/AINode.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AINode.cpp $(Debug_Include_Path) -o gccDebug/AI/AINode.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AINode.cpp $(Debug_Include_Path) > gccDebug/AI/AINode.d

# Compiles file AI/AITaskTypes.cpp for the Debug configuration...
-include gccDebug/AI/AITaskTypes.d
gccDebug/AI/AITaskTypes.o: AI/AITaskTypes.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AITaskTypes.cpp $(Debug_Include_Path) -o gccDebug/AI/AITaskTypes.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AITaskTypes.cpp $(Debug_Include_Path) > gccDebug/AI/AITaskTypes.d

# Compiles file AI/AI_Idle.cpp for the Debug configuration...
-include gccDebug/AI/AI_Idle.d
gccDebug/AI/AI_Idle.o: AI/AI_Idle.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AI_Idle.cpp $(Debug_Include_Path) -o gccDebug/AI/AI_Idle.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AI_Idle.cpp $(Debug_Include_Path) > gccDebug/AI/AI_Idle.d

# Compiles file AI/AI_MovementTarget.cpp for the Debug configuration...
-include gccDebug/AI/AI_MovementTarget.d
gccDebug/AI/AI_MovementTarget.o: AI/AI_MovementTarget.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AI_MovementTarget.cpp $(Debug_Include_Path) -o gccDebug/AI/AI_MovementTarget.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AI_MovementTarget.cpp $(Debug_Include_Path) > gccDebug/AI/AI_MovementTarget.d

# Compiles file AI/ai_navigator.cpp for the Debug configuration...
-include gccDebug/AI/ai_navigator.d
gccDebug/AI/ai_navigator.o: AI/ai_navigator.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/ai_navigator.cpp $(Debug_Include_Path) -o gccDebug/AI/ai_navigator.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/ai_navigator.cpp $(Debug_Include_Path) > gccDebug/AI/ai_navigator.d

# Compiles file AI/Detour/Source/DetourAlloc.cpp for the Debug configuration...
-include gccDebug/AI/Detour/Source/DetourAlloc.d
gccDebug/AI/Detour/Source/DetourAlloc.o: AI/Detour/Source/DetourAlloc.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Detour/Source/DetourAlloc.cpp $(Debug_Include_Path) -o gccDebug/AI/Detour/Source/DetourAlloc.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Detour/Source/DetourAlloc.cpp $(Debug_Include_Path) > gccDebug/AI/Detour/Source/DetourAlloc.d

# Compiles file AI/Detour/Source/DetourCommon.cpp for the Debug configuration...
-include gccDebug/AI/Detour/Source/DetourCommon.d
gccDebug/AI/Detour/Source/DetourCommon.o: AI/Detour/Source/DetourCommon.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Detour/Source/DetourCommon.cpp $(Debug_Include_Path) -o gccDebug/AI/Detour/Source/DetourCommon.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Detour/Source/DetourCommon.cpp $(Debug_Include_Path) > gccDebug/AI/Detour/Source/DetourCommon.d

# Compiles file AI/Detour/Source/DetourNavMesh.cpp for the Debug configuration...
-include gccDebug/AI/Detour/Source/DetourNavMesh.d
gccDebug/AI/Detour/Source/DetourNavMesh.o: AI/Detour/Source/DetourNavMesh.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Detour/Source/DetourNavMesh.cpp $(Debug_Include_Path) -o gccDebug/AI/Detour/Source/DetourNavMesh.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Detour/Source/DetourNavMesh.cpp $(Debug_Include_Path) > gccDebug/AI/Detour/Source/DetourNavMesh.d

# Compiles file AI/Detour/Source/DetourNavMeshBuilder.cpp for the Debug configuration...
-include gccDebug/AI/Detour/Source/DetourNavMeshBuilder.d
gccDebug/AI/Detour/Source/DetourNavMeshBuilder.o: AI/Detour/Source/DetourNavMeshBuilder.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshBuilder.cpp $(Debug_Include_Path) -o gccDebug/AI/Detour/Source/DetourNavMeshBuilder.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshBuilder.cpp $(Debug_Include_Path) > gccDebug/AI/Detour/Source/DetourNavMeshBuilder.d

# Compiles file AI/Detour/Source/DetourNavMeshQuery.cpp for the Debug configuration...
-include gccDebug/AI/Detour/Source/DetourNavMeshQuery.d
gccDebug/AI/Detour/Source/DetourNavMeshQuery.o: AI/Detour/Source/DetourNavMeshQuery.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshQuery.cpp $(Debug_Include_Path) -o gccDebug/AI/Detour/Source/DetourNavMeshQuery.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshQuery.cpp $(Debug_Include_Path) > gccDebug/AI/Detour/Source/DetourNavMeshQuery.d

# Compiles file AI/Detour/Source/DetourNode.cpp for the Debug configuration...
-include gccDebug/AI/Detour/Source/DetourNode.d
gccDebug/AI/Detour/Source/DetourNode.o: AI/Detour/Source/DetourNode.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Detour/Source/DetourNode.cpp $(Debug_Include_Path) -o gccDebug/AI/Detour/Source/DetourNode.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Detour/Source/DetourNode.cpp $(Debug_Include_Path) > gccDebug/AI/Detour/Source/DetourNode.d

# Compiles file AI/Recast/Source/Recast.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/Recast.d
gccDebug/AI/Recast/Source/Recast.o: AI/Recast/Source/Recast.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/Recast.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/Recast.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/Recast.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/Recast.d

# Compiles file AI/Recast/Source/RecastAlloc.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastAlloc.d
gccDebug/AI/Recast/Source/RecastAlloc.o: AI/Recast/Source/RecastAlloc.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastAlloc.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastAlloc.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastAlloc.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastAlloc.d

# Compiles file AI/Recast/Source/RecastArea.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastArea.d
gccDebug/AI/Recast/Source/RecastArea.o: AI/Recast/Source/RecastArea.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastArea.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastArea.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastArea.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastArea.d

# Compiles file AI/Recast/Source/RecastContour.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastContour.d
gccDebug/AI/Recast/Source/RecastContour.o: AI/Recast/Source/RecastContour.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastContour.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastContour.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastContour.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastContour.d

# Compiles file AI/Recast/Source/RecastFilter.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastFilter.d
gccDebug/AI/Recast/Source/RecastFilter.o: AI/Recast/Source/RecastFilter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastFilter.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastFilter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastFilter.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastFilter.d

# Compiles file AI/Recast/Source/RecastLayers.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastLayers.d
gccDebug/AI/Recast/Source/RecastLayers.o: AI/Recast/Source/RecastLayers.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastLayers.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastLayers.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastLayers.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastLayers.d

# Compiles file AI/Recast/Source/RecastMesh.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastMesh.d
gccDebug/AI/Recast/Source/RecastMesh.o: AI/Recast/Source/RecastMesh.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastMesh.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastMesh.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastMesh.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastMesh.d

# Compiles file AI/Recast/Source/RecastMeshDetail.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastMeshDetail.d
gccDebug/AI/Recast/Source/RecastMeshDetail.o: AI/Recast/Source/RecastMeshDetail.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastMeshDetail.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastMeshDetail.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastMeshDetail.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastMeshDetail.d

# Compiles file AI/Recast/Source/RecastRasterization.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastRasterization.d
gccDebug/AI/Recast/Source/RecastRasterization.o: AI/Recast/Source/RecastRasterization.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastRasterization.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastRasterization.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastRasterization.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastRasterization.d

# Compiles file AI/Recast/Source/RecastRegion.cpp for the Debug configuration...
-include gccDebug/AI/Recast/Source/RecastRegion.d
gccDebug/AI/Recast/Source/RecastRegion.o: AI/Recast/Source/RecastRegion.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/Recast/Source/RecastRegion.cpp $(Debug_Include_Path) -o gccDebug/AI/Recast/Source/RecastRegion.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/Recast/Source/RecastRegion.cpp $(Debug_Include_Path) > gccDebug/AI/Recast/Source/RecastRegion.d

# Compiles file AI/AIMemoryBase.cpp for the Debug configuration...
-include gccDebug/AI/AIMemoryBase.d
gccDebug/AI/AIMemoryBase.o: AI/AIMemoryBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AIMemoryBase.cpp $(Debug_Include_Path) -o gccDebug/AI/AIMemoryBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AIMemoryBase.cpp $(Debug_Include_Path) > gccDebug/AI/AIMemoryBase.d

# Compiles file AI/AIScriptTask.cpp for the Debug configuration...
-include gccDebug/AI/AIScriptTask.d
gccDebug/AI/AIScriptTask.o: AI/AIScriptTask.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AIScriptTask.cpp $(Debug_Include_Path) -o gccDebug/AI/AIScriptTask.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AIScriptTask.cpp $(Debug_Include_Path) > gccDebug/AI/AIScriptTask.d

# Compiles file AI/AITaskActionBase.cpp for the Debug configuration...
-include gccDebug/AI/AITaskActionBase.d
gccDebug/AI/AITaskActionBase.o: AI/AITaskActionBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AI/AITaskActionBase.cpp $(Debug_Include_Path) -o gccDebug/AI/AITaskActionBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AI/AITaskActionBase.cpp $(Debug_Include_Path) > gccDebug/AI/AITaskActionBase.d

# Compiles file AmmoType.cpp for the Debug configuration...
-include gccDebug/AmmoType.d
gccDebug/AmmoType.o: AmmoType.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c AmmoType.cpp $(Debug_Include_Path) -o gccDebug/AmmoType.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM AmmoType.cpp $(Debug_Include_Path) > gccDebug/AmmoType.d

# Compiles file BaseEntity.cpp for the Debug configuration...
-include gccDebug/BaseEntity.d
gccDebug/BaseEntity.o: BaseEntity.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseEntity.cpp $(Debug_Include_Path) -o gccDebug/BaseEntity.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseEntity.cpp $(Debug_Include_Path) > gccDebug/BaseEntity.d

# Compiles file InfoCamera.cpp for the Debug configuration...
-include gccDebug/InfoCamera.d
gccDebug/InfoCamera.o: InfoCamera.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c InfoCamera.cpp $(Debug_Include_Path) -o gccDebug/InfoCamera.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM InfoCamera.cpp $(Debug_Include_Path) > gccDebug/InfoCamera.d

# Compiles file Ladder.cpp for the Debug configuration...
-include gccDebug/Ladder.d
gccDebug/Ladder.o: Ladder.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Ladder.cpp $(Debug_Include_Path) -o gccDebug/Ladder.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Ladder.cpp $(Debug_Include_Path) > gccDebug/Ladder.d

# Compiles file LogicEntities.cpp for the Debug configuration...
-include gccDebug/LogicEntities.d
gccDebug/LogicEntities.o: LogicEntities.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c LogicEntities.cpp $(Debug_Include_Path) -o gccDebug/LogicEntities.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM LogicEntities.cpp $(Debug_Include_Path) > gccDebug/LogicEntities.d

# Compiles file BulletSimulator.cpp for the Debug configuration...
-include gccDebug/BulletSimulator.d
gccDebug/BulletSimulator.o: BulletSimulator.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BulletSimulator.cpp $(Debug_Include_Path) -o gccDebug/BulletSimulator.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BulletSimulator.cpp $(Debug_Include_Path) > gccDebug/BulletSimulator.d

# Compiles file BaseActor.cpp for the Debug configuration...
-include gccDebug/BaseActor.d
gccDebug/BaseActor.o: BaseActor.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseActor.cpp $(Debug_Include_Path) -o gccDebug/BaseActor.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseActor.cpp $(Debug_Include_Path) > gccDebug/BaseActor.d

# Compiles file BaseAnimating.cpp for the Debug configuration...
-include gccDebug/BaseAnimating.d
gccDebug/BaseAnimating.o: BaseAnimating.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseAnimating.cpp $(Debug_Include_Path) -o gccDebug/BaseAnimating.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseAnimating.cpp $(Debug_Include_Path) > gccDebug/BaseAnimating.d

# Compiles file BaseWeapon.cpp for the Debug configuration...
-include gccDebug/BaseWeapon.d
gccDebug/BaseWeapon.o: BaseWeapon.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseWeapon.cpp $(Debug_Include_Path) -o gccDebug/BaseWeapon.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseWeapon.cpp $(Debug_Include_Path) > gccDebug/BaseWeapon.d

# Compiles file EffectEntities.cpp for the Debug configuration...
-include gccDebug/EffectEntities.d
gccDebug/EffectEntities.o: EffectEntities.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EffectEntities.cpp $(Debug_Include_Path) -o gccDebug/EffectEntities.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EffectEntities.cpp $(Debug_Include_Path) > gccDebug/EffectEntities.d

# Compiles file EngineEntities.cpp for the Debug configuration...
-include gccDebug/EngineEntities.d
gccDebug/EngineEntities.o: EngineEntities.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EngineEntities.cpp $(Debug_Include_Path) -o gccDebug/EngineEntities.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EngineEntities.cpp $(Debug_Include_Path) > gccDebug/EngineEntities.d

# Compiles file PlayerSpawn.cpp for the Debug configuration...
-include gccDebug/PlayerSpawn.d
gccDebug/PlayerSpawn.o: PlayerSpawn.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c PlayerSpawn.cpp $(Debug_Include_Path) -o gccDebug/PlayerSpawn.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM PlayerSpawn.cpp $(Debug_Include_Path) > gccDebug/PlayerSpawn.d

# Compiles file ModelEntities.cpp for the Debug configuration...
-include gccDebug/ModelEntities.d
gccDebug/ModelEntities.o: ModelEntities.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ModelEntities.cpp $(Debug_Include_Path) -o gccDebug/ModelEntities.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ModelEntities.cpp $(Debug_Include_Path) > gccDebug/ModelEntities.d

# Compiles file SoundEntities.cpp for the Debug configuration...
-include gccDebug/SoundEntities.d
gccDebug/SoundEntities.o: SoundEntities.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SoundEntities.cpp $(Debug_Include_Path) -o gccDebug/SoundEntities.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SoundEntities.cpp $(Debug_Include_Path) > gccDebug/SoundEntities.d

# Compiles file Triggers.cpp for the Debug configuration...
-include gccDebug/Triggers.d
gccDebug/Triggers.o: Triggers.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Triggers.cpp $(Debug_Include_Path) -o gccDebug/Triggers.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Triggers.cpp $(Debug_Include_Path) > gccDebug/Triggers.d

# Compiles file ../public/anim_activity.cpp for the Debug configuration...
-include gccDebug/../public/anim_activity.d
gccDebug/../public/anim_activity.o: ../public/anim_activity.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/anim_activity.cpp $(Debug_Include_Path) -o gccDebug/../public/anim_activity.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/anim_activity.cpp $(Debug_Include_Path) > gccDebug/../public/anim_activity.d

# Compiles file ../public/anim_events.cpp for the Debug configuration...
-include gccDebug/../public/anim_events.d
gccDebug/../public/anim_events.o: ../public/anim_events.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/anim_events.cpp $(Debug_Include_Path) -o gccDebug/../public/anim_events.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/anim_events.cpp $(Debug_Include_Path) > gccDebug/../public/anim_events.d

# Compiles file ../public/BoneSetup.cpp for the Debug configuration...
-include gccDebug/../public/BoneSetup.d
gccDebug/../public/BoneSetup.o: ../public/BoneSetup.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BoneSetup.cpp $(Debug_Include_Path) -o gccDebug/../public/BoneSetup.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BoneSetup.cpp $(Debug_Include_Path) > gccDebug/../public/BoneSetup.d

# Compiles file ../public/ragdoll.cpp for the Debug configuration...
-include gccDebug/../public/ragdoll.d
gccDebug/../public/ragdoll.o: ../public/ragdoll.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ragdoll.cpp $(Debug_Include_Path) -o gccDebug/../public/ragdoll.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ragdoll.cpp $(Debug_Include_Path) > gccDebug/../public/ragdoll.d

# Compiles file EntityQueue.cpp for the Debug configuration...
-include gccDebug/EntityQueue.d
gccDebug/EntityQueue.o: EntityQueue.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityQueue.cpp $(Debug_Include_Path) -o gccDebug/EntityQueue.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityQueue.cpp $(Debug_Include_Path) > gccDebug/EntityQueue.d

# Compiles file EntityDataField.cpp for the Debug configuration...
-include gccDebug/EntityDataField.d
gccDebug/EntityDataField.o: EntityDataField.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityDataField.cpp $(Debug_Include_Path) -o gccDebug/EntityDataField.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityDataField.cpp $(Debug_Include_Path) > gccDebug/EntityDataField.d

# Compiles file GameTrace.cpp for the Debug configuration...
-include gccDebug/GameTrace.d
gccDebug/GameTrace.o: GameTrace.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameTrace.cpp $(Debug_Include_Path) -o gccDebug/GameTrace.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameTrace.cpp $(Debug_Include_Path) > gccDebug/GameTrace.d

# Compiles file Effects.cpp for the Debug configuration...
-include gccDebug/Effects.d
gccDebug/Effects.o: Effects.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Effects.cpp $(Debug_Include_Path) -o gccDebug/Effects.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Effects.cpp $(Debug_Include_Path) > gccDebug/Effects.d

# Compiles file GameInput.cpp for the Debug configuration...
-include gccDebug/GameInput.d
gccDebug/GameInput.o: GameInput.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameInput.cpp $(Debug_Include_Path) -o gccDebug/GameInput.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameInput.cpp $(Debug_Include_Path) > gccDebug/GameInput.d

# Compiles file GlobalVarsGame.cpp for the Debug configuration...
-include gccDebug/GlobalVarsGame.d
gccDebug/GlobalVarsGame.o: GlobalVarsGame.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GlobalVarsGame.cpp $(Debug_Include_Path) -o gccDebug/GlobalVarsGame.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GlobalVarsGame.cpp $(Debug_Include_Path) > gccDebug/GlobalVarsGame.d

# Compiles file MaterialProxy.cpp for the Debug configuration...
-include gccDebug/MaterialProxy.d
gccDebug/MaterialProxy.o: MaterialProxy.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c MaterialProxy.cpp $(Debug_Include_Path) -o gccDebug/MaterialProxy.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM MaterialProxy.cpp $(Debug_Include_Path) > gccDebug/MaterialProxy.d

# Compiles file Rain.cpp for the Debug configuration...
-include gccDebug/Rain.d
gccDebug/Rain.o: Rain.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Rain.cpp $(Debug_Include_Path) -o gccDebug/Rain.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Rain.cpp $(Debug_Include_Path) > gccDebug/Rain.d

# Compiles file Snow.cpp for the Debug configuration...
-include gccDebug/Snow.d
gccDebug/Snow.o: Snow.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Snow.cpp $(Debug_Include_Path) -o gccDebug/Snow.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Snow.cpp $(Debug_Include_Path) > gccDebug/Snow.d

# Compiles file SaveGame_Events.cpp for the Debug configuration...
-include gccDebug/SaveGame_Events.d
gccDebug/SaveGame_Events.o: SaveGame_Events.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SaveGame_Events.cpp $(Debug_Include_Path) -o gccDebug/SaveGame_Events.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SaveGame_Events.cpp $(Debug_Include_Path) > gccDebug/SaveGame_Events.d

# Compiles file SaveRestoreManager.cpp for the Debug configuration...
-include gccDebug/SaveRestoreManager.d
gccDebug/SaveRestoreManager.o: SaveRestoreManager.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SaveRestoreManager.cpp $(Debug_Include_Path) -o gccDebug/SaveRestoreManager.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SaveRestoreManager.cpp $(Debug_Include_Path) > gccDebug/SaveRestoreManager.d

# Compiles file EffectRender.cpp for the Debug configuration...
-include gccDebug/EffectRender.d
gccDebug/EffectRender.o: EffectRender.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EffectRender.cpp $(Debug_Include_Path) -o gccDebug/EffectRender.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EffectRender.cpp $(Debug_Include_Path) > gccDebug/EffectRender.d

# Compiles file FilterPipelineBase.cpp for the Debug configuration...
-include gccDebug/FilterPipelineBase.d
gccDebug/FilterPipelineBase.o: FilterPipelineBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c FilterPipelineBase.cpp $(Debug_Include_Path) -o gccDebug/FilterPipelineBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM FilterPipelineBase.cpp $(Debug_Include_Path) > gccDebug/FilterPipelineBase.d

# Compiles file GameRenderer.cpp for the Debug configuration...
-include gccDebug/GameRenderer.d
gccDebug/GameRenderer.o: GameRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameRenderer.cpp $(Debug_Include_Path) -o gccDebug/GameRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameRenderer.cpp $(Debug_Include_Path) > gccDebug/GameRenderer.d

# Compiles file ParticleRenderer.cpp for the Debug configuration...
-include gccDebug/ParticleRenderer.d
gccDebug/ParticleRenderer.o: ParticleRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ParticleRenderer.cpp $(Debug_Include_Path) -o gccDebug/ParticleRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ParticleRenderer.cpp $(Debug_Include_Path) > gccDebug/ParticleRenderer.d

# Compiles file RenderDefs.cpp for the Debug configuration...
-include gccDebug/RenderDefs.d
gccDebug/RenderDefs.o: RenderDefs.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c RenderDefs.cpp $(Debug_Include_Path) -o gccDebug/RenderDefs.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM RenderDefs.cpp $(Debug_Include_Path) > gccDebug/RenderDefs.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the Debug configuration...
-include gccDebug/../public/GameSoundEmitterSystem.d
gccDebug/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(Debug_Include_Path) -o gccDebug/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(Debug_Include_Path) > gccDebug/../public/GameSoundEmitterSystem.d

# Compiles file PigeonGame/BasePigeon.cpp for the Debug configuration...
-include gccDebug/PigeonGame/BasePigeon.d
gccDebug/PigeonGame/BasePigeon.o: PigeonGame/BasePigeon.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c PigeonGame/BasePigeon.cpp $(Debug_Include_Path) -o gccDebug/PigeonGame/BasePigeon.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM PigeonGame/BasePigeon.cpp $(Debug_Include_Path) > gccDebug/PigeonGame/BasePigeon.d

# Compiles file PigeonGame/GameRules_Pigeon.cpp for the Debug configuration...
-include gccDebug/PigeonGame/GameRules_Pigeon.d
gccDebug/PigeonGame/GameRules_Pigeon.o: PigeonGame/GameRules_Pigeon.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c PigeonGame/GameRules_Pigeon.cpp $(Debug_Include_Path) -o gccDebug/PigeonGame/GameRules_Pigeon.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM PigeonGame/GameRules_Pigeon.cpp $(Debug_Include_Path) > gccDebug/PigeonGame/GameRules_Pigeon.d

# Compiles file PigeonGame/Pigeon_Player.cpp for the Debug configuration...
-include gccDebug/PigeonGame/Pigeon_Player.d
gccDebug/PigeonGame/Pigeon_Player.o: PigeonGame/Pigeon_Player.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c PigeonGame/Pigeon_Player.cpp $(Debug_Include_Path) -o gccDebug/PigeonGame/Pigeon_Player.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM PigeonGame/Pigeon_Player.cpp $(Debug_Include_Path) > gccDebug/PigeonGame/Pigeon_Player.d

# Compiles file GameRules_WhiteCage.cpp for the Debug configuration...
-include gccDebug/GameRules_WhiteCage.d
gccDebug/GameRules_WhiteCage.o: GameRules_WhiteCage.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GameRules_WhiteCage.cpp $(Debug_Include_Path) -o gccDebug/GameRules_WhiteCage.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GameRules_WhiteCage.cpp $(Debug_Include_Path) > gccDebug/GameRules_WhiteCage.d

# Compiles file npc_cyborg.cpp for the Debug configuration...
-include gccDebug/npc_cyborg.d
gccDebug/npc_cyborg.o: npc_cyborg.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c npc_cyborg.cpp $(Debug_Include_Path) -o gccDebug/npc_cyborg.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM npc_cyborg.cpp $(Debug_Include_Path) > gccDebug/npc_cyborg.d

# Compiles file npc_soldier.cpp for the Debug configuration...
-include gccDebug/npc_soldier.d
gccDebug/npc_soldier.o: npc_soldier.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c npc_soldier.cpp $(Debug_Include_Path) -o gccDebug/npc_soldier.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM npc_soldier.cpp $(Debug_Include_Path) > gccDebug/npc_soldier.d

# Compiles file Player.cpp for the Debug configuration...
-include gccDebug/Player.d
gccDebug/Player.o: Player.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c Player.cpp $(Debug_Include_Path) -o gccDebug/Player.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM Player.cpp $(Debug_Include_Path) > gccDebug/Player.d

# Compiles file viewmodel.cpp for the Debug configuration...
-include gccDebug/viewmodel.d
gccDebug/viewmodel.o: viewmodel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c viewmodel.cpp $(Debug_Include_Path) -o gccDebug/viewmodel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM viewmodel.cpp $(Debug_Include_Path) > gccDebug/viewmodel.d

# Compiles file weapon_ak74.cpp for the Debug configuration...
-include gccDebug/weapon_ak74.d
gccDebug/weapon_ak74.o: weapon_ak74.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c weapon_ak74.cpp $(Debug_Include_Path) -o gccDebug/weapon_ak74.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM weapon_ak74.cpp $(Debug_Include_Path) > gccDebug/weapon_ak74.d

# Compiles file weapon_deagle.cpp for the Debug configuration...
-include gccDebug/weapon_deagle.d
gccDebug/weapon_deagle.o: weapon_deagle.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c weapon_deagle.cpp $(Debug_Include_Path) -o gccDebug/weapon_deagle.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM weapon_deagle.cpp $(Debug_Include_Path) > gccDebug/weapon_deagle.d

# Compiles file weapon_f1.cpp for the Debug configuration...
-include gccDebug/weapon_f1.d
gccDebug/weapon_f1.o: weapon_f1.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c weapon_f1.cpp $(Debug_Include_Path) -o gccDebug/weapon_f1.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM weapon_f1.cpp $(Debug_Include_Path) > gccDebug/weapon_f1.d

# Compiles file weapon_wiremine.cpp for the Debug configuration...
-include gccDebug/weapon_wiremine.d
gccDebug/weapon_wiremine.o: weapon_wiremine.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c weapon_wiremine.cpp $(Debug_Include_Path) -o gccDebug/weapon_wiremine.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM weapon_wiremine.cpp $(Debug_Include_Path) > gccDebug/weapon_wiremine.d

# Compiles file EGUI/EQUI_Manager.cpp for the Debug configuration...
-include gccDebug/EGUI/EQUI_Manager.d
gccDebug/EGUI/EQUI_Manager.o: EGUI/EQUI_Manager.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EGUI/EQUI_Manager.cpp $(Debug_Include_Path) -o gccDebug/EGUI/EQUI_Manager.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EGUI/EQUI_Manager.cpp $(Debug_Include_Path) > gccDebug/EGUI/EQUI_Manager.d

# Compiles file EGUI/EqUI_Panel.cpp for the Debug configuration...
-include gccDebug/EGUI/EqUI_Panel.d
gccDebug/EGUI/EqUI_Panel.o: EGUI/EqUI_Panel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EGUI/EqUI_Panel.cpp $(Debug_Include_Path) -o gccDebug/EGUI/EqUI_Panel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EGUI/EqUI_Panel.cpp $(Debug_Include_Path) > gccDebug/EGUI/EqUI_Panel.d

# Compiles file EntityBind.cpp for the Debug configuration...
-include gccDebug/EntityBind.d
gccDebug/EntityBind.o: EntityBind.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityBind.cpp $(Debug_Include_Path) -o gccDebug/EntityBind.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityBind.cpp $(Debug_Include_Path) > gccDebug/EntityBind.d

# Compiles file EqGMS.cpp for the Debug configuration...
-include gccDebug/EqGMS.d
gccDebug/EqGMS.o: EqGMS.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EqGMS.cpp $(Debug_Include_Path) -o gccDebug/EqGMS.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EqGMS.cpp $(Debug_Include_Path) > gccDebug/EqGMS.d

# Compiles file EngineScriptBind.cpp for the Debug configuration...
-include gccDebug/EngineScriptBind.d
gccDebug/EngineScriptBind.o: EngineScriptBind.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EngineScriptBind.cpp $(Debug_Include_Path) -o gccDebug/EngineScriptBind.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EngineScriptBind.cpp $(Debug_Include_Path) > gccDebug/EngineScriptBind.d

# Compiles file gmScript/gm/gmArraySimple.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmArraySimple.d
gccDebug/gmScript/gm/gmArraySimple.o: gmScript/gm/gmArraySimple.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmArraySimple.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmArraySimple.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmArraySimple.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmArraySimple.d

# Compiles file gmScript/gm/gmByteCode.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmByteCode.d
gccDebug/gmScript/gm/gmByteCode.o: gmScript/gm/gmByteCode.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmByteCode.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmByteCode.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmByteCode.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmByteCode.d

# Compiles file gmScript/gm/gmByteCodeGen.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmByteCodeGen.d
gccDebug/gmScript/gm/gmByteCodeGen.o: gmScript/gm/gmByteCodeGen.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmByteCodeGen.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmByteCodeGen.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmByteCodeGen.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmByteCodeGen.d

# Compiles file gmScript/gm/gmCodeGen.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmCodeGen.d
gccDebug/gmScript/gm/gmCodeGen.o: gmScript/gm/gmCodeGen.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmCodeGen.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmCodeGen.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmCodeGen.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmCodeGen.d

# Compiles file gmScript/gm/gmCodeGenHooks.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmCodeGenHooks.d
gccDebug/gmScript/gm/gmCodeGenHooks.o: gmScript/gm/gmCodeGenHooks.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmCodeGenHooks.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmCodeGenHooks.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmCodeGenHooks.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmCodeGenHooks.d

# Compiles file gmScript/gm/gmCodeTree.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmCodeTree.d
gccDebug/gmScript/gm/gmCodeTree.o: gmScript/gm/gmCodeTree.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmCodeTree.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmCodeTree.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmCodeTree.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmCodeTree.d

# Compiles file gmScript/gm/gmCrc.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmCrc.d
gccDebug/gmScript/gm/gmCrc.o: gmScript/gm/gmCrc.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmCrc.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmCrc.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmCrc.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmCrc.d

# Compiles file gmScript/gm/gmDebug.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmDebug.d
gccDebug/gmScript/gm/gmDebug.o: gmScript/gm/gmDebug.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmDebug.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmDebug.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmDebug.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmDebug.d

# Compiles file gmScript/gm/gmFunctionObject.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmFunctionObject.d
gccDebug/gmScript/gm/gmFunctionObject.o: gmScript/gm/gmFunctionObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmFunctionObject.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmFunctionObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmFunctionObject.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmFunctionObject.d

# Compiles file gmScript/gm/gmHash.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmHash.d
gccDebug/gmScript/gm/gmHash.o: gmScript/gm/gmHash.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmHash.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmHash.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmHash.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmHash.d

# Compiles file gmScript/gm/gmIncGC.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmIncGC.d
gccDebug/gmScript/gm/gmIncGC.o: gmScript/gm/gmIncGC.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmIncGC.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmIncGC.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmIncGC.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmIncGC.d

# Compiles file gmScript/gm/gmLibHooks.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmLibHooks.d
gccDebug/gmScript/gm/gmLibHooks.o: gmScript/gm/gmLibHooks.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmLibHooks.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmLibHooks.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmLibHooks.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmLibHooks.d

# Compiles file gmScript/gm/gmListDouble.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmListDouble.d
gccDebug/gmScript/gm/gmListDouble.o: gmScript/gm/gmListDouble.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmListDouble.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmListDouble.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmListDouble.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmListDouble.d

# Compiles file gmScript/gm/gmLog.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmLog.d
gccDebug/gmScript/gm/gmLog.o: gmScript/gm/gmLog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmLog.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmLog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmLog.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmLog.d

# Compiles file gmScript/gm/gmMachine.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmMachine.d
gccDebug/gmScript/gm/gmMachine.o: gmScript/gm/gmMachine.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmMachine.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmMachine.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmMachine.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmMachine.d

# Compiles file gmScript/gm/gmMachineLib.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmMachineLib.d
gccDebug/gmScript/gm/gmMachineLib.o: gmScript/gm/gmMachineLib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmMachineLib.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmMachineLib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmMachineLib.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmMachineLib.d

# Compiles file gmScript/gm/gmMem.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmMem.d
gccDebug/gmScript/gm/gmMem.o: gmScript/gm/gmMem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmMem.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmMem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmMem.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmMem.d

# Compiles file gmScript/gm/gmMemChain.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmMemChain.d
gccDebug/gmScript/gm/gmMemChain.o: gmScript/gm/gmMemChain.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmMemChain.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmMemChain.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmMemChain.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmMemChain.d

# Compiles file gmScript/gm/gmMemFixed.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmMemFixed.d
gccDebug/gmScript/gm/gmMemFixed.o: gmScript/gm/gmMemFixed.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmMemFixed.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmMemFixed.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmMemFixed.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmMemFixed.d

# Compiles file gmScript/gm/gmMemFixedSet.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmMemFixedSet.d
gccDebug/gmScript/gm/gmMemFixedSet.o: gmScript/gm/gmMemFixedSet.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmMemFixedSet.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmMemFixedSet.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmMemFixedSet.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmMemFixedSet.d

# Compiles file gmScript/gm/gmOperators.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmOperators.d
gccDebug/gmScript/gm/gmOperators.o: gmScript/gm/gmOperators.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmOperators.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmOperators.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmOperators.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmOperators.d

# Compiles file gmScript/gm/gmParser.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmParser.d
gccDebug/gmScript/gm/gmParser.o: gmScript/gm/gmParser.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmParser.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmParser.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmParser.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmParser.d

# Compiles file gmScript/gm/gmScanner.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmScanner.d
gccDebug/gmScript/gm/gmScanner.o: gmScript/gm/gmScanner.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmScanner.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmScanner.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmScanner.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmScanner.d

# Compiles file gmScript/gm/gmStream.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmStream.d
gccDebug/gmScript/gm/gmStream.o: gmScript/gm/gmStream.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmStream.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmStream.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmStream.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmStream.d

# Compiles file gmScript/gm/gmStreamBuffer.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmStreamBuffer.d
gccDebug/gmScript/gm/gmStreamBuffer.o: gmScript/gm/gmStreamBuffer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmStreamBuffer.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmStreamBuffer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmStreamBuffer.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmStreamBuffer.d

# Compiles file gmScript/gm/gmStringObject.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmStringObject.d
gccDebug/gmScript/gm/gmStringObject.o: gmScript/gm/gmStringObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmStringObject.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmStringObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmStringObject.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmStringObject.d

# Compiles file gmScript/gm/gmTableObject.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmTableObject.d
gccDebug/gmScript/gm/gmTableObject.o: gmScript/gm/gmTableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmTableObject.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmTableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmTableObject.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmTableObject.d

# Compiles file gmScript/gm/gmThread.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmThread.d
gccDebug/gmScript/gm/gmThread.o: gmScript/gm/gmThread.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmThread.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmThread.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmThread.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmThread.d

# Compiles file gmScript/gm/gmUserObject.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmUserObject.d
gccDebug/gmScript/gm/gmUserObject.o: gmScript/gm/gmUserObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmUserObject.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmUserObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmUserObject.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmUserObject.d

# Compiles file gmScript/gm/gmUtil.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmUtil.d
gccDebug/gmScript/gm/gmUtil.o: gmScript/gm/gmUtil.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmUtil.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmUtil.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmUtil.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmUtil.d

# Compiles file gmScript/gm/gmVariable.cpp for the Debug configuration...
-include gccDebug/gmScript/gm/gmVariable.d
gccDebug/gmScript/gm/gmVariable.o: gmScript/gm/gmVariable.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/gm/gmVariable.cpp $(Debug_Include_Path) -o gccDebug/gmScript/gm/gmVariable.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/gm/gmVariable.cpp $(Debug_Include_Path) > gccDebug/gmScript/gm/gmVariable.d

# Compiles file gmScript/binds/gmArrayLib.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmArrayLib.d
gccDebug/gmScript/binds/gmArrayLib.o: gmScript/binds/gmArrayLib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmArrayLib.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmArrayLib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmArrayLib.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmArrayLib.d

# Compiles file gmScript/binds/gmCall.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmCall.d
gccDebug/gmScript/binds/gmCall.o: gmScript/binds/gmCall.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmCall.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmCall.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmCall.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmCall.d

# Compiles file gmScript/binds/gmGCRoot.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmGCRoot.d
gccDebug/gmScript/binds/gmGCRoot.o: gmScript/binds/gmGCRoot.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmGCRoot.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmGCRoot.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmGCRoot.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmGCRoot.d

# Compiles file gmScript/binds/gmGCRootUtil.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmGCRootUtil.d
gccDebug/gmScript/binds/gmGCRootUtil.o: gmScript/binds/gmGCRootUtil.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmGCRootUtil.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmGCRootUtil.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmGCRootUtil.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmGCRootUtil.d

# Compiles file gmScript/binds/gmHelpers.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmHelpers.d
gccDebug/gmScript/binds/gmHelpers.o: gmScript/binds/gmHelpers.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmHelpers.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmHelpers.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmHelpers.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmHelpers.d

# Compiles file gmScript/binds/gmMathLib.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmMathLib.d
gccDebug/gmScript/binds/gmMathLib.o: gmScript/binds/gmMathLib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmMathLib.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmMathLib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmMathLib.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmMathLib.d

# Compiles file gmScript/binds/gmStringLib.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmStringLib.d
gccDebug/gmScript/binds/gmStringLib.o: gmScript/binds/gmStringLib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmStringLib.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmStringLib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmStringLib.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmStringLib.d

# Compiles file gmScript/binds/gmSystemLib.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmSystemLib.d
gccDebug/gmScript/binds/gmSystemLib.o: gmScript/binds/gmSystemLib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmSystemLib.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmSystemLib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmSystemLib.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmSystemLib.d

# Compiles file gmScript/binds/gmVector3Lib.cpp for the Debug configuration...
-include gccDebug/gmScript/binds/gmVector3Lib.d
gccDebug/gmScript/binds/gmVector3Lib.o: gmScript/binds/gmVector3Lib.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c gmScript/binds/gmVector3Lib.cpp $(Debug_Include_Path) -o gccDebug/gmScript/binds/gmVector3Lib.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM gmScript/binds/gmVector3Lib.cpp $(Debug_Include_Path) > gccDebug/gmScript/binds/gmVector3Lib.d

# Builds the PigeonGameRelease configuration...
.PHONY: PigeonGameRelease
PigeonGameRelease: create_folders gccPigeonGameRelease/../public/Utils/GeomTools.o gccPigeonGameRelease/../public/Math/math_util.o gccPigeonGameRelease/../public/BaseShader.o gccPigeonGameRelease/../public/BaseRenderableObject.o gccPigeonGameRelease/../public/RenderList.o gccPigeonGameRelease/../public/ViewParams.o gccPigeonGameRelease/../public/dsm_esm_loader.o gccPigeonGameRelease/../public/dsm_loader.o gccPigeonGameRelease/../public/dsm_obj_loader.o gccPigeonGameRelease/GameLibrary.o gccPigeonGameRelease/GameState.o gccPigeonGameRelease/AI/AIBaseNPC.o gccPigeonGameRelease/AI/AINode.o gccPigeonGameRelease/AI/AITaskTypes.o gccPigeonGameRelease/AI/AI_Idle.o gccPigeonGameRelease/AI/AI_MovementTarget.o gccPigeonGameRelease/AI/ai_navigator.o gccPigeonGameRelease/AI/Detour/Source/DetourAlloc.o gccPigeonGameRelease/AI/Detour/Source/DetourCommon.o gccPigeonGameRelease/AI/Detour/Source/DetourNavMesh.o gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshQuery.o gccPigeonGameRelease/AI/Detour/Source/DetourNode.o gccPigeonGameRelease/AI/Recast/Source/Recast.o gccPigeonGameRelease/AI/Recast/Source/RecastAlloc.o gccPigeonGameRelease/AI/Recast/Source/RecastArea.o gccPigeonGameRelease/AI/Recast/Source/RecastContour.o gccPigeonGameRelease/AI/Recast/Source/RecastFilter.o gccPigeonGameRelease/AI/Recast/Source/RecastLayers.o gccPigeonGameRelease/AI/Recast/Source/RecastMesh.o gccPigeonGameRelease/AI/Recast/Source/RecastMeshDetail.o gccPigeonGameRelease/AI/Recast/Source/RecastRasterization.o gccPigeonGameRelease/AI/Recast/Source/RecastRegion.o gccPigeonGameRelease/AI/AIMemoryBase.o gccPigeonGameRelease/AI/AIScriptTask.o gccPigeonGameRelease/AI/AITaskActionBase.o gccPigeonGameRelease/AmmoType.o gccPigeonGameRelease/BaseEntity.o gccPigeonGameRelease/InfoCamera.o gccPigeonGameRelease/Ladder.o gccPigeonGameRelease/LogicEntities.o gccPigeonGameRelease/BulletSimulator.o gccPigeonGameRelease/BaseActor.o gccPigeonGameRelease/BaseAnimating.o gccPigeonGameRelease/BaseWeapon.o gccPigeonGameRelease/EffectEntities.o gccPigeonGameRelease/EngineEntities.o gccPigeonGameRelease/PlayerSpawn.o gccPigeonGameRelease/ModelEntities.o gccPigeonGameRelease/SoundEntities.o gccPigeonGameRelease/Triggers.o gccPigeonGameRelease/../public/anim_activity.o gccPigeonGameRelease/../public/anim_events.o gccPigeonGameRelease/../public/BoneSetup.o gccPigeonGameRelease/../public/ragdoll.o gccPigeonGameRelease/EntityQueue.o gccPigeonGameRelease/EntityDataField.o gccPigeonGameRelease/GameTrace.o gccPigeonGameRelease/Effects.o gccPigeonGameRelease/GameInput.o gccPigeonGameRelease/GlobalVarsGame.o gccPigeonGameRelease/MaterialProxy.o gccPigeonGameRelease/Rain.o gccPigeonGameRelease/Snow.o gccPigeonGameRelease/SaveGame_Events.o gccPigeonGameRelease/SaveRestoreManager.o gccPigeonGameRelease/EffectRender.o gccPigeonGameRelease/FilterPipelineBase.o gccPigeonGameRelease/GameRenderer.o gccPigeonGameRelease/ParticleRenderer.o gccPigeonGameRelease/RenderDefs.o gccPigeonGameRelease/../public/GameSoundEmitterSystem.o gccPigeonGameRelease/PigeonGame/BasePigeon.o gccPigeonGameRelease/PigeonGame/GameRules_Pigeon.o gccPigeonGameRelease/PigeonGame/Pigeon_Player.o gccPigeonGameRelease/GameRules_WhiteCage.o gccPigeonGameRelease/npc_cyborg.o gccPigeonGameRelease/npc_soldier.o gccPigeonGameRelease/Player.o gccPigeonGameRelease/viewmodel.o gccPigeonGameRelease/weapon_ak74.o gccPigeonGameRelease/weapon_deagle.o gccPigeonGameRelease/weapon_f1.o gccPigeonGameRelease/weapon_wiremine.o gccPigeonGameRelease/EGUI/EQUI_Manager.o gccPigeonGameRelease/EGUI/EqUI_Panel.o gccPigeonGameRelease/EntityBind.o gccPigeonGameRelease/EqGMS.o gccPigeonGameRelease/EngineScriptBind.o gccPigeonGameRelease/gmScript/gm/gmArraySimple.o gccPigeonGameRelease/gmScript/gm/gmByteCode.o gccPigeonGameRelease/gmScript/gm/gmByteCodeGen.o gccPigeonGameRelease/gmScript/gm/gmCodeGen.o gccPigeonGameRelease/gmScript/gm/gmCodeGenHooks.o gccPigeonGameRelease/gmScript/gm/gmCodeTree.o gccPigeonGameRelease/gmScript/gm/gmCrc.o gccPigeonGameRelease/gmScript/gm/gmDebug.o gccPigeonGameRelease/gmScript/gm/gmFunctionObject.o gccPigeonGameRelease/gmScript/gm/gmHash.o gccPigeonGameRelease/gmScript/gm/gmIncGC.o gccPigeonGameRelease/gmScript/gm/gmLibHooks.o gccPigeonGameRelease/gmScript/gm/gmListDouble.o gccPigeonGameRelease/gmScript/gm/gmLog.o gccPigeonGameRelease/gmScript/gm/gmMachine.o gccPigeonGameRelease/gmScript/gm/gmMachineLib.o gccPigeonGameRelease/gmScript/gm/gmMem.o gccPigeonGameRelease/gmScript/gm/gmMemChain.o gccPigeonGameRelease/gmScript/gm/gmMemFixed.o gccPigeonGameRelease/gmScript/gm/gmMemFixedSet.o gccPigeonGameRelease/gmScript/gm/gmOperators.o gccPigeonGameRelease/gmScript/gm/gmParser.o gccPigeonGameRelease/gmScript/gm/gmScanner.o gccPigeonGameRelease/gmScript/gm/gmStream.o gccPigeonGameRelease/gmScript/gm/gmStreamBuffer.o gccPigeonGameRelease/gmScript/gm/gmStringObject.o gccPigeonGameRelease/gmScript/gm/gmTableObject.o gccPigeonGameRelease/gmScript/gm/gmThread.o gccPigeonGameRelease/gmScript/gm/gmUserObject.o gccPigeonGameRelease/gmScript/gm/gmUtil.o gccPigeonGameRelease/gmScript/gm/gmVariable.o gccPigeonGameRelease/gmScript/binds/gmArrayLib.o gccPigeonGameRelease/gmScript/binds/gmCall.o gccPigeonGameRelease/gmScript/binds/gmGCRoot.o gccPigeonGameRelease/gmScript/binds/gmGCRootUtil.o gccPigeonGameRelease/gmScript/binds/gmHelpers.o gccPigeonGameRelease/gmScript/binds/gmMathLib.o gccPigeonGameRelease/gmScript/binds/gmStringLib.o gccPigeonGameRelease/gmScript/binds/gmSystemLib.o gccPigeonGameRelease/gmScript/binds/gmVector3Lib.o 
	g++ -fPIC -shared -Wl,-soname,libGameDLL.so -o gccPigeonGameRelease/libGameDLL.so gccPigeonGameRelease/../public/Utils/GeomTools.o gccPigeonGameRelease/../public/Math/math_util.o gccPigeonGameRelease/../public/BaseShader.o gccPigeonGameRelease/../public/BaseRenderableObject.o gccPigeonGameRelease/../public/RenderList.o gccPigeonGameRelease/../public/ViewParams.o gccPigeonGameRelease/../public/dsm_esm_loader.o gccPigeonGameRelease/../public/dsm_loader.o gccPigeonGameRelease/../public/dsm_obj_loader.o gccPigeonGameRelease/GameLibrary.o gccPigeonGameRelease/GameState.o gccPigeonGameRelease/AI/AIBaseNPC.o gccPigeonGameRelease/AI/AINode.o gccPigeonGameRelease/AI/AITaskTypes.o gccPigeonGameRelease/AI/AI_Idle.o gccPigeonGameRelease/AI/AI_MovementTarget.o gccPigeonGameRelease/AI/ai_navigator.o gccPigeonGameRelease/AI/Detour/Source/DetourAlloc.o gccPigeonGameRelease/AI/Detour/Source/DetourCommon.o gccPigeonGameRelease/AI/Detour/Source/DetourNavMesh.o gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshQuery.o gccPigeonGameRelease/AI/Detour/Source/DetourNode.o gccPigeonGameRelease/AI/Recast/Source/Recast.o gccPigeonGameRelease/AI/Recast/Source/RecastAlloc.o gccPigeonGameRelease/AI/Recast/Source/RecastArea.o gccPigeonGameRelease/AI/Recast/Source/RecastContour.o gccPigeonGameRelease/AI/Recast/Source/RecastFilter.o gccPigeonGameRelease/AI/Recast/Source/RecastLayers.o gccPigeonGameRelease/AI/Recast/Source/RecastMesh.o gccPigeonGameRelease/AI/Recast/Source/RecastMeshDetail.o gccPigeonGameRelease/AI/Recast/Source/RecastRasterization.o gccPigeonGameRelease/AI/Recast/Source/RecastRegion.o gccPigeonGameRelease/AI/AIMemoryBase.o gccPigeonGameRelease/AI/AIScriptTask.o gccPigeonGameRelease/AI/AITaskActionBase.o gccPigeonGameRelease/AmmoType.o gccPigeonGameRelease/BaseEntity.o gccPigeonGameRelease/InfoCamera.o gccPigeonGameRelease/Ladder.o gccPigeonGameRelease/LogicEntities.o gccPigeonGameRelease/BulletSimulator.o gccPigeonGameRelease/BaseActor.o gccPigeonGameRelease/BaseAnimating.o gccPigeonGameRelease/BaseWeapon.o gccPigeonGameRelease/EffectEntities.o gccPigeonGameRelease/EngineEntities.o gccPigeonGameRelease/PlayerSpawn.o gccPigeonGameRelease/ModelEntities.o gccPigeonGameRelease/SoundEntities.o gccPigeonGameRelease/Triggers.o gccPigeonGameRelease/../public/anim_activity.o gccPigeonGameRelease/../public/anim_events.o gccPigeonGameRelease/../public/BoneSetup.o gccPigeonGameRelease/../public/ragdoll.o gccPigeonGameRelease/EntityQueue.o gccPigeonGameRelease/EntityDataField.o gccPigeonGameRelease/GameTrace.o gccPigeonGameRelease/Effects.o gccPigeonGameRelease/GameInput.o gccPigeonGameRelease/GlobalVarsGame.o gccPigeonGameRelease/MaterialProxy.o gccPigeonGameRelease/Rain.o gccPigeonGameRelease/Snow.o gccPigeonGameRelease/SaveGame_Events.o gccPigeonGameRelease/SaveRestoreManager.o gccPigeonGameRelease/EffectRender.o gccPigeonGameRelease/FilterPipelineBase.o gccPigeonGameRelease/GameRenderer.o gccPigeonGameRelease/ParticleRenderer.o gccPigeonGameRelease/RenderDefs.o gccPigeonGameRelease/../public/GameSoundEmitterSystem.o gccPigeonGameRelease/PigeonGame/BasePigeon.o gccPigeonGameRelease/PigeonGame/GameRules_Pigeon.o gccPigeonGameRelease/PigeonGame/Pigeon_Player.o gccPigeonGameRelease/GameRules_WhiteCage.o gccPigeonGameRelease/npc_cyborg.o gccPigeonGameRelease/npc_soldier.o gccPigeonGameRelease/Player.o gccPigeonGameRelease/viewmodel.o gccPigeonGameRelease/weapon_ak74.o gccPigeonGameRelease/weapon_deagle.o gccPigeonGameRelease/weapon_f1.o gccPigeonGameRelease/weapon_wiremine.o gccPigeonGameRelease/EGUI/EQUI_Manager.o gccPigeonGameRelease/EGUI/EqUI_Panel.o gccPigeonGameRelease/EntityBind.o gccPigeonGameRelease/EqGMS.o gccPigeonGameRelease/EngineScriptBind.o gccPigeonGameRelease/gmScript/gm/gmArraySimple.o gccPigeonGameRelease/gmScript/gm/gmByteCode.o gccPigeonGameRelease/gmScript/gm/gmByteCodeGen.o gccPigeonGameRelease/gmScript/gm/gmCodeGen.o gccPigeonGameRelease/gmScript/gm/gmCodeGenHooks.o gccPigeonGameRelease/gmScript/gm/gmCodeTree.o gccPigeonGameRelease/gmScript/gm/gmCrc.o gccPigeonGameRelease/gmScript/gm/gmDebug.o gccPigeonGameRelease/gmScript/gm/gmFunctionObject.o gccPigeonGameRelease/gmScript/gm/gmHash.o gccPigeonGameRelease/gmScript/gm/gmIncGC.o gccPigeonGameRelease/gmScript/gm/gmLibHooks.o gccPigeonGameRelease/gmScript/gm/gmListDouble.o gccPigeonGameRelease/gmScript/gm/gmLog.o gccPigeonGameRelease/gmScript/gm/gmMachine.o gccPigeonGameRelease/gmScript/gm/gmMachineLib.o gccPigeonGameRelease/gmScript/gm/gmMem.o gccPigeonGameRelease/gmScript/gm/gmMemChain.o gccPigeonGameRelease/gmScript/gm/gmMemFixed.o gccPigeonGameRelease/gmScript/gm/gmMemFixedSet.o gccPigeonGameRelease/gmScript/gm/gmOperators.o gccPigeonGameRelease/gmScript/gm/gmParser.o gccPigeonGameRelease/gmScript/gm/gmScanner.o gccPigeonGameRelease/gmScript/gm/gmStream.o gccPigeonGameRelease/gmScript/gm/gmStreamBuffer.o gccPigeonGameRelease/gmScript/gm/gmStringObject.o gccPigeonGameRelease/gmScript/gm/gmTableObject.o gccPigeonGameRelease/gmScript/gm/gmThread.o gccPigeonGameRelease/gmScript/gm/gmUserObject.o gccPigeonGameRelease/gmScript/gm/gmUtil.o gccPigeonGameRelease/gmScript/gm/gmVariable.o gccPigeonGameRelease/gmScript/binds/gmArrayLib.o gccPigeonGameRelease/gmScript/binds/gmCall.o gccPigeonGameRelease/gmScript/binds/gmGCRoot.o gccPigeonGameRelease/gmScript/binds/gmGCRootUtil.o gccPigeonGameRelease/gmScript/binds/gmHelpers.o gccPigeonGameRelease/gmScript/binds/gmMathLib.o gccPigeonGameRelease/gmScript/binds/gmStringLib.o gccPigeonGameRelease/gmScript/binds/gmSystemLib.o gccPigeonGameRelease/gmScript/binds/gmVector3Lib.o  $(PigeonGameRelease_Implicitly_Linked_Objects)

# Compiles file ../public/Utils/GeomTools.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/Utils/GeomTools.d
gccPigeonGameRelease/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/Utils/GeomTools.d

# Compiles file ../public/Math/math_util.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/Math/math_util.d
gccPigeonGameRelease/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/Math/math_util.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/Math/math_util.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/Math/math_util.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/Math/math_util.d

# Compiles file ../public/BaseShader.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/BaseShader.d
gccPigeonGameRelease/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/BaseShader.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/BaseShader.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/BaseShader.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/BaseShader.d

# Compiles file ../public/BaseRenderableObject.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/BaseRenderableObject.d
gccPigeonGameRelease/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/BaseRenderableObject.d

# Compiles file ../public/RenderList.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/RenderList.d
gccPigeonGameRelease/../public/RenderList.o: ../public/RenderList.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/RenderList.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/RenderList.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/RenderList.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/RenderList.d

# Compiles file ../public/ViewParams.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/ViewParams.d
gccPigeonGameRelease/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/ViewParams.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/ViewParams.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/ViewParams.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/ViewParams.d

# Compiles file ../public/dsm_esm_loader.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/dsm_esm_loader.d
gccPigeonGameRelease/../public/dsm_esm_loader.o: ../public/dsm_esm_loader.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/dsm_esm_loader.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/dsm_esm_loader.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/dsm_esm_loader.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/dsm_esm_loader.d

# Compiles file ../public/dsm_loader.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/dsm_loader.d
gccPigeonGameRelease/../public/dsm_loader.o: ../public/dsm_loader.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/dsm_loader.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/dsm_loader.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/dsm_loader.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/dsm_loader.d

# Compiles file ../public/dsm_obj_loader.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/dsm_obj_loader.d
gccPigeonGameRelease/../public/dsm_obj_loader.o: ../public/dsm_obj_loader.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/dsm_obj_loader.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/dsm_obj_loader.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/dsm_obj_loader.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/dsm_obj_loader.d

# Compiles file GameLibrary.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GameLibrary.d
gccPigeonGameRelease/GameLibrary.o: GameLibrary.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GameLibrary.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GameLibrary.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GameLibrary.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GameLibrary.d

# Compiles file GameState.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GameState.d
gccPigeonGameRelease/GameState.o: GameState.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GameState.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GameState.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GameState.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GameState.d

# Compiles file AI/AIBaseNPC.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AIBaseNPC.d
gccPigeonGameRelease/AI/AIBaseNPC.o: AI/AIBaseNPC.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AIBaseNPC.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AIBaseNPC.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AIBaseNPC.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AIBaseNPC.d

# Compiles file AI/AINode.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AINode.d
gccPigeonGameRelease/AI/AINode.o: AI/AINode.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AINode.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AINode.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AINode.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AINode.d

# Compiles file AI/AITaskTypes.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AITaskTypes.d
gccPigeonGameRelease/AI/AITaskTypes.o: AI/AITaskTypes.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AITaskTypes.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AITaskTypes.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AITaskTypes.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AITaskTypes.d

# Compiles file AI/AI_Idle.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AI_Idle.d
gccPigeonGameRelease/AI/AI_Idle.o: AI/AI_Idle.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AI_Idle.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AI_Idle.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AI_Idle.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AI_Idle.d

# Compiles file AI/AI_MovementTarget.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AI_MovementTarget.d
gccPigeonGameRelease/AI/AI_MovementTarget.o: AI/AI_MovementTarget.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AI_MovementTarget.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AI_MovementTarget.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AI_MovementTarget.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AI_MovementTarget.d

# Compiles file AI/ai_navigator.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/ai_navigator.d
gccPigeonGameRelease/AI/ai_navigator.o: AI/ai_navigator.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/ai_navigator.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/ai_navigator.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/ai_navigator.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/ai_navigator.d

# Compiles file AI/Detour/Source/DetourAlloc.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Detour/Source/DetourAlloc.d
gccPigeonGameRelease/AI/Detour/Source/DetourAlloc.o: AI/Detour/Source/DetourAlloc.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourAlloc.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Detour/Source/DetourAlloc.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourAlloc.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Detour/Source/DetourAlloc.d

# Compiles file AI/Detour/Source/DetourCommon.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Detour/Source/DetourCommon.d
gccPigeonGameRelease/AI/Detour/Source/DetourCommon.o: AI/Detour/Source/DetourCommon.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourCommon.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Detour/Source/DetourCommon.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourCommon.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Detour/Source/DetourCommon.d

# Compiles file AI/Detour/Source/DetourNavMesh.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Detour/Source/DetourNavMesh.d
gccPigeonGameRelease/AI/Detour/Source/DetourNavMesh.o: AI/Detour/Source/DetourNavMesh.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNavMesh.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Detour/Source/DetourNavMesh.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNavMesh.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Detour/Source/DetourNavMesh.d

# Compiles file AI/Detour/Source/DetourNavMeshBuilder.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshBuilder.d
gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o: AI/Detour/Source/DetourNavMeshBuilder.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshBuilder.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshBuilder.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshBuilder.d

# Compiles file AI/Detour/Source/DetourNavMeshQuery.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshQuery.d
gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshQuery.o: AI/Detour/Source/DetourNavMeshQuery.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshQuery.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshQuery.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshQuery.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Detour/Source/DetourNavMeshQuery.d

# Compiles file AI/Detour/Source/DetourNode.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Detour/Source/DetourNode.d
gccPigeonGameRelease/AI/Detour/Source/DetourNode.o: AI/Detour/Source/DetourNode.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNode.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Detour/Source/DetourNode.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNode.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Detour/Source/DetourNode.d

# Compiles file AI/Recast/Source/Recast.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/Recast.d
gccPigeonGameRelease/AI/Recast/Source/Recast.o: AI/Recast/Source/Recast.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/Recast.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/Recast.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/Recast.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/Recast.d

# Compiles file AI/Recast/Source/RecastAlloc.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastAlloc.d
gccPigeonGameRelease/AI/Recast/Source/RecastAlloc.o: AI/Recast/Source/RecastAlloc.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastAlloc.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastAlloc.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastAlloc.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastAlloc.d

# Compiles file AI/Recast/Source/RecastArea.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastArea.d
gccPigeonGameRelease/AI/Recast/Source/RecastArea.o: AI/Recast/Source/RecastArea.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastArea.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastArea.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastArea.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastArea.d

# Compiles file AI/Recast/Source/RecastContour.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastContour.d
gccPigeonGameRelease/AI/Recast/Source/RecastContour.o: AI/Recast/Source/RecastContour.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastContour.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastContour.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastContour.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastContour.d

# Compiles file AI/Recast/Source/RecastFilter.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastFilter.d
gccPigeonGameRelease/AI/Recast/Source/RecastFilter.o: AI/Recast/Source/RecastFilter.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastFilter.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastFilter.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastFilter.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastFilter.d

# Compiles file AI/Recast/Source/RecastLayers.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastLayers.d
gccPigeonGameRelease/AI/Recast/Source/RecastLayers.o: AI/Recast/Source/RecastLayers.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastLayers.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastLayers.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastLayers.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastLayers.d

# Compiles file AI/Recast/Source/RecastMesh.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastMesh.d
gccPigeonGameRelease/AI/Recast/Source/RecastMesh.o: AI/Recast/Source/RecastMesh.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastMesh.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastMesh.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastMesh.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastMesh.d

# Compiles file AI/Recast/Source/RecastMeshDetail.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastMeshDetail.d
gccPigeonGameRelease/AI/Recast/Source/RecastMeshDetail.o: AI/Recast/Source/RecastMeshDetail.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastMeshDetail.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastMeshDetail.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastMeshDetail.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastMeshDetail.d

# Compiles file AI/Recast/Source/RecastRasterization.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastRasterization.d
gccPigeonGameRelease/AI/Recast/Source/RecastRasterization.o: AI/Recast/Source/RecastRasterization.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastRasterization.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastRasterization.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastRasterization.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastRasterization.d

# Compiles file AI/Recast/Source/RecastRegion.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/Recast/Source/RecastRegion.d
gccPigeonGameRelease/AI/Recast/Source/RecastRegion.o: AI/Recast/Source/RecastRegion.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastRegion.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/Recast/Source/RecastRegion.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastRegion.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/Recast/Source/RecastRegion.d

# Compiles file AI/AIMemoryBase.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AIMemoryBase.d
gccPigeonGameRelease/AI/AIMemoryBase.o: AI/AIMemoryBase.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AIMemoryBase.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AIMemoryBase.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AIMemoryBase.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AIMemoryBase.d

# Compiles file AI/AIScriptTask.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AIScriptTask.d
gccPigeonGameRelease/AI/AIScriptTask.o: AI/AIScriptTask.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AIScriptTask.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AIScriptTask.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AIScriptTask.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AIScriptTask.d

# Compiles file AI/AITaskActionBase.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AI/AITaskActionBase.d
gccPigeonGameRelease/AI/AITaskActionBase.o: AI/AITaskActionBase.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AI/AITaskActionBase.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AI/AITaskActionBase.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AI/AITaskActionBase.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AI/AITaskActionBase.d

# Compiles file AmmoType.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/AmmoType.d
gccPigeonGameRelease/AmmoType.o: AmmoType.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c AmmoType.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/AmmoType.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM AmmoType.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/AmmoType.d

# Compiles file BaseEntity.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/BaseEntity.d
gccPigeonGameRelease/BaseEntity.o: BaseEntity.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c BaseEntity.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/BaseEntity.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM BaseEntity.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/BaseEntity.d

# Compiles file InfoCamera.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/InfoCamera.d
gccPigeonGameRelease/InfoCamera.o: InfoCamera.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c InfoCamera.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/InfoCamera.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM InfoCamera.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/InfoCamera.d

# Compiles file Ladder.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/Ladder.d
gccPigeonGameRelease/Ladder.o: Ladder.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c Ladder.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/Ladder.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM Ladder.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/Ladder.d

# Compiles file LogicEntities.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/LogicEntities.d
gccPigeonGameRelease/LogicEntities.o: LogicEntities.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c LogicEntities.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/LogicEntities.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM LogicEntities.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/LogicEntities.d

# Compiles file BulletSimulator.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/BulletSimulator.d
gccPigeonGameRelease/BulletSimulator.o: BulletSimulator.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c BulletSimulator.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/BulletSimulator.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM BulletSimulator.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/BulletSimulator.d

# Compiles file BaseActor.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/BaseActor.d
gccPigeonGameRelease/BaseActor.o: BaseActor.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c BaseActor.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/BaseActor.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM BaseActor.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/BaseActor.d

# Compiles file BaseAnimating.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/BaseAnimating.d
gccPigeonGameRelease/BaseAnimating.o: BaseAnimating.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c BaseAnimating.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/BaseAnimating.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM BaseAnimating.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/BaseAnimating.d

# Compiles file BaseWeapon.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/BaseWeapon.d
gccPigeonGameRelease/BaseWeapon.o: BaseWeapon.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c BaseWeapon.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/BaseWeapon.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM BaseWeapon.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/BaseWeapon.d

# Compiles file EffectEntities.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EffectEntities.d
gccPigeonGameRelease/EffectEntities.o: EffectEntities.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EffectEntities.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EffectEntities.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EffectEntities.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EffectEntities.d

# Compiles file EngineEntities.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EngineEntities.d
gccPigeonGameRelease/EngineEntities.o: EngineEntities.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EngineEntities.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EngineEntities.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EngineEntities.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EngineEntities.d

# Compiles file PlayerSpawn.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/PlayerSpawn.d
gccPigeonGameRelease/PlayerSpawn.o: PlayerSpawn.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c PlayerSpawn.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/PlayerSpawn.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM PlayerSpawn.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/PlayerSpawn.d

# Compiles file ModelEntities.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/ModelEntities.d
gccPigeonGameRelease/ModelEntities.o: ModelEntities.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ModelEntities.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/ModelEntities.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ModelEntities.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/ModelEntities.d

# Compiles file SoundEntities.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/SoundEntities.d
gccPigeonGameRelease/SoundEntities.o: SoundEntities.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c SoundEntities.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/SoundEntities.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM SoundEntities.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/SoundEntities.d

# Compiles file Triggers.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/Triggers.d
gccPigeonGameRelease/Triggers.o: Triggers.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c Triggers.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/Triggers.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM Triggers.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/Triggers.d

# Compiles file ../public/anim_activity.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/anim_activity.d
gccPigeonGameRelease/../public/anim_activity.o: ../public/anim_activity.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/anim_activity.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/anim_activity.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/anim_activity.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/anim_activity.d

# Compiles file ../public/anim_events.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/anim_events.d
gccPigeonGameRelease/../public/anim_events.o: ../public/anim_events.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/anim_events.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/anim_events.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/anim_events.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/anim_events.d

# Compiles file ../public/BoneSetup.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/BoneSetup.d
gccPigeonGameRelease/../public/BoneSetup.o: ../public/BoneSetup.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/BoneSetup.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/BoneSetup.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/BoneSetup.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/BoneSetup.d

# Compiles file ../public/ragdoll.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/ragdoll.d
gccPigeonGameRelease/../public/ragdoll.o: ../public/ragdoll.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/ragdoll.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/ragdoll.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/ragdoll.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/ragdoll.d

# Compiles file EntityQueue.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EntityQueue.d
gccPigeonGameRelease/EntityQueue.o: EntityQueue.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EntityQueue.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EntityQueue.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EntityQueue.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EntityQueue.d

# Compiles file EntityDataField.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EntityDataField.d
gccPigeonGameRelease/EntityDataField.o: EntityDataField.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EntityDataField.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EntityDataField.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EntityDataField.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EntityDataField.d

# Compiles file GameTrace.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GameTrace.d
gccPigeonGameRelease/GameTrace.o: GameTrace.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GameTrace.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GameTrace.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GameTrace.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GameTrace.d

# Compiles file Effects.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/Effects.d
gccPigeonGameRelease/Effects.o: Effects.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c Effects.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/Effects.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM Effects.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/Effects.d

# Compiles file GameInput.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GameInput.d
gccPigeonGameRelease/GameInput.o: GameInput.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GameInput.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GameInput.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GameInput.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GameInput.d

# Compiles file GlobalVarsGame.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GlobalVarsGame.d
gccPigeonGameRelease/GlobalVarsGame.o: GlobalVarsGame.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GlobalVarsGame.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GlobalVarsGame.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GlobalVarsGame.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GlobalVarsGame.d

# Compiles file MaterialProxy.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/MaterialProxy.d
gccPigeonGameRelease/MaterialProxy.o: MaterialProxy.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c MaterialProxy.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/MaterialProxy.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM MaterialProxy.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/MaterialProxy.d

# Compiles file Rain.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/Rain.d
gccPigeonGameRelease/Rain.o: Rain.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c Rain.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/Rain.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM Rain.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/Rain.d

# Compiles file Snow.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/Snow.d
gccPigeonGameRelease/Snow.o: Snow.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c Snow.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/Snow.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM Snow.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/Snow.d

# Compiles file SaveGame_Events.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/SaveGame_Events.d
gccPigeonGameRelease/SaveGame_Events.o: SaveGame_Events.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c SaveGame_Events.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/SaveGame_Events.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM SaveGame_Events.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/SaveGame_Events.d

# Compiles file SaveRestoreManager.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/SaveRestoreManager.d
gccPigeonGameRelease/SaveRestoreManager.o: SaveRestoreManager.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c SaveRestoreManager.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/SaveRestoreManager.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM SaveRestoreManager.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/SaveRestoreManager.d

# Compiles file EffectRender.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EffectRender.d
gccPigeonGameRelease/EffectRender.o: EffectRender.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EffectRender.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EffectRender.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EffectRender.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EffectRender.d

# Compiles file FilterPipelineBase.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/FilterPipelineBase.d
gccPigeonGameRelease/FilterPipelineBase.o: FilterPipelineBase.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c FilterPipelineBase.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/FilterPipelineBase.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM FilterPipelineBase.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/FilterPipelineBase.d

# Compiles file GameRenderer.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GameRenderer.d
gccPigeonGameRelease/GameRenderer.o: GameRenderer.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GameRenderer.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GameRenderer.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GameRenderer.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GameRenderer.d

# Compiles file ParticleRenderer.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/ParticleRenderer.d
gccPigeonGameRelease/ParticleRenderer.o: ParticleRenderer.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ParticleRenderer.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/ParticleRenderer.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ParticleRenderer.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/ParticleRenderer.d

# Compiles file RenderDefs.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/RenderDefs.d
gccPigeonGameRelease/RenderDefs.o: RenderDefs.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c RenderDefs.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/RenderDefs.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM RenderDefs.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/RenderDefs.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/../public/GameSoundEmitterSystem.d
gccPigeonGameRelease/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/../public/GameSoundEmitterSystem.d

# Compiles file PigeonGame/BasePigeon.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/PigeonGame/BasePigeon.d
gccPigeonGameRelease/PigeonGame/BasePigeon.o: PigeonGame/BasePigeon.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c PigeonGame/BasePigeon.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/PigeonGame/BasePigeon.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM PigeonGame/BasePigeon.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/PigeonGame/BasePigeon.d

# Compiles file PigeonGame/GameRules_Pigeon.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/PigeonGame/GameRules_Pigeon.d
gccPigeonGameRelease/PigeonGame/GameRules_Pigeon.o: PigeonGame/GameRules_Pigeon.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c PigeonGame/GameRules_Pigeon.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/PigeonGame/GameRules_Pigeon.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM PigeonGame/GameRules_Pigeon.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/PigeonGame/GameRules_Pigeon.d

# Compiles file PigeonGame/Pigeon_Player.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/PigeonGame/Pigeon_Player.d
gccPigeonGameRelease/PigeonGame/Pigeon_Player.o: PigeonGame/Pigeon_Player.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c PigeonGame/Pigeon_Player.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/PigeonGame/Pigeon_Player.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM PigeonGame/Pigeon_Player.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/PigeonGame/Pigeon_Player.d

# Compiles file GameRules_WhiteCage.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/GameRules_WhiteCage.d
gccPigeonGameRelease/GameRules_WhiteCage.o: GameRules_WhiteCage.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c GameRules_WhiteCage.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/GameRules_WhiteCage.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM GameRules_WhiteCage.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/GameRules_WhiteCage.d

# Compiles file npc_cyborg.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/npc_cyborg.d
gccPigeonGameRelease/npc_cyborg.o: npc_cyborg.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c npc_cyborg.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/npc_cyborg.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM npc_cyborg.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/npc_cyborg.d

# Compiles file npc_soldier.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/npc_soldier.d
gccPigeonGameRelease/npc_soldier.o: npc_soldier.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c npc_soldier.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/npc_soldier.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM npc_soldier.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/npc_soldier.d

# Compiles file Player.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/Player.d
gccPigeonGameRelease/Player.o: Player.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c Player.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/Player.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM Player.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/Player.d

# Compiles file viewmodel.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/viewmodel.d
gccPigeonGameRelease/viewmodel.o: viewmodel.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c viewmodel.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/viewmodel.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM viewmodel.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/viewmodel.d

# Compiles file weapon_ak74.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/weapon_ak74.d
gccPigeonGameRelease/weapon_ak74.o: weapon_ak74.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c weapon_ak74.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/weapon_ak74.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM weapon_ak74.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/weapon_ak74.d

# Compiles file weapon_deagle.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/weapon_deagle.d
gccPigeonGameRelease/weapon_deagle.o: weapon_deagle.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c weapon_deagle.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/weapon_deagle.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM weapon_deagle.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/weapon_deagle.d

# Compiles file weapon_f1.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/weapon_f1.d
gccPigeonGameRelease/weapon_f1.o: weapon_f1.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c weapon_f1.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/weapon_f1.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM weapon_f1.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/weapon_f1.d

# Compiles file weapon_wiremine.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/weapon_wiremine.d
gccPigeonGameRelease/weapon_wiremine.o: weapon_wiremine.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c weapon_wiremine.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/weapon_wiremine.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM weapon_wiremine.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/weapon_wiremine.d

# Compiles file EGUI/EQUI_Manager.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EGUI/EQUI_Manager.d
gccPigeonGameRelease/EGUI/EQUI_Manager.o: EGUI/EQUI_Manager.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EGUI/EQUI_Manager.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EGUI/EQUI_Manager.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EGUI/EQUI_Manager.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EGUI/EQUI_Manager.d

# Compiles file EGUI/EqUI_Panel.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EGUI/EqUI_Panel.d
gccPigeonGameRelease/EGUI/EqUI_Panel.o: EGUI/EqUI_Panel.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EGUI/EqUI_Panel.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EGUI/EqUI_Panel.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EGUI/EqUI_Panel.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EGUI/EqUI_Panel.d

# Compiles file EntityBind.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EntityBind.d
gccPigeonGameRelease/EntityBind.o: EntityBind.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EntityBind.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EntityBind.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EntityBind.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EntityBind.d

# Compiles file EqGMS.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EqGMS.d
gccPigeonGameRelease/EqGMS.o: EqGMS.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EqGMS.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EqGMS.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EqGMS.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EqGMS.d

# Compiles file EngineScriptBind.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/EngineScriptBind.d
gccPigeonGameRelease/EngineScriptBind.o: EngineScriptBind.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c EngineScriptBind.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/EngineScriptBind.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM EngineScriptBind.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/EngineScriptBind.d

# Compiles file gmScript/gm/gmArraySimple.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmArraySimple.d
gccPigeonGameRelease/gmScript/gm/gmArraySimple.o: gmScript/gm/gmArraySimple.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmArraySimple.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmArraySimple.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmArraySimple.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmArraySimple.d

# Compiles file gmScript/gm/gmByteCode.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmByteCode.d
gccPigeonGameRelease/gmScript/gm/gmByteCode.o: gmScript/gm/gmByteCode.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmByteCode.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmByteCode.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmByteCode.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmByteCode.d

# Compiles file gmScript/gm/gmByteCodeGen.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmByteCodeGen.d
gccPigeonGameRelease/gmScript/gm/gmByteCodeGen.o: gmScript/gm/gmByteCodeGen.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmByteCodeGen.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmByteCodeGen.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmByteCodeGen.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmByteCodeGen.d

# Compiles file gmScript/gm/gmCodeGen.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmCodeGen.d
gccPigeonGameRelease/gmScript/gm/gmCodeGen.o: gmScript/gm/gmCodeGen.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmCodeGen.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmCodeGen.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmCodeGen.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmCodeGen.d

# Compiles file gmScript/gm/gmCodeGenHooks.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmCodeGenHooks.d
gccPigeonGameRelease/gmScript/gm/gmCodeGenHooks.o: gmScript/gm/gmCodeGenHooks.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmCodeGenHooks.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmCodeGenHooks.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmCodeGenHooks.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmCodeGenHooks.d

# Compiles file gmScript/gm/gmCodeTree.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmCodeTree.d
gccPigeonGameRelease/gmScript/gm/gmCodeTree.o: gmScript/gm/gmCodeTree.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmCodeTree.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmCodeTree.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmCodeTree.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmCodeTree.d

# Compiles file gmScript/gm/gmCrc.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmCrc.d
gccPigeonGameRelease/gmScript/gm/gmCrc.o: gmScript/gm/gmCrc.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmCrc.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmCrc.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmCrc.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmCrc.d

# Compiles file gmScript/gm/gmDebug.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmDebug.d
gccPigeonGameRelease/gmScript/gm/gmDebug.o: gmScript/gm/gmDebug.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmDebug.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmDebug.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmDebug.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmDebug.d

# Compiles file gmScript/gm/gmFunctionObject.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmFunctionObject.d
gccPigeonGameRelease/gmScript/gm/gmFunctionObject.o: gmScript/gm/gmFunctionObject.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmFunctionObject.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmFunctionObject.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmFunctionObject.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmFunctionObject.d

# Compiles file gmScript/gm/gmHash.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmHash.d
gccPigeonGameRelease/gmScript/gm/gmHash.o: gmScript/gm/gmHash.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmHash.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmHash.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmHash.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmHash.d

# Compiles file gmScript/gm/gmIncGC.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmIncGC.d
gccPigeonGameRelease/gmScript/gm/gmIncGC.o: gmScript/gm/gmIncGC.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmIncGC.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmIncGC.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmIncGC.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmIncGC.d

# Compiles file gmScript/gm/gmLibHooks.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmLibHooks.d
gccPigeonGameRelease/gmScript/gm/gmLibHooks.o: gmScript/gm/gmLibHooks.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmLibHooks.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmLibHooks.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmLibHooks.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmLibHooks.d

# Compiles file gmScript/gm/gmListDouble.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmListDouble.d
gccPigeonGameRelease/gmScript/gm/gmListDouble.o: gmScript/gm/gmListDouble.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmListDouble.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmListDouble.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmListDouble.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmListDouble.d

# Compiles file gmScript/gm/gmLog.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmLog.d
gccPigeonGameRelease/gmScript/gm/gmLog.o: gmScript/gm/gmLog.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmLog.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmLog.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmLog.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmLog.d

# Compiles file gmScript/gm/gmMachine.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmMachine.d
gccPigeonGameRelease/gmScript/gm/gmMachine.o: gmScript/gm/gmMachine.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmMachine.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmMachine.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmMachine.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmMachine.d

# Compiles file gmScript/gm/gmMachineLib.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmMachineLib.d
gccPigeonGameRelease/gmScript/gm/gmMachineLib.o: gmScript/gm/gmMachineLib.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmMachineLib.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmMachineLib.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmMachineLib.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmMachineLib.d

# Compiles file gmScript/gm/gmMem.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmMem.d
gccPigeonGameRelease/gmScript/gm/gmMem.o: gmScript/gm/gmMem.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmMem.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmMem.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmMem.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmMem.d

# Compiles file gmScript/gm/gmMemChain.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmMemChain.d
gccPigeonGameRelease/gmScript/gm/gmMemChain.o: gmScript/gm/gmMemChain.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmMemChain.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmMemChain.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmMemChain.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmMemChain.d

# Compiles file gmScript/gm/gmMemFixed.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmMemFixed.d
gccPigeonGameRelease/gmScript/gm/gmMemFixed.o: gmScript/gm/gmMemFixed.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmMemFixed.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmMemFixed.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmMemFixed.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmMemFixed.d

# Compiles file gmScript/gm/gmMemFixedSet.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmMemFixedSet.d
gccPigeonGameRelease/gmScript/gm/gmMemFixedSet.o: gmScript/gm/gmMemFixedSet.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmMemFixedSet.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmMemFixedSet.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmMemFixedSet.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmMemFixedSet.d

# Compiles file gmScript/gm/gmOperators.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmOperators.d
gccPigeonGameRelease/gmScript/gm/gmOperators.o: gmScript/gm/gmOperators.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmOperators.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmOperators.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmOperators.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmOperators.d

# Compiles file gmScript/gm/gmParser.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmParser.d
gccPigeonGameRelease/gmScript/gm/gmParser.o: gmScript/gm/gmParser.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmParser.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmParser.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmParser.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmParser.d

# Compiles file gmScript/gm/gmScanner.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmScanner.d
gccPigeonGameRelease/gmScript/gm/gmScanner.o: gmScript/gm/gmScanner.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmScanner.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmScanner.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmScanner.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmScanner.d

# Compiles file gmScript/gm/gmStream.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmStream.d
gccPigeonGameRelease/gmScript/gm/gmStream.o: gmScript/gm/gmStream.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmStream.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmStream.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmStream.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmStream.d

# Compiles file gmScript/gm/gmStreamBuffer.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmStreamBuffer.d
gccPigeonGameRelease/gmScript/gm/gmStreamBuffer.o: gmScript/gm/gmStreamBuffer.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmStreamBuffer.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmStreamBuffer.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmStreamBuffer.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmStreamBuffer.d

# Compiles file gmScript/gm/gmStringObject.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmStringObject.d
gccPigeonGameRelease/gmScript/gm/gmStringObject.o: gmScript/gm/gmStringObject.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmStringObject.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmStringObject.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmStringObject.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmStringObject.d

# Compiles file gmScript/gm/gmTableObject.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmTableObject.d
gccPigeonGameRelease/gmScript/gm/gmTableObject.o: gmScript/gm/gmTableObject.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmTableObject.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmTableObject.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmTableObject.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmTableObject.d

# Compiles file gmScript/gm/gmThread.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmThread.d
gccPigeonGameRelease/gmScript/gm/gmThread.o: gmScript/gm/gmThread.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmThread.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmThread.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmThread.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmThread.d

# Compiles file gmScript/gm/gmUserObject.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmUserObject.d
gccPigeonGameRelease/gmScript/gm/gmUserObject.o: gmScript/gm/gmUserObject.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmUserObject.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmUserObject.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmUserObject.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmUserObject.d

# Compiles file gmScript/gm/gmUtil.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmUtil.d
gccPigeonGameRelease/gmScript/gm/gmUtil.o: gmScript/gm/gmUtil.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmUtil.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmUtil.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmUtil.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmUtil.d

# Compiles file gmScript/gm/gmVariable.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/gm/gmVariable.d
gccPigeonGameRelease/gmScript/gm/gmVariable.o: gmScript/gm/gmVariable.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/gm/gmVariable.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/gm/gmVariable.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/gm/gmVariable.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/gm/gmVariable.d

# Compiles file gmScript/binds/gmArrayLib.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmArrayLib.d
gccPigeonGameRelease/gmScript/binds/gmArrayLib.o: gmScript/binds/gmArrayLib.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmArrayLib.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmArrayLib.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmArrayLib.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmArrayLib.d

# Compiles file gmScript/binds/gmCall.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmCall.d
gccPigeonGameRelease/gmScript/binds/gmCall.o: gmScript/binds/gmCall.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmCall.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmCall.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmCall.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmCall.d

# Compiles file gmScript/binds/gmGCRoot.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmGCRoot.d
gccPigeonGameRelease/gmScript/binds/gmGCRoot.o: gmScript/binds/gmGCRoot.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmGCRoot.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmGCRoot.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmGCRoot.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmGCRoot.d

# Compiles file gmScript/binds/gmGCRootUtil.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmGCRootUtil.d
gccPigeonGameRelease/gmScript/binds/gmGCRootUtil.o: gmScript/binds/gmGCRootUtil.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmGCRootUtil.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmGCRootUtil.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmGCRootUtil.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmGCRootUtil.d

# Compiles file gmScript/binds/gmHelpers.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmHelpers.d
gccPigeonGameRelease/gmScript/binds/gmHelpers.o: gmScript/binds/gmHelpers.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmHelpers.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmHelpers.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmHelpers.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmHelpers.d

# Compiles file gmScript/binds/gmMathLib.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmMathLib.d
gccPigeonGameRelease/gmScript/binds/gmMathLib.o: gmScript/binds/gmMathLib.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmMathLib.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmMathLib.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmMathLib.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmMathLib.d

# Compiles file gmScript/binds/gmStringLib.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmStringLib.d
gccPigeonGameRelease/gmScript/binds/gmStringLib.o: gmScript/binds/gmStringLib.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmStringLib.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmStringLib.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmStringLib.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmStringLib.d

# Compiles file gmScript/binds/gmSystemLib.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmSystemLib.d
gccPigeonGameRelease/gmScript/binds/gmSystemLib.o: gmScript/binds/gmSystemLib.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmSystemLib.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmSystemLib.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmSystemLib.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmSystemLib.d

# Compiles file gmScript/binds/gmVector3Lib.cpp for the PigeonGameRelease configuration...
-include gccPigeonGameRelease/gmScript/binds/gmVector3Lib.d
gccPigeonGameRelease/gmScript/binds/gmVector3Lib.o: gmScript/binds/gmVector3Lib.cpp
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -c gmScript/binds/gmVector3Lib.cpp $(PigeonGameRelease_Include_Path) -o gccPigeonGameRelease/gmScript/binds/gmVector3Lib.o
	$(CPP_COMPILER) $(PigeonGameRelease_Preprocessor_Definitions) $(PigeonGameRelease_Compiler_Flags) -MM gmScript/binds/gmVector3Lib.cpp $(PigeonGameRelease_Include_Path) > gccPigeonGameRelease/gmScript/binds/gmVector3Lib.d

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/../public/Utils/GeomTools.o gccRelease/../public/Math/math_util.o gccRelease/../public/BaseShader.o gccRelease/../public/BaseRenderableObject.o gccRelease/../public/RenderList.o gccRelease/../public/ViewParams.o gccRelease/../public/dsm_esm_loader.o gccRelease/../public/dsm_loader.o gccRelease/../public/dsm_obj_loader.o gccRelease/GameLibrary.o gccRelease/GameState.o gccRelease/AI/AIBaseNPC.o gccRelease/AI/AINode.o gccRelease/AI/AITaskTypes.o gccRelease/AI/AI_Idle.o gccRelease/AI/AI_MovementTarget.o gccRelease/AI/ai_navigator.o gccRelease/AI/Detour/Source/DetourAlloc.o gccRelease/AI/Detour/Source/DetourCommon.o gccRelease/AI/Detour/Source/DetourNavMesh.o gccRelease/AI/Detour/Source/DetourNavMeshBuilder.o gccRelease/AI/Detour/Source/DetourNavMeshQuery.o gccRelease/AI/Detour/Source/DetourNode.o gccRelease/AI/Recast/Source/Recast.o gccRelease/AI/Recast/Source/RecastAlloc.o gccRelease/AI/Recast/Source/RecastArea.o gccRelease/AI/Recast/Source/RecastContour.o gccRelease/AI/Recast/Source/RecastFilter.o gccRelease/AI/Recast/Source/RecastLayers.o gccRelease/AI/Recast/Source/RecastMesh.o gccRelease/AI/Recast/Source/RecastMeshDetail.o gccRelease/AI/Recast/Source/RecastRasterization.o gccRelease/AI/Recast/Source/RecastRegion.o gccRelease/AI/AIMemoryBase.o gccRelease/AI/AIScriptTask.o gccRelease/AI/AITaskActionBase.o gccRelease/AmmoType.o gccRelease/BaseEntity.o gccRelease/InfoCamera.o gccRelease/Ladder.o gccRelease/LogicEntities.o gccRelease/BulletSimulator.o gccRelease/BaseActor.o gccRelease/BaseAnimating.o gccRelease/BaseWeapon.o gccRelease/EffectEntities.o gccRelease/EngineEntities.o gccRelease/PlayerSpawn.o gccRelease/ModelEntities.o gccRelease/SoundEntities.o gccRelease/Triggers.o gccRelease/../public/anim_activity.o gccRelease/../public/anim_events.o gccRelease/../public/BoneSetup.o gccRelease/../public/ragdoll.o gccRelease/EntityQueue.o gccRelease/EntityDataField.o gccRelease/GameTrace.o gccRelease/Effects.o gccRelease/GameInput.o gccRelease/GlobalVarsGame.o gccRelease/MaterialProxy.o gccRelease/Rain.o gccRelease/Snow.o gccRelease/SaveGame_Events.o gccRelease/SaveRestoreManager.o gccRelease/EffectRender.o gccRelease/FilterPipelineBase.o gccRelease/GameRenderer.o gccRelease/ParticleRenderer.o gccRelease/RenderDefs.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/PigeonGame/BasePigeon.o gccRelease/PigeonGame/GameRules_Pigeon.o gccRelease/PigeonGame/Pigeon_Player.o gccRelease/GameRules_WhiteCage.o gccRelease/npc_cyborg.o gccRelease/npc_soldier.o gccRelease/Player.o gccRelease/viewmodel.o gccRelease/weapon_ak74.o gccRelease/weapon_deagle.o gccRelease/weapon_f1.o gccRelease/weapon_wiremine.o gccRelease/EGUI/EQUI_Manager.o gccRelease/EGUI/EqUI_Panel.o gccRelease/EntityBind.o gccRelease/EqGMS.o gccRelease/EngineScriptBind.o gccRelease/gmScript/gm/gmArraySimple.o gccRelease/gmScript/gm/gmByteCode.o gccRelease/gmScript/gm/gmByteCodeGen.o gccRelease/gmScript/gm/gmCodeGen.o gccRelease/gmScript/gm/gmCodeGenHooks.o gccRelease/gmScript/gm/gmCodeTree.o gccRelease/gmScript/gm/gmCrc.o gccRelease/gmScript/gm/gmDebug.o gccRelease/gmScript/gm/gmFunctionObject.o gccRelease/gmScript/gm/gmHash.o gccRelease/gmScript/gm/gmIncGC.o gccRelease/gmScript/gm/gmLibHooks.o gccRelease/gmScript/gm/gmListDouble.o gccRelease/gmScript/gm/gmLog.o gccRelease/gmScript/gm/gmMachine.o gccRelease/gmScript/gm/gmMachineLib.o gccRelease/gmScript/gm/gmMem.o gccRelease/gmScript/gm/gmMemChain.o gccRelease/gmScript/gm/gmMemFixed.o gccRelease/gmScript/gm/gmMemFixedSet.o gccRelease/gmScript/gm/gmOperators.o gccRelease/gmScript/gm/gmParser.o gccRelease/gmScript/gm/gmScanner.o gccRelease/gmScript/gm/gmStream.o gccRelease/gmScript/gm/gmStreamBuffer.o gccRelease/gmScript/gm/gmStringObject.o gccRelease/gmScript/gm/gmTableObject.o gccRelease/gmScript/gm/gmThread.o gccRelease/gmScript/gm/gmUserObject.o gccRelease/gmScript/gm/gmUtil.o gccRelease/gmScript/gm/gmVariable.o gccRelease/gmScript/binds/gmArrayLib.o gccRelease/gmScript/binds/gmCall.o gccRelease/gmScript/binds/gmGCRoot.o gccRelease/gmScript/binds/gmGCRootUtil.o gccRelease/gmScript/binds/gmHelpers.o gccRelease/gmScript/binds/gmMathLib.o gccRelease/gmScript/binds/gmStringLib.o gccRelease/gmScript/binds/gmSystemLib.o gccRelease/gmScript/binds/gmVector3Lib.o 
	g++ -fPIC -shared -Wl,-soname,libGameDLL.so -o gccRelease/libGameDLL.so gccRelease/../public/Utils/GeomTools.o gccRelease/../public/Math/math_util.o gccRelease/../public/BaseShader.o gccRelease/../public/BaseRenderableObject.o gccRelease/../public/RenderList.o gccRelease/../public/ViewParams.o gccRelease/../public/dsm_esm_loader.o gccRelease/../public/dsm_loader.o gccRelease/../public/dsm_obj_loader.o gccRelease/GameLibrary.o gccRelease/GameState.o gccRelease/AI/AIBaseNPC.o gccRelease/AI/AINode.o gccRelease/AI/AITaskTypes.o gccRelease/AI/AI_Idle.o gccRelease/AI/AI_MovementTarget.o gccRelease/AI/ai_navigator.o gccRelease/AI/Detour/Source/DetourAlloc.o gccRelease/AI/Detour/Source/DetourCommon.o gccRelease/AI/Detour/Source/DetourNavMesh.o gccRelease/AI/Detour/Source/DetourNavMeshBuilder.o gccRelease/AI/Detour/Source/DetourNavMeshQuery.o gccRelease/AI/Detour/Source/DetourNode.o gccRelease/AI/Recast/Source/Recast.o gccRelease/AI/Recast/Source/RecastAlloc.o gccRelease/AI/Recast/Source/RecastArea.o gccRelease/AI/Recast/Source/RecastContour.o gccRelease/AI/Recast/Source/RecastFilter.o gccRelease/AI/Recast/Source/RecastLayers.o gccRelease/AI/Recast/Source/RecastMesh.o gccRelease/AI/Recast/Source/RecastMeshDetail.o gccRelease/AI/Recast/Source/RecastRasterization.o gccRelease/AI/Recast/Source/RecastRegion.o gccRelease/AI/AIMemoryBase.o gccRelease/AI/AIScriptTask.o gccRelease/AI/AITaskActionBase.o gccRelease/AmmoType.o gccRelease/BaseEntity.o gccRelease/InfoCamera.o gccRelease/Ladder.o gccRelease/LogicEntities.o gccRelease/BulletSimulator.o gccRelease/BaseActor.o gccRelease/BaseAnimating.o gccRelease/BaseWeapon.o gccRelease/EffectEntities.o gccRelease/EngineEntities.o gccRelease/PlayerSpawn.o gccRelease/ModelEntities.o gccRelease/SoundEntities.o gccRelease/Triggers.o gccRelease/../public/anim_activity.o gccRelease/../public/anim_events.o gccRelease/../public/BoneSetup.o gccRelease/../public/ragdoll.o gccRelease/EntityQueue.o gccRelease/EntityDataField.o gccRelease/GameTrace.o gccRelease/Effects.o gccRelease/GameInput.o gccRelease/GlobalVarsGame.o gccRelease/MaterialProxy.o gccRelease/Rain.o gccRelease/Snow.o gccRelease/SaveGame_Events.o gccRelease/SaveRestoreManager.o gccRelease/EffectRender.o gccRelease/FilterPipelineBase.o gccRelease/GameRenderer.o gccRelease/ParticleRenderer.o gccRelease/RenderDefs.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/PigeonGame/BasePigeon.o gccRelease/PigeonGame/GameRules_Pigeon.o gccRelease/PigeonGame/Pigeon_Player.o gccRelease/GameRules_WhiteCage.o gccRelease/npc_cyborg.o gccRelease/npc_soldier.o gccRelease/Player.o gccRelease/viewmodel.o gccRelease/weapon_ak74.o gccRelease/weapon_deagle.o gccRelease/weapon_f1.o gccRelease/weapon_wiremine.o gccRelease/EGUI/EQUI_Manager.o gccRelease/EGUI/EqUI_Panel.o gccRelease/EntityBind.o gccRelease/EqGMS.o gccRelease/EngineScriptBind.o gccRelease/gmScript/gm/gmArraySimple.o gccRelease/gmScript/gm/gmByteCode.o gccRelease/gmScript/gm/gmByteCodeGen.o gccRelease/gmScript/gm/gmCodeGen.o gccRelease/gmScript/gm/gmCodeGenHooks.o gccRelease/gmScript/gm/gmCodeTree.o gccRelease/gmScript/gm/gmCrc.o gccRelease/gmScript/gm/gmDebug.o gccRelease/gmScript/gm/gmFunctionObject.o gccRelease/gmScript/gm/gmHash.o gccRelease/gmScript/gm/gmIncGC.o gccRelease/gmScript/gm/gmLibHooks.o gccRelease/gmScript/gm/gmListDouble.o gccRelease/gmScript/gm/gmLog.o gccRelease/gmScript/gm/gmMachine.o gccRelease/gmScript/gm/gmMachineLib.o gccRelease/gmScript/gm/gmMem.o gccRelease/gmScript/gm/gmMemChain.o gccRelease/gmScript/gm/gmMemFixed.o gccRelease/gmScript/gm/gmMemFixedSet.o gccRelease/gmScript/gm/gmOperators.o gccRelease/gmScript/gm/gmParser.o gccRelease/gmScript/gm/gmScanner.o gccRelease/gmScript/gm/gmStream.o gccRelease/gmScript/gm/gmStreamBuffer.o gccRelease/gmScript/gm/gmStringObject.o gccRelease/gmScript/gm/gmTableObject.o gccRelease/gmScript/gm/gmThread.o gccRelease/gmScript/gm/gmUserObject.o gccRelease/gmScript/gm/gmUtil.o gccRelease/gmScript/gm/gmVariable.o gccRelease/gmScript/binds/gmArrayLib.o gccRelease/gmScript/binds/gmCall.o gccRelease/gmScript/binds/gmGCRoot.o gccRelease/gmScript/binds/gmGCRootUtil.o gccRelease/gmScript/binds/gmHelpers.o gccRelease/gmScript/binds/gmMathLib.o gccRelease/gmScript/binds/gmStringLib.o gccRelease/gmScript/binds/gmSystemLib.o gccRelease/gmScript/binds/gmVector3Lib.o  $(Release_Implicitly_Linked_Objects)

# Compiles file ../public/Utils/GeomTools.cpp for the Release configuration...
-include gccRelease/../public/Utils/GeomTools.d
gccRelease/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(Release_Include_Path) -o gccRelease/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(Release_Include_Path) > gccRelease/../public/Utils/GeomTools.d

# Compiles file ../public/Math/math_util.cpp for the Release configuration...
-include gccRelease/../public/Math/math_util.d
gccRelease/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Math/math_util.cpp $(Release_Include_Path) -o gccRelease/../public/Math/math_util.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Math/math_util.cpp $(Release_Include_Path) > gccRelease/../public/Math/math_util.d

# Compiles file ../public/BaseShader.cpp for the Release configuration...
-include gccRelease/../public/BaseShader.d
gccRelease/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BaseShader.cpp $(Release_Include_Path) -o gccRelease/../public/BaseShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BaseShader.cpp $(Release_Include_Path) > gccRelease/../public/BaseShader.d

# Compiles file ../public/BaseRenderableObject.cpp for the Release configuration...
-include gccRelease/../public/BaseRenderableObject.d
gccRelease/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(Release_Include_Path) -o gccRelease/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(Release_Include_Path) > gccRelease/../public/BaseRenderableObject.d

# Compiles file ../public/RenderList.cpp for the Release configuration...
-include gccRelease/../public/RenderList.d
gccRelease/../public/RenderList.o: ../public/RenderList.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/RenderList.cpp $(Release_Include_Path) -o gccRelease/../public/RenderList.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/RenderList.cpp $(Release_Include_Path) > gccRelease/../public/RenderList.d

# Compiles file ../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../public/ViewParams.d
gccRelease/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../public/ViewParams.d

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

# Compiles file GameLibrary.cpp for the Release configuration...
-include gccRelease/GameLibrary.d
gccRelease/GameLibrary.o: GameLibrary.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameLibrary.cpp $(Release_Include_Path) -o gccRelease/GameLibrary.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameLibrary.cpp $(Release_Include_Path) > gccRelease/GameLibrary.d

# Compiles file GameState.cpp for the Release configuration...
-include gccRelease/GameState.d
gccRelease/GameState.o: GameState.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameState.cpp $(Release_Include_Path) -o gccRelease/GameState.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameState.cpp $(Release_Include_Path) > gccRelease/GameState.d

# Compiles file AI/AIBaseNPC.cpp for the Release configuration...
-include gccRelease/AI/AIBaseNPC.d
gccRelease/AI/AIBaseNPC.o: AI/AIBaseNPC.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AIBaseNPC.cpp $(Release_Include_Path) -o gccRelease/AI/AIBaseNPC.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AIBaseNPC.cpp $(Release_Include_Path) > gccRelease/AI/AIBaseNPC.d

# Compiles file AI/AINode.cpp for the Release configuration...
-include gccRelease/AI/AINode.d
gccRelease/AI/AINode.o: AI/AINode.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AINode.cpp $(Release_Include_Path) -o gccRelease/AI/AINode.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AINode.cpp $(Release_Include_Path) > gccRelease/AI/AINode.d

# Compiles file AI/AITaskTypes.cpp for the Release configuration...
-include gccRelease/AI/AITaskTypes.d
gccRelease/AI/AITaskTypes.o: AI/AITaskTypes.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AITaskTypes.cpp $(Release_Include_Path) -o gccRelease/AI/AITaskTypes.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AITaskTypes.cpp $(Release_Include_Path) > gccRelease/AI/AITaskTypes.d

# Compiles file AI/AI_Idle.cpp for the Release configuration...
-include gccRelease/AI/AI_Idle.d
gccRelease/AI/AI_Idle.o: AI/AI_Idle.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AI_Idle.cpp $(Release_Include_Path) -o gccRelease/AI/AI_Idle.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AI_Idle.cpp $(Release_Include_Path) > gccRelease/AI/AI_Idle.d

# Compiles file AI/AI_MovementTarget.cpp for the Release configuration...
-include gccRelease/AI/AI_MovementTarget.d
gccRelease/AI/AI_MovementTarget.o: AI/AI_MovementTarget.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AI_MovementTarget.cpp $(Release_Include_Path) -o gccRelease/AI/AI_MovementTarget.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AI_MovementTarget.cpp $(Release_Include_Path) > gccRelease/AI/AI_MovementTarget.d

# Compiles file AI/ai_navigator.cpp for the Release configuration...
-include gccRelease/AI/ai_navigator.d
gccRelease/AI/ai_navigator.o: AI/ai_navigator.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/ai_navigator.cpp $(Release_Include_Path) -o gccRelease/AI/ai_navigator.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/ai_navigator.cpp $(Release_Include_Path) > gccRelease/AI/ai_navigator.d

# Compiles file AI/Detour/Source/DetourAlloc.cpp for the Release configuration...
-include gccRelease/AI/Detour/Source/DetourAlloc.d
gccRelease/AI/Detour/Source/DetourAlloc.o: AI/Detour/Source/DetourAlloc.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Detour/Source/DetourAlloc.cpp $(Release_Include_Path) -o gccRelease/AI/Detour/Source/DetourAlloc.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Detour/Source/DetourAlloc.cpp $(Release_Include_Path) > gccRelease/AI/Detour/Source/DetourAlloc.d

# Compiles file AI/Detour/Source/DetourCommon.cpp for the Release configuration...
-include gccRelease/AI/Detour/Source/DetourCommon.d
gccRelease/AI/Detour/Source/DetourCommon.o: AI/Detour/Source/DetourCommon.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Detour/Source/DetourCommon.cpp $(Release_Include_Path) -o gccRelease/AI/Detour/Source/DetourCommon.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Detour/Source/DetourCommon.cpp $(Release_Include_Path) > gccRelease/AI/Detour/Source/DetourCommon.d

# Compiles file AI/Detour/Source/DetourNavMesh.cpp for the Release configuration...
-include gccRelease/AI/Detour/Source/DetourNavMesh.d
gccRelease/AI/Detour/Source/DetourNavMesh.o: AI/Detour/Source/DetourNavMesh.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Detour/Source/DetourNavMesh.cpp $(Release_Include_Path) -o gccRelease/AI/Detour/Source/DetourNavMesh.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Detour/Source/DetourNavMesh.cpp $(Release_Include_Path) > gccRelease/AI/Detour/Source/DetourNavMesh.d

# Compiles file AI/Detour/Source/DetourNavMeshBuilder.cpp for the Release configuration...
-include gccRelease/AI/Detour/Source/DetourNavMeshBuilder.d
gccRelease/AI/Detour/Source/DetourNavMeshBuilder.o: AI/Detour/Source/DetourNavMeshBuilder.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshBuilder.cpp $(Release_Include_Path) -o gccRelease/AI/Detour/Source/DetourNavMeshBuilder.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshBuilder.cpp $(Release_Include_Path) > gccRelease/AI/Detour/Source/DetourNavMeshBuilder.d

# Compiles file AI/Detour/Source/DetourNavMeshQuery.cpp for the Release configuration...
-include gccRelease/AI/Detour/Source/DetourNavMeshQuery.d
gccRelease/AI/Detour/Source/DetourNavMeshQuery.o: AI/Detour/Source/DetourNavMeshQuery.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshQuery.cpp $(Release_Include_Path) -o gccRelease/AI/Detour/Source/DetourNavMeshQuery.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshQuery.cpp $(Release_Include_Path) > gccRelease/AI/Detour/Source/DetourNavMeshQuery.d

# Compiles file AI/Detour/Source/DetourNode.cpp for the Release configuration...
-include gccRelease/AI/Detour/Source/DetourNode.d
gccRelease/AI/Detour/Source/DetourNode.o: AI/Detour/Source/DetourNode.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Detour/Source/DetourNode.cpp $(Release_Include_Path) -o gccRelease/AI/Detour/Source/DetourNode.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Detour/Source/DetourNode.cpp $(Release_Include_Path) > gccRelease/AI/Detour/Source/DetourNode.d

# Compiles file AI/Recast/Source/Recast.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/Recast.d
gccRelease/AI/Recast/Source/Recast.o: AI/Recast/Source/Recast.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/Recast.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/Recast.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/Recast.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/Recast.d

# Compiles file AI/Recast/Source/RecastAlloc.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastAlloc.d
gccRelease/AI/Recast/Source/RecastAlloc.o: AI/Recast/Source/RecastAlloc.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastAlloc.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastAlloc.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastAlloc.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastAlloc.d

# Compiles file AI/Recast/Source/RecastArea.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastArea.d
gccRelease/AI/Recast/Source/RecastArea.o: AI/Recast/Source/RecastArea.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastArea.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastArea.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastArea.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastArea.d

# Compiles file AI/Recast/Source/RecastContour.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastContour.d
gccRelease/AI/Recast/Source/RecastContour.o: AI/Recast/Source/RecastContour.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastContour.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastContour.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastContour.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastContour.d

# Compiles file AI/Recast/Source/RecastFilter.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastFilter.d
gccRelease/AI/Recast/Source/RecastFilter.o: AI/Recast/Source/RecastFilter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastFilter.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastFilter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastFilter.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastFilter.d

# Compiles file AI/Recast/Source/RecastLayers.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastLayers.d
gccRelease/AI/Recast/Source/RecastLayers.o: AI/Recast/Source/RecastLayers.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastLayers.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastLayers.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastLayers.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastLayers.d

# Compiles file AI/Recast/Source/RecastMesh.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastMesh.d
gccRelease/AI/Recast/Source/RecastMesh.o: AI/Recast/Source/RecastMesh.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastMesh.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastMesh.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastMesh.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastMesh.d

# Compiles file AI/Recast/Source/RecastMeshDetail.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastMeshDetail.d
gccRelease/AI/Recast/Source/RecastMeshDetail.o: AI/Recast/Source/RecastMeshDetail.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastMeshDetail.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastMeshDetail.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastMeshDetail.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastMeshDetail.d

# Compiles file AI/Recast/Source/RecastRasterization.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastRasterization.d
gccRelease/AI/Recast/Source/RecastRasterization.o: AI/Recast/Source/RecastRasterization.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastRasterization.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastRasterization.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastRasterization.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastRasterization.d

# Compiles file AI/Recast/Source/RecastRegion.cpp for the Release configuration...
-include gccRelease/AI/Recast/Source/RecastRegion.d
gccRelease/AI/Recast/Source/RecastRegion.o: AI/Recast/Source/RecastRegion.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/Recast/Source/RecastRegion.cpp $(Release_Include_Path) -o gccRelease/AI/Recast/Source/RecastRegion.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/Recast/Source/RecastRegion.cpp $(Release_Include_Path) > gccRelease/AI/Recast/Source/RecastRegion.d

# Compiles file AI/AIMemoryBase.cpp for the Release configuration...
-include gccRelease/AI/AIMemoryBase.d
gccRelease/AI/AIMemoryBase.o: AI/AIMemoryBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AIMemoryBase.cpp $(Release_Include_Path) -o gccRelease/AI/AIMemoryBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AIMemoryBase.cpp $(Release_Include_Path) > gccRelease/AI/AIMemoryBase.d

# Compiles file AI/AIScriptTask.cpp for the Release configuration...
-include gccRelease/AI/AIScriptTask.d
gccRelease/AI/AIScriptTask.o: AI/AIScriptTask.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AIScriptTask.cpp $(Release_Include_Path) -o gccRelease/AI/AIScriptTask.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AIScriptTask.cpp $(Release_Include_Path) > gccRelease/AI/AIScriptTask.d

# Compiles file AI/AITaskActionBase.cpp for the Release configuration...
-include gccRelease/AI/AITaskActionBase.d
gccRelease/AI/AITaskActionBase.o: AI/AITaskActionBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AI/AITaskActionBase.cpp $(Release_Include_Path) -o gccRelease/AI/AITaskActionBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AI/AITaskActionBase.cpp $(Release_Include_Path) > gccRelease/AI/AITaskActionBase.d

# Compiles file AmmoType.cpp for the Release configuration...
-include gccRelease/AmmoType.d
gccRelease/AmmoType.o: AmmoType.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c AmmoType.cpp $(Release_Include_Path) -o gccRelease/AmmoType.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM AmmoType.cpp $(Release_Include_Path) > gccRelease/AmmoType.d

# Compiles file BaseEntity.cpp for the Release configuration...
-include gccRelease/BaseEntity.d
gccRelease/BaseEntity.o: BaseEntity.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseEntity.cpp $(Release_Include_Path) -o gccRelease/BaseEntity.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseEntity.cpp $(Release_Include_Path) > gccRelease/BaseEntity.d

# Compiles file InfoCamera.cpp for the Release configuration...
-include gccRelease/InfoCamera.d
gccRelease/InfoCamera.o: InfoCamera.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c InfoCamera.cpp $(Release_Include_Path) -o gccRelease/InfoCamera.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM InfoCamera.cpp $(Release_Include_Path) > gccRelease/InfoCamera.d

# Compiles file Ladder.cpp for the Release configuration...
-include gccRelease/Ladder.d
gccRelease/Ladder.o: Ladder.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Ladder.cpp $(Release_Include_Path) -o gccRelease/Ladder.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Ladder.cpp $(Release_Include_Path) > gccRelease/Ladder.d

# Compiles file LogicEntities.cpp for the Release configuration...
-include gccRelease/LogicEntities.d
gccRelease/LogicEntities.o: LogicEntities.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c LogicEntities.cpp $(Release_Include_Path) -o gccRelease/LogicEntities.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM LogicEntities.cpp $(Release_Include_Path) > gccRelease/LogicEntities.d

# Compiles file BulletSimulator.cpp for the Release configuration...
-include gccRelease/BulletSimulator.d
gccRelease/BulletSimulator.o: BulletSimulator.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BulletSimulator.cpp $(Release_Include_Path) -o gccRelease/BulletSimulator.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BulletSimulator.cpp $(Release_Include_Path) > gccRelease/BulletSimulator.d

# Compiles file BaseActor.cpp for the Release configuration...
-include gccRelease/BaseActor.d
gccRelease/BaseActor.o: BaseActor.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseActor.cpp $(Release_Include_Path) -o gccRelease/BaseActor.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseActor.cpp $(Release_Include_Path) > gccRelease/BaseActor.d

# Compiles file BaseAnimating.cpp for the Release configuration...
-include gccRelease/BaseAnimating.d
gccRelease/BaseAnimating.o: BaseAnimating.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseAnimating.cpp $(Release_Include_Path) -o gccRelease/BaseAnimating.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseAnimating.cpp $(Release_Include_Path) > gccRelease/BaseAnimating.d

# Compiles file BaseWeapon.cpp for the Release configuration...
-include gccRelease/BaseWeapon.d
gccRelease/BaseWeapon.o: BaseWeapon.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseWeapon.cpp $(Release_Include_Path) -o gccRelease/BaseWeapon.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseWeapon.cpp $(Release_Include_Path) > gccRelease/BaseWeapon.d

# Compiles file EffectEntities.cpp for the Release configuration...
-include gccRelease/EffectEntities.d
gccRelease/EffectEntities.o: EffectEntities.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EffectEntities.cpp $(Release_Include_Path) -o gccRelease/EffectEntities.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EffectEntities.cpp $(Release_Include_Path) > gccRelease/EffectEntities.d

# Compiles file EngineEntities.cpp for the Release configuration...
-include gccRelease/EngineEntities.d
gccRelease/EngineEntities.o: EngineEntities.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EngineEntities.cpp $(Release_Include_Path) -o gccRelease/EngineEntities.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EngineEntities.cpp $(Release_Include_Path) > gccRelease/EngineEntities.d

# Compiles file PlayerSpawn.cpp for the Release configuration...
-include gccRelease/PlayerSpawn.d
gccRelease/PlayerSpawn.o: PlayerSpawn.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c PlayerSpawn.cpp $(Release_Include_Path) -o gccRelease/PlayerSpawn.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM PlayerSpawn.cpp $(Release_Include_Path) > gccRelease/PlayerSpawn.d

# Compiles file ModelEntities.cpp for the Release configuration...
-include gccRelease/ModelEntities.d
gccRelease/ModelEntities.o: ModelEntities.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ModelEntities.cpp $(Release_Include_Path) -o gccRelease/ModelEntities.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ModelEntities.cpp $(Release_Include_Path) > gccRelease/ModelEntities.d

# Compiles file SoundEntities.cpp for the Release configuration...
-include gccRelease/SoundEntities.d
gccRelease/SoundEntities.o: SoundEntities.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SoundEntities.cpp $(Release_Include_Path) -o gccRelease/SoundEntities.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SoundEntities.cpp $(Release_Include_Path) > gccRelease/SoundEntities.d

# Compiles file Triggers.cpp for the Release configuration...
-include gccRelease/Triggers.d
gccRelease/Triggers.o: Triggers.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Triggers.cpp $(Release_Include_Path) -o gccRelease/Triggers.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Triggers.cpp $(Release_Include_Path) > gccRelease/Triggers.d

# Compiles file ../public/anim_activity.cpp for the Release configuration...
-include gccRelease/../public/anim_activity.d
gccRelease/../public/anim_activity.o: ../public/anim_activity.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/anim_activity.cpp $(Release_Include_Path) -o gccRelease/../public/anim_activity.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/anim_activity.cpp $(Release_Include_Path) > gccRelease/../public/anim_activity.d

# Compiles file ../public/anim_events.cpp for the Release configuration...
-include gccRelease/../public/anim_events.d
gccRelease/../public/anim_events.o: ../public/anim_events.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/anim_events.cpp $(Release_Include_Path) -o gccRelease/../public/anim_events.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/anim_events.cpp $(Release_Include_Path) > gccRelease/../public/anim_events.d

# Compiles file ../public/BoneSetup.cpp for the Release configuration...
-include gccRelease/../public/BoneSetup.d
gccRelease/../public/BoneSetup.o: ../public/BoneSetup.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BoneSetup.cpp $(Release_Include_Path) -o gccRelease/../public/BoneSetup.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BoneSetup.cpp $(Release_Include_Path) > gccRelease/../public/BoneSetup.d

# Compiles file ../public/ragdoll.cpp for the Release configuration...
-include gccRelease/../public/ragdoll.d
gccRelease/../public/ragdoll.o: ../public/ragdoll.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ragdoll.cpp $(Release_Include_Path) -o gccRelease/../public/ragdoll.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ragdoll.cpp $(Release_Include_Path) > gccRelease/../public/ragdoll.d

# Compiles file EntityQueue.cpp for the Release configuration...
-include gccRelease/EntityQueue.d
gccRelease/EntityQueue.o: EntityQueue.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityQueue.cpp $(Release_Include_Path) -o gccRelease/EntityQueue.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityQueue.cpp $(Release_Include_Path) > gccRelease/EntityQueue.d

# Compiles file EntityDataField.cpp for the Release configuration...
-include gccRelease/EntityDataField.d
gccRelease/EntityDataField.o: EntityDataField.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityDataField.cpp $(Release_Include_Path) -o gccRelease/EntityDataField.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityDataField.cpp $(Release_Include_Path) > gccRelease/EntityDataField.d

# Compiles file GameTrace.cpp for the Release configuration...
-include gccRelease/GameTrace.d
gccRelease/GameTrace.o: GameTrace.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameTrace.cpp $(Release_Include_Path) -o gccRelease/GameTrace.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameTrace.cpp $(Release_Include_Path) > gccRelease/GameTrace.d

# Compiles file Effects.cpp for the Release configuration...
-include gccRelease/Effects.d
gccRelease/Effects.o: Effects.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Effects.cpp $(Release_Include_Path) -o gccRelease/Effects.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Effects.cpp $(Release_Include_Path) > gccRelease/Effects.d

# Compiles file GameInput.cpp for the Release configuration...
-include gccRelease/GameInput.d
gccRelease/GameInput.o: GameInput.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameInput.cpp $(Release_Include_Path) -o gccRelease/GameInput.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameInput.cpp $(Release_Include_Path) > gccRelease/GameInput.d

# Compiles file GlobalVarsGame.cpp for the Release configuration...
-include gccRelease/GlobalVarsGame.d
gccRelease/GlobalVarsGame.o: GlobalVarsGame.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GlobalVarsGame.cpp $(Release_Include_Path) -o gccRelease/GlobalVarsGame.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GlobalVarsGame.cpp $(Release_Include_Path) > gccRelease/GlobalVarsGame.d

# Compiles file MaterialProxy.cpp for the Release configuration...
-include gccRelease/MaterialProxy.d
gccRelease/MaterialProxy.o: MaterialProxy.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c MaterialProxy.cpp $(Release_Include_Path) -o gccRelease/MaterialProxy.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM MaterialProxy.cpp $(Release_Include_Path) > gccRelease/MaterialProxy.d

# Compiles file Rain.cpp for the Release configuration...
-include gccRelease/Rain.d
gccRelease/Rain.o: Rain.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Rain.cpp $(Release_Include_Path) -o gccRelease/Rain.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Rain.cpp $(Release_Include_Path) > gccRelease/Rain.d

# Compiles file Snow.cpp for the Release configuration...
-include gccRelease/Snow.d
gccRelease/Snow.o: Snow.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Snow.cpp $(Release_Include_Path) -o gccRelease/Snow.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Snow.cpp $(Release_Include_Path) > gccRelease/Snow.d

# Compiles file SaveGame_Events.cpp for the Release configuration...
-include gccRelease/SaveGame_Events.d
gccRelease/SaveGame_Events.o: SaveGame_Events.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SaveGame_Events.cpp $(Release_Include_Path) -o gccRelease/SaveGame_Events.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SaveGame_Events.cpp $(Release_Include_Path) > gccRelease/SaveGame_Events.d

# Compiles file SaveRestoreManager.cpp for the Release configuration...
-include gccRelease/SaveRestoreManager.d
gccRelease/SaveRestoreManager.o: SaveRestoreManager.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SaveRestoreManager.cpp $(Release_Include_Path) -o gccRelease/SaveRestoreManager.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SaveRestoreManager.cpp $(Release_Include_Path) > gccRelease/SaveRestoreManager.d

# Compiles file EffectRender.cpp for the Release configuration...
-include gccRelease/EffectRender.d
gccRelease/EffectRender.o: EffectRender.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EffectRender.cpp $(Release_Include_Path) -o gccRelease/EffectRender.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EffectRender.cpp $(Release_Include_Path) > gccRelease/EffectRender.d

# Compiles file FilterPipelineBase.cpp for the Release configuration...
-include gccRelease/FilterPipelineBase.d
gccRelease/FilterPipelineBase.o: FilterPipelineBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c FilterPipelineBase.cpp $(Release_Include_Path) -o gccRelease/FilterPipelineBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM FilterPipelineBase.cpp $(Release_Include_Path) > gccRelease/FilterPipelineBase.d

# Compiles file GameRenderer.cpp for the Release configuration...
-include gccRelease/GameRenderer.d
gccRelease/GameRenderer.o: GameRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameRenderer.cpp $(Release_Include_Path) -o gccRelease/GameRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameRenderer.cpp $(Release_Include_Path) > gccRelease/GameRenderer.d

# Compiles file ParticleRenderer.cpp for the Release configuration...
-include gccRelease/ParticleRenderer.d
gccRelease/ParticleRenderer.o: ParticleRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ParticleRenderer.cpp $(Release_Include_Path) -o gccRelease/ParticleRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ParticleRenderer.cpp $(Release_Include_Path) > gccRelease/ParticleRenderer.d

# Compiles file RenderDefs.cpp for the Release configuration...
-include gccRelease/RenderDefs.d
gccRelease/RenderDefs.o: RenderDefs.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c RenderDefs.cpp $(Release_Include_Path) -o gccRelease/RenderDefs.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM RenderDefs.cpp $(Release_Include_Path) > gccRelease/RenderDefs.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the Release configuration...
-include gccRelease/../public/GameSoundEmitterSystem.d
gccRelease/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(Release_Include_Path) -o gccRelease/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(Release_Include_Path) > gccRelease/../public/GameSoundEmitterSystem.d

# Compiles file PigeonGame/BasePigeon.cpp for the Release configuration...
-include gccRelease/PigeonGame/BasePigeon.d
gccRelease/PigeonGame/BasePigeon.o: PigeonGame/BasePigeon.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c PigeonGame/BasePigeon.cpp $(Release_Include_Path) -o gccRelease/PigeonGame/BasePigeon.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM PigeonGame/BasePigeon.cpp $(Release_Include_Path) > gccRelease/PigeonGame/BasePigeon.d

# Compiles file PigeonGame/GameRules_Pigeon.cpp for the Release configuration...
-include gccRelease/PigeonGame/GameRules_Pigeon.d
gccRelease/PigeonGame/GameRules_Pigeon.o: PigeonGame/GameRules_Pigeon.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c PigeonGame/GameRules_Pigeon.cpp $(Release_Include_Path) -o gccRelease/PigeonGame/GameRules_Pigeon.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM PigeonGame/GameRules_Pigeon.cpp $(Release_Include_Path) > gccRelease/PigeonGame/GameRules_Pigeon.d

# Compiles file PigeonGame/Pigeon_Player.cpp for the Release configuration...
-include gccRelease/PigeonGame/Pigeon_Player.d
gccRelease/PigeonGame/Pigeon_Player.o: PigeonGame/Pigeon_Player.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c PigeonGame/Pigeon_Player.cpp $(Release_Include_Path) -o gccRelease/PigeonGame/Pigeon_Player.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM PigeonGame/Pigeon_Player.cpp $(Release_Include_Path) > gccRelease/PigeonGame/Pigeon_Player.d

# Compiles file GameRules_WhiteCage.cpp for the Release configuration...
-include gccRelease/GameRules_WhiteCage.d
gccRelease/GameRules_WhiteCage.o: GameRules_WhiteCage.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GameRules_WhiteCage.cpp $(Release_Include_Path) -o gccRelease/GameRules_WhiteCage.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GameRules_WhiteCage.cpp $(Release_Include_Path) > gccRelease/GameRules_WhiteCage.d

# Compiles file npc_cyborg.cpp for the Release configuration...
-include gccRelease/npc_cyborg.d
gccRelease/npc_cyborg.o: npc_cyborg.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c npc_cyborg.cpp $(Release_Include_Path) -o gccRelease/npc_cyborg.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM npc_cyborg.cpp $(Release_Include_Path) > gccRelease/npc_cyborg.d

# Compiles file npc_soldier.cpp for the Release configuration...
-include gccRelease/npc_soldier.d
gccRelease/npc_soldier.o: npc_soldier.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c npc_soldier.cpp $(Release_Include_Path) -o gccRelease/npc_soldier.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM npc_soldier.cpp $(Release_Include_Path) > gccRelease/npc_soldier.d

# Compiles file Player.cpp for the Release configuration...
-include gccRelease/Player.d
gccRelease/Player.o: Player.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c Player.cpp $(Release_Include_Path) -o gccRelease/Player.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM Player.cpp $(Release_Include_Path) > gccRelease/Player.d

# Compiles file viewmodel.cpp for the Release configuration...
-include gccRelease/viewmodel.d
gccRelease/viewmodel.o: viewmodel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c viewmodel.cpp $(Release_Include_Path) -o gccRelease/viewmodel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM viewmodel.cpp $(Release_Include_Path) > gccRelease/viewmodel.d

# Compiles file weapon_ak74.cpp for the Release configuration...
-include gccRelease/weapon_ak74.d
gccRelease/weapon_ak74.o: weapon_ak74.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c weapon_ak74.cpp $(Release_Include_Path) -o gccRelease/weapon_ak74.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM weapon_ak74.cpp $(Release_Include_Path) > gccRelease/weapon_ak74.d

# Compiles file weapon_deagle.cpp for the Release configuration...
-include gccRelease/weapon_deagle.d
gccRelease/weapon_deagle.o: weapon_deagle.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c weapon_deagle.cpp $(Release_Include_Path) -o gccRelease/weapon_deagle.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM weapon_deagle.cpp $(Release_Include_Path) > gccRelease/weapon_deagle.d

# Compiles file weapon_f1.cpp for the Release configuration...
-include gccRelease/weapon_f1.d
gccRelease/weapon_f1.o: weapon_f1.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c weapon_f1.cpp $(Release_Include_Path) -o gccRelease/weapon_f1.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM weapon_f1.cpp $(Release_Include_Path) > gccRelease/weapon_f1.d

# Compiles file weapon_wiremine.cpp for the Release configuration...
-include gccRelease/weapon_wiremine.d
gccRelease/weapon_wiremine.o: weapon_wiremine.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c weapon_wiremine.cpp $(Release_Include_Path) -o gccRelease/weapon_wiremine.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM weapon_wiremine.cpp $(Release_Include_Path) > gccRelease/weapon_wiremine.d

# Compiles file EGUI/EQUI_Manager.cpp for the Release configuration...
-include gccRelease/EGUI/EQUI_Manager.d
gccRelease/EGUI/EQUI_Manager.o: EGUI/EQUI_Manager.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EGUI/EQUI_Manager.cpp $(Release_Include_Path) -o gccRelease/EGUI/EQUI_Manager.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EGUI/EQUI_Manager.cpp $(Release_Include_Path) > gccRelease/EGUI/EQUI_Manager.d

# Compiles file EGUI/EqUI_Panel.cpp for the Release configuration...
-include gccRelease/EGUI/EqUI_Panel.d
gccRelease/EGUI/EqUI_Panel.o: EGUI/EqUI_Panel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EGUI/EqUI_Panel.cpp $(Release_Include_Path) -o gccRelease/EGUI/EqUI_Panel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EGUI/EqUI_Panel.cpp $(Release_Include_Path) > gccRelease/EGUI/EqUI_Panel.d

# Compiles file EntityBind.cpp for the Release configuration...
-include gccRelease/EntityBind.d
gccRelease/EntityBind.o: EntityBind.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityBind.cpp $(Release_Include_Path) -o gccRelease/EntityBind.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityBind.cpp $(Release_Include_Path) > gccRelease/EntityBind.d

# Compiles file EqGMS.cpp for the Release configuration...
-include gccRelease/EqGMS.d
gccRelease/EqGMS.o: EqGMS.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EqGMS.cpp $(Release_Include_Path) -o gccRelease/EqGMS.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EqGMS.cpp $(Release_Include_Path) > gccRelease/EqGMS.d

# Compiles file EngineScriptBind.cpp for the Release configuration...
-include gccRelease/EngineScriptBind.d
gccRelease/EngineScriptBind.o: EngineScriptBind.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EngineScriptBind.cpp $(Release_Include_Path) -o gccRelease/EngineScriptBind.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EngineScriptBind.cpp $(Release_Include_Path) > gccRelease/EngineScriptBind.d

# Compiles file gmScript/gm/gmArraySimple.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmArraySimple.d
gccRelease/gmScript/gm/gmArraySimple.o: gmScript/gm/gmArraySimple.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmArraySimple.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmArraySimple.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmArraySimple.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmArraySimple.d

# Compiles file gmScript/gm/gmByteCode.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmByteCode.d
gccRelease/gmScript/gm/gmByteCode.o: gmScript/gm/gmByteCode.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmByteCode.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmByteCode.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmByteCode.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmByteCode.d

# Compiles file gmScript/gm/gmByteCodeGen.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmByteCodeGen.d
gccRelease/gmScript/gm/gmByteCodeGen.o: gmScript/gm/gmByteCodeGen.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmByteCodeGen.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmByteCodeGen.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmByteCodeGen.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmByteCodeGen.d

# Compiles file gmScript/gm/gmCodeGen.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmCodeGen.d
gccRelease/gmScript/gm/gmCodeGen.o: gmScript/gm/gmCodeGen.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmCodeGen.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmCodeGen.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmCodeGen.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmCodeGen.d

# Compiles file gmScript/gm/gmCodeGenHooks.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmCodeGenHooks.d
gccRelease/gmScript/gm/gmCodeGenHooks.o: gmScript/gm/gmCodeGenHooks.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmCodeGenHooks.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmCodeGenHooks.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmCodeGenHooks.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmCodeGenHooks.d

# Compiles file gmScript/gm/gmCodeTree.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmCodeTree.d
gccRelease/gmScript/gm/gmCodeTree.o: gmScript/gm/gmCodeTree.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmCodeTree.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmCodeTree.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmCodeTree.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmCodeTree.d

# Compiles file gmScript/gm/gmCrc.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmCrc.d
gccRelease/gmScript/gm/gmCrc.o: gmScript/gm/gmCrc.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmCrc.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmCrc.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmCrc.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmCrc.d

# Compiles file gmScript/gm/gmDebug.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmDebug.d
gccRelease/gmScript/gm/gmDebug.o: gmScript/gm/gmDebug.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmDebug.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmDebug.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmDebug.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmDebug.d

# Compiles file gmScript/gm/gmFunctionObject.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmFunctionObject.d
gccRelease/gmScript/gm/gmFunctionObject.o: gmScript/gm/gmFunctionObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmFunctionObject.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmFunctionObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmFunctionObject.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmFunctionObject.d

# Compiles file gmScript/gm/gmHash.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmHash.d
gccRelease/gmScript/gm/gmHash.o: gmScript/gm/gmHash.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmHash.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmHash.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmHash.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmHash.d

# Compiles file gmScript/gm/gmIncGC.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmIncGC.d
gccRelease/gmScript/gm/gmIncGC.o: gmScript/gm/gmIncGC.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmIncGC.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmIncGC.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmIncGC.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmIncGC.d

# Compiles file gmScript/gm/gmLibHooks.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmLibHooks.d
gccRelease/gmScript/gm/gmLibHooks.o: gmScript/gm/gmLibHooks.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmLibHooks.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmLibHooks.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmLibHooks.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmLibHooks.d

# Compiles file gmScript/gm/gmListDouble.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmListDouble.d
gccRelease/gmScript/gm/gmListDouble.o: gmScript/gm/gmListDouble.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmListDouble.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmListDouble.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmListDouble.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmListDouble.d

# Compiles file gmScript/gm/gmLog.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmLog.d
gccRelease/gmScript/gm/gmLog.o: gmScript/gm/gmLog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmLog.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmLog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmLog.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmLog.d

# Compiles file gmScript/gm/gmMachine.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmMachine.d
gccRelease/gmScript/gm/gmMachine.o: gmScript/gm/gmMachine.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmMachine.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmMachine.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmMachine.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmMachine.d

# Compiles file gmScript/gm/gmMachineLib.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmMachineLib.d
gccRelease/gmScript/gm/gmMachineLib.o: gmScript/gm/gmMachineLib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmMachineLib.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmMachineLib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmMachineLib.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmMachineLib.d

# Compiles file gmScript/gm/gmMem.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmMem.d
gccRelease/gmScript/gm/gmMem.o: gmScript/gm/gmMem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmMem.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmMem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmMem.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmMem.d

# Compiles file gmScript/gm/gmMemChain.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmMemChain.d
gccRelease/gmScript/gm/gmMemChain.o: gmScript/gm/gmMemChain.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmMemChain.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmMemChain.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmMemChain.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmMemChain.d

# Compiles file gmScript/gm/gmMemFixed.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmMemFixed.d
gccRelease/gmScript/gm/gmMemFixed.o: gmScript/gm/gmMemFixed.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmMemFixed.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmMemFixed.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmMemFixed.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmMemFixed.d

# Compiles file gmScript/gm/gmMemFixedSet.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmMemFixedSet.d
gccRelease/gmScript/gm/gmMemFixedSet.o: gmScript/gm/gmMemFixedSet.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmMemFixedSet.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmMemFixedSet.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmMemFixedSet.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmMemFixedSet.d

# Compiles file gmScript/gm/gmOperators.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmOperators.d
gccRelease/gmScript/gm/gmOperators.o: gmScript/gm/gmOperators.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmOperators.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmOperators.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmOperators.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmOperators.d

# Compiles file gmScript/gm/gmParser.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmParser.d
gccRelease/gmScript/gm/gmParser.o: gmScript/gm/gmParser.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmParser.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmParser.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmParser.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmParser.d

# Compiles file gmScript/gm/gmScanner.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmScanner.d
gccRelease/gmScript/gm/gmScanner.o: gmScript/gm/gmScanner.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmScanner.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmScanner.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmScanner.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmScanner.d

# Compiles file gmScript/gm/gmStream.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmStream.d
gccRelease/gmScript/gm/gmStream.o: gmScript/gm/gmStream.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmStream.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmStream.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmStream.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmStream.d

# Compiles file gmScript/gm/gmStreamBuffer.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmStreamBuffer.d
gccRelease/gmScript/gm/gmStreamBuffer.o: gmScript/gm/gmStreamBuffer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmStreamBuffer.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmStreamBuffer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmStreamBuffer.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmStreamBuffer.d

# Compiles file gmScript/gm/gmStringObject.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmStringObject.d
gccRelease/gmScript/gm/gmStringObject.o: gmScript/gm/gmStringObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmStringObject.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmStringObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmStringObject.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmStringObject.d

# Compiles file gmScript/gm/gmTableObject.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmTableObject.d
gccRelease/gmScript/gm/gmTableObject.o: gmScript/gm/gmTableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmTableObject.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmTableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmTableObject.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmTableObject.d

# Compiles file gmScript/gm/gmThread.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmThread.d
gccRelease/gmScript/gm/gmThread.o: gmScript/gm/gmThread.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmThread.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmThread.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmThread.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmThread.d

# Compiles file gmScript/gm/gmUserObject.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmUserObject.d
gccRelease/gmScript/gm/gmUserObject.o: gmScript/gm/gmUserObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmUserObject.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmUserObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmUserObject.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmUserObject.d

# Compiles file gmScript/gm/gmUtil.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmUtil.d
gccRelease/gmScript/gm/gmUtil.o: gmScript/gm/gmUtil.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmUtil.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmUtil.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmUtil.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmUtil.d

# Compiles file gmScript/gm/gmVariable.cpp for the Release configuration...
-include gccRelease/gmScript/gm/gmVariable.d
gccRelease/gmScript/gm/gmVariable.o: gmScript/gm/gmVariable.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/gm/gmVariable.cpp $(Release_Include_Path) -o gccRelease/gmScript/gm/gmVariable.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/gm/gmVariable.cpp $(Release_Include_Path) > gccRelease/gmScript/gm/gmVariable.d

# Compiles file gmScript/binds/gmArrayLib.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmArrayLib.d
gccRelease/gmScript/binds/gmArrayLib.o: gmScript/binds/gmArrayLib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmArrayLib.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmArrayLib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmArrayLib.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmArrayLib.d

# Compiles file gmScript/binds/gmCall.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmCall.d
gccRelease/gmScript/binds/gmCall.o: gmScript/binds/gmCall.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmCall.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmCall.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmCall.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmCall.d

# Compiles file gmScript/binds/gmGCRoot.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmGCRoot.d
gccRelease/gmScript/binds/gmGCRoot.o: gmScript/binds/gmGCRoot.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmGCRoot.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmGCRoot.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmGCRoot.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmGCRoot.d

# Compiles file gmScript/binds/gmGCRootUtil.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmGCRootUtil.d
gccRelease/gmScript/binds/gmGCRootUtil.o: gmScript/binds/gmGCRootUtil.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmGCRootUtil.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmGCRootUtil.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmGCRootUtil.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmGCRootUtil.d

# Compiles file gmScript/binds/gmHelpers.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmHelpers.d
gccRelease/gmScript/binds/gmHelpers.o: gmScript/binds/gmHelpers.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmHelpers.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmHelpers.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmHelpers.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmHelpers.d

# Compiles file gmScript/binds/gmMathLib.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmMathLib.d
gccRelease/gmScript/binds/gmMathLib.o: gmScript/binds/gmMathLib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmMathLib.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmMathLib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmMathLib.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmMathLib.d

# Compiles file gmScript/binds/gmStringLib.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmStringLib.d
gccRelease/gmScript/binds/gmStringLib.o: gmScript/binds/gmStringLib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmStringLib.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmStringLib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmStringLib.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmStringLib.d

# Compiles file gmScript/binds/gmSystemLib.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmSystemLib.d
gccRelease/gmScript/binds/gmSystemLib.o: gmScript/binds/gmSystemLib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmSystemLib.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmSystemLib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmSystemLib.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmSystemLib.d

# Compiles file gmScript/binds/gmVector3Lib.cpp for the Release configuration...
-include gccRelease/gmScript/binds/gmVector3Lib.d
gccRelease/gmScript/binds/gmVector3Lib.o: gmScript/binds/gmVector3Lib.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c gmScript/binds/gmVector3Lib.cpp $(Release_Include_Path) -o gccRelease/gmScript/binds/gmVector3Lib.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM gmScript/binds/gmVector3Lib.cpp $(Release_Include_Path) > gccRelease/gmScript/binds/gmVector3Lib.d

# Builds the STDGameRelease configuration...
.PHONY: STDGameRelease
STDGameRelease: create_folders gccSTDGameRelease/../public/Utils/GeomTools.o gccSTDGameRelease/../public/Math/math_util.o gccSTDGameRelease/../public/BaseShader.o gccSTDGameRelease/../public/BaseRenderableObject.o gccSTDGameRelease/../public/RenderList.o gccSTDGameRelease/../public/ViewParams.o gccSTDGameRelease/../public/dsm_esm_loader.o gccSTDGameRelease/../public/dsm_loader.o gccSTDGameRelease/../public/dsm_obj_loader.o gccSTDGameRelease/GameLibrary.o gccSTDGameRelease/GameState.o gccSTDGameRelease/AI/AIBaseNPC.o gccSTDGameRelease/AI/AINode.o gccSTDGameRelease/AI/AITaskTypes.o gccSTDGameRelease/AI/AI_Idle.o gccSTDGameRelease/AI/AI_MovementTarget.o gccSTDGameRelease/AI/ai_navigator.o gccSTDGameRelease/AI/Detour/Source/DetourAlloc.o gccSTDGameRelease/AI/Detour/Source/DetourCommon.o gccSTDGameRelease/AI/Detour/Source/DetourNavMesh.o gccSTDGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o gccSTDGameRelease/AI/Detour/Source/DetourNavMeshQuery.o gccSTDGameRelease/AI/Detour/Source/DetourNode.o gccSTDGameRelease/AI/Recast/Source/Recast.o gccSTDGameRelease/AI/Recast/Source/RecastAlloc.o gccSTDGameRelease/AI/Recast/Source/RecastArea.o gccSTDGameRelease/AI/Recast/Source/RecastContour.o gccSTDGameRelease/AI/Recast/Source/RecastFilter.o gccSTDGameRelease/AI/Recast/Source/RecastLayers.o gccSTDGameRelease/AI/Recast/Source/RecastMesh.o gccSTDGameRelease/AI/Recast/Source/RecastMeshDetail.o gccSTDGameRelease/AI/Recast/Source/RecastRasterization.o gccSTDGameRelease/AI/Recast/Source/RecastRegion.o gccSTDGameRelease/AI/AIMemoryBase.o gccSTDGameRelease/AI/AIScriptTask.o gccSTDGameRelease/AI/AITaskActionBase.o gccSTDGameRelease/AmmoType.o gccSTDGameRelease/BaseEntity.o gccSTDGameRelease/InfoCamera.o gccSTDGameRelease/Ladder.o gccSTDGameRelease/LogicEntities.o gccSTDGameRelease/BulletSimulator.o gccSTDGameRelease/BaseActor.o gccSTDGameRelease/BaseAnimating.o gccSTDGameRelease/BaseWeapon.o gccSTDGameRelease/EffectEntities.o gccSTDGameRelease/EngineEntities.o gccSTDGameRelease/PlayerSpawn.o gccSTDGameRelease/ModelEntities.o gccSTDGameRelease/SoundEntities.o gccSTDGameRelease/Triggers.o gccSTDGameRelease/../public/anim_activity.o gccSTDGameRelease/../public/anim_events.o gccSTDGameRelease/../public/BoneSetup.o gccSTDGameRelease/../public/ragdoll.o gccSTDGameRelease/EntityQueue.o gccSTDGameRelease/EntityDataField.o gccSTDGameRelease/GameTrace.o gccSTDGameRelease/Effects.o gccSTDGameRelease/GameInput.o gccSTDGameRelease/GlobalVarsGame.o gccSTDGameRelease/MaterialProxy.o gccSTDGameRelease/Rain.o gccSTDGameRelease/Snow.o gccSTDGameRelease/SaveGame_Events.o gccSTDGameRelease/SaveRestoreManager.o gccSTDGameRelease/EffectRender.o gccSTDGameRelease/FilterPipelineBase.o gccSTDGameRelease/GameRenderer.o gccSTDGameRelease/ParticleRenderer.o gccSTDGameRelease/RenderDefs.o gccSTDGameRelease/../public/GameSoundEmitterSystem.o gccSTDGameRelease/PigeonGame/BasePigeon.o gccSTDGameRelease/PigeonGame/GameRules_Pigeon.o gccSTDGameRelease/PigeonGame/Pigeon_Player.o gccSTDGameRelease/GameRules_WhiteCage.o gccSTDGameRelease/npc_cyborg.o gccSTDGameRelease/npc_soldier.o gccSTDGameRelease/Player.o gccSTDGameRelease/viewmodel.o gccSTDGameRelease/weapon_ak74.o gccSTDGameRelease/weapon_deagle.o gccSTDGameRelease/weapon_f1.o gccSTDGameRelease/weapon_wiremine.o gccSTDGameRelease/EGUI/EQUI_Manager.o gccSTDGameRelease/EGUI/EqUI_Panel.o gccSTDGameRelease/EntityBind.o gccSTDGameRelease/EqGMS.o gccSTDGameRelease/EngineScriptBind.o gccSTDGameRelease/gmScript/gm/gmArraySimple.o gccSTDGameRelease/gmScript/gm/gmByteCode.o gccSTDGameRelease/gmScript/gm/gmByteCodeGen.o gccSTDGameRelease/gmScript/gm/gmCodeGen.o gccSTDGameRelease/gmScript/gm/gmCodeGenHooks.o gccSTDGameRelease/gmScript/gm/gmCodeTree.o gccSTDGameRelease/gmScript/gm/gmCrc.o gccSTDGameRelease/gmScript/gm/gmDebug.o gccSTDGameRelease/gmScript/gm/gmFunctionObject.o gccSTDGameRelease/gmScript/gm/gmHash.o gccSTDGameRelease/gmScript/gm/gmIncGC.o gccSTDGameRelease/gmScript/gm/gmLibHooks.o gccSTDGameRelease/gmScript/gm/gmListDouble.o gccSTDGameRelease/gmScript/gm/gmLog.o gccSTDGameRelease/gmScript/gm/gmMachine.o gccSTDGameRelease/gmScript/gm/gmMachineLib.o gccSTDGameRelease/gmScript/gm/gmMem.o gccSTDGameRelease/gmScript/gm/gmMemChain.o gccSTDGameRelease/gmScript/gm/gmMemFixed.o gccSTDGameRelease/gmScript/gm/gmMemFixedSet.o gccSTDGameRelease/gmScript/gm/gmOperators.o gccSTDGameRelease/gmScript/gm/gmParser.o gccSTDGameRelease/gmScript/gm/gmScanner.o gccSTDGameRelease/gmScript/gm/gmStream.o gccSTDGameRelease/gmScript/gm/gmStreamBuffer.o gccSTDGameRelease/gmScript/gm/gmStringObject.o gccSTDGameRelease/gmScript/gm/gmTableObject.o gccSTDGameRelease/gmScript/gm/gmThread.o gccSTDGameRelease/gmScript/gm/gmUserObject.o gccSTDGameRelease/gmScript/gm/gmUtil.o gccSTDGameRelease/gmScript/gm/gmVariable.o gccSTDGameRelease/gmScript/binds/gmArrayLib.o gccSTDGameRelease/gmScript/binds/gmCall.o gccSTDGameRelease/gmScript/binds/gmGCRoot.o gccSTDGameRelease/gmScript/binds/gmGCRootUtil.o gccSTDGameRelease/gmScript/binds/gmHelpers.o gccSTDGameRelease/gmScript/binds/gmMathLib.o gccSTDGameRelease/gmScript/binds/gmStringLib.o gccSTDGameRelease/gmScript/binds/gmSystemLib.o gccSTDGameRelease/gmScript/binds/gmVector3Lib.o 
	g++ -fPIC -shared -Wl,-soname,libGameDLL.so -o gccSTDGameRelease/libGameDLL.so gccSTDGameRelease/../public/Utils/GeomTools.o gccSTDGameRelease/../public/Math/math_util.o gccSTDGameRelease/../public/BaseShader.o gccSTDGameRelease/../public/BaseRenderableObject.o gccSTDGameRelease/../public/RenderList.o gccSTDGameRelease/../public/ViewParams.o gccSTDGameRelease/../public/dsm_esm_loader.o gccSTDGameRelease/../public/dsm_loader.o gccSTDGameRelease/../public/dsm_obj_loader.o gccSTDGameRelease/GameLibrary.o gccSTDGameRelease/GameState.o gccSTDGameRelease/AI/AIBaseNPC.o gccSTDGameRelease/AI/AINode.o gccSTDGameRelease/AI/AITaskTypes.o gccSTDGameRelease/AI/AI_Idle.o gccSTDGameRelease/AI/AI_MovementTarget.o gccSTDGameRelease/AI/ai_navigator.o gccSTDGameRelease/AI/Detour/Source/DetourAlloc.o gccSTDGameRelease/AI/Detour/Source/DetourCommon.o gccSTDGameRelease/AI/Detour/Source/DetourNavMesh.o gccSTDGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o gccSTDGameRelease/AI/Detour/Source/DetourNavMeshQuery.o gccSTDGameRelease/AI/Detour/Source/DetourNode.o gccSTDGameRelease/AI/Recast/Source/Recast.o gccSTDGameRelease/AI/Recast/Source/RecastAlloc.o gccSTDGameRelease/AI/Recast/Source/RecastArea.o gccSTDGameRelease/AI/Recast/Source/RecastContour.o gccSTDGameRelease/AI/Recast/Source/RecastFilter.o gccSTDGameRelease/AI/Recast/Source/RecastLayers.o gccSTDGameRelease/AI/Recast/Source/RecastMesh.o gccSTDGameRelease/AI/Recast/Source/RecastMeshDetail.o gccSTDGameRelease/AI/Recast/Source/RecastRasterization.o gccSTDGameRelease/AI/Recast/Source/RecastRegion.o gccSTDGameRelease/AI/AIMemoryBase.o gccSTDGameRelease/AI/AIScriptTask.o gccSTDGameRelease/AI/AITaskActionBase.o gccSTDGameRelease/AmmoType.o gccSTDGameRelease/BaseEntity.o gccSTDGameRelease/InfoCamera.o gccSTDGameRelease/Ladder.o gccSTDGameRelease/LogicEntities.o gccSTDGameRelease/BulletSimulator.o gccSTDGameRelease/BaseActor.o gccSTDGameRelease/BaseAnimating.o gccSTDGameRelease/BaseWeapon.o gccSTDGameRelease/EffectEntities.o gccSTDGameRelease/EngineEntities.o gccSTDGameRelease/PlayerSpawn.o gccSTDGameRelease/ModelEntities.o gccSTDGameRelease/SoundEntities.o gccSTDGameRelease/Triggers.o gccSTDGameRelease/../public/anim_activity.o gccSTDGameRelease/../public/anim_events.o gccSTDGameRelease/../public/BoneSetup.o gccSTDGameRelease/../public/ragdoll.o gccSTDGameRelease/EntityQueue.o gccSTDGameRelease/EntityDataField.o gccSTDGameRelease/GameTrace.o gccSTDGameRelease/Effects.o gccSTDGameRelease/GameInput.o gccSTDGameRelease/GlobalVarsGame.o gccSTDGameRelease/MaterialProxy.o gccSTDGameRelease/Rain.o gccSTDGameRelease/Snow.o gccSTDGameRelease/SaveGame_Events.o gccSTDGameRelease/SaveRestoreManager.o gccSTDGameRelease/EffectRender.o gccSTDGameRelease/FilterPipelineBase.o gccSTDGameRelease/GameRenderer.o gccSTDGameRelease/ParticleRenderer.o gccSTDGameRelease/RenderDefs.o gccSTDGameRelease/../public/GameSoundEmitterSystem.o gccSTDGameRelease/PigeonGame/BasePigeon.o gccSTDGameRelease/PigeonGame/GameRules_Pigeon.o gccSTDGameRelease/PigeonGame/Pigeon_Player.o gccSTDGameRelease/GameRules_WhiteCage.o gccSTDGameRelease/npc_cyborg.o gccSTDGameRelease/npc_soldier.o gccSTDGameRelease/Player.o gccSTDGameRelease/viewmodel.o gccSTDGameRelease/weapon_ak74.o gccSTDGameRelease/weapon_deagle.o gccSTDGameRelease/weapon_f1.o gccSTDGameRelease/weapon_wiremine.o gccSTDGameRelease/EGUI/EQUI_Manager.o gccSTDGameRelease/EGUI/EqUI_Panel.o gccSTDGameRelease/EntityBind.o gccSTDGameRelease/EqGMS.o gccSTDGameRelease/EngineScriptBind.o gccSTDGameRelease/gmScript/gm/gmArraySimple.o gccSTDGameRelease/gmScript/gm/gmByteCode.o gccSTDGameRelease/gmScript/gm/gmByteCodeGen.o gccSTDGameRelease/gmScript/gm/gmCodeGen.o gccSTDGameRelease/gmScript/gm/gmCodeGenHooks.o gccSTDGameRelease/gmScript/gm/gmCodeTree.o gccSTDGameRelease/gmScript/gm/gmCrc.o gccSTDGameRelease/gmScript/gm/gmDebug.o gccSTDGameRelease/gmScript/gm/gmFunctionObject.o gccSTDGameRelease/gmScript/gm/gmHash.o gccSTDGameRelease/gmScript/gm/gmIncGC.o gccSTDGameRelease/gmScript/gm/gmLibHooks.o gccSTDGameRelease/gmScript/gm/gmListDouble.o gccSTDGameRelease/gmScript/gm/gmLog.o gccSTDGameRelease/gmScript/gm/gmMachine.o gccSTDGameRelease/gmScript/gm/gmMachineLib.o gccSTDGameRelease/gmScript/gm/gmMem.o gccSTDGameRelease/gmScript/gm/gmMemChain.o gccSTDGameRelease/gmScript/gm/gmMemFixed.o gccSTDGameRelease/gmScript/gm/gmMemFixedSet.o gccSTDGameRelease/gmScript/gm/gmOperators.o gccSTDGameRelease/gmScript/gm/gmParser.o gccSTDGameRelease/gmScript/gm/gmScanner.o gccSTDGameRelease/gmScript/gm/gmStream.o gccSTDGameRelease/gmScript/gm/gmStreamBuffer.o gccSTDGameRelease/gmScript/gm/gmStringObject.o gccSTDGameRelease/gmScript/gm/gmTableObject.o gccSTDGameRelease/gmScript/gm/gmThread.o gccSTDGameRelease/gmScript/gm/gmUserObject.o gccSTDGameRelease/gmScript/gm/gmUtil.o gccSTDGameRelease/gmScript/gm/gmVariable.o gccSTDGameRelease/gmScript/binds/gmArrayLib.o gccSTDGameRelease/gmScript/binds/gmCall.o gccSTDGameRelease/gmScript/binds/gmGCRoot.o gccSTDGameRelease/gmScript/binds/gmGCRootUtil.o gccSTDGameRelease/gmScript/binds/gmHelpers.o gccSTDGameRelease/gmScript/binds/gmMathLib.o gccSTDGameRelease/gmScript/binds/gmStringLib.o gccSTDGameRelease/gmScript/binds/gmSystemLib.o gccSTDGameRelease/gmScript/binds/gmVector3Lib.o  $(STDGameRelease_Implicitly_Linked_Objects)

# Compiles file ../public/Utils/GeomTools.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/Utils/GeomTools.d
gccSTDGameRelease/../public/Utils/GeomTools.o: ../public/Utils/GeomTools.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/Utils/GeomTools.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/Utils/GeomTools.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/Utils/GeomTools.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/Utils/GeomTools.d

# Compiles file ../public/Math/math_util.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/Math/math_util.d
gccSTDGameRelease/../public/Math/math_util.o: ../public/Math/math_util.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/Math/math_util.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/Math/math_util.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/Math/math_util.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/Math/math_util.d

# Compiles file ../public/BaseShader.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/BaseShader.d
gccSTDGameRelease/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/BaseShader.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/BaseShader.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/BaseShader.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/BaseShader.d

# Compiles file ../public/BaseRenderableObject.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/BaseRenderableObject.d
gccSTDGameRelease/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/BaseRenderableObject.d

# Compiles file ../public/RenderList.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/RenderList.d
gccSTDGameRelease/../public/RenderList.o: ../public/RenderList.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/RenderList.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/RenderList.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/RenderList.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/RenderList.d

# Compiles file ../public/ViewParams.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/ViewParams.d
gccSTDGameRelease/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/ViewParams.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/ViewParams.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/ViewParams.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/ViewParams.d

# Compiles file ../public/dsm_esm_loader.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/dsm_esm_loader.d
gccSTDGameRelease/../public/dsm_esm_loader.o: ../public/dsm_esm_loader.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/dsm_esm_loader.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/dsm_esm_loader.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/dsm_esm_loader.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/dsm_esm_loader.d

# Compiles file ../public/dsm_loader.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/dsm_loader.d
gccSTDGameRelease/../public/dsm_loader.o: ../public/dsm_loader.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/dsm_loader.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/dsm_loader.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/dsm_loader.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/dsm_loader.d

# Compiles file ../public/dsm_obj_loader.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/dsm_obj_loader.d
gccSTDGameRelease/../public/dsm_obj_loader.o: ../public/dsm_obj_loader.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/dsm_obj_loader.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/dsm_obj_loader.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/dsm_obj_loader.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/dsm_obj_loader.d

# Compiles file GameLibrary.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GameLibrary.d
gccSTDGameRelease/GameLibrary.o: GameLibrary.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GameLibrary.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GameLibrary.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GameLibrary.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GameLibrary.d

# Compiles file GameState.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GameState.d
gccSTDGameRelease/GameState.o: GameState.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GameState.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GameState.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GameState.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GameState.d

# Compiles file AI/AIBaseNPC.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AIBaseNPC.d
gccSTDGameRelease/AI/AIBaseNPC.o: AI/AIBaseNPC.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AIBaseNPC.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AIBaseNPC.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AIBaseNPC.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AIBaseNPC.d

# Compiles file AI/AINode.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AINode.d
gccSTDGameRelease/AI/AINode.o: AI/AINode.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AINode.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AINode.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AINode.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AINode.d

# Compiles file AI/AITaskTypes.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AITaskTypes.d
gccSTDGameRelease/AI/AITaskTypes.o: AI/AITaskTypes.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AITaskTypes.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AITaskTypes.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AITaskTypes.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AITaskTypes.d

# Compiles file AI/AI_Idle.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AI_Idle.d
gccSTDGameRelease/AI/AI_Idle.o: AI/AI_Idle.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AI_Idle.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AI_Idle.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AI_Idle.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AI_Idle.d

# Compiles file AI/AI_MovementTarget.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AI_MovementTarget.d
gccSTDGameRelease/AI/AI_MovementTarget.o: AI/AI_MovementTarget.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AI_MovementTarget.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AI_MovementTarget.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AI_MovementTarget.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AI_MovementTarget.d

# Compiles file AI/ai_navigator.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/ai_navigator.d
gccSTDGameRelease/AI/ai_navigator.o: AI/ai_navigator.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/ai_navigator.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/ai_navigator.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/ai_navigator.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/ai_navigator.d

# Compiles file AI/Detour/Source/DetourAlloc.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Detour/Source/DetourAlloc.d
gccSTDGameRelease/AI/Detour/Source/DetourAlloc.o: AI/Detour/Source/DetourAlloc.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourAlloc.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Detour/Source/DetourAlloc.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourAlloc.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Detour/Source/DetourAlloc.d

# Compiles file AI/Detour/Source/DetourCommon.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Detour/Source/DetourCommon.d
gccSTDGameRelease/AI/Detour/Source/DetourCommon.o: AI/Detour/Source/DetourCommon.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourCommon.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Detour/Source/DetourCommon.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourCommon.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Detour/Source/DetourCommon.d

# Compiles file AI/Detour/Source/DetourNavMesh.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Detour/Source/DetourNavMesh.d
gccSTDGameRelease/AI/Detour/Source/DetourNavMesh.o: AI/Detour/Source/DetourNavMesh.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNavMesh.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Detour/Source/DetourNavMesh.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNavMesh.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Detour/Source/DetourNavMesh.d

# Compiles file AI/Detour/Source/DetourNavMeshBuilder.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Detour/Source/DetourNavMeshBuilder.d
gccSTDGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o: AI/Detour/Source/DetourNavMeshBuilder.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshBuilder.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Detour/Source/DetourNavMeshBuilder.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshBuilder.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Detour/Source/DetourNavMeshBuilder.d

# Compiles file AI/Detour/Source/DetourNavMeshQuery.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Detour/Source/DetourNavMeshQuery.d
gccSTDGameRelease/AI/Detour/Source/DetourNavMeshQuery.o: AI/Detour/Source/DetourNavMeshQuery.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNavMeshQuery.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Detour/Source/DetourNavMeshQuery.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNavMeshQuery.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Detour/Source/DetourNavMeshQuery.d

# Compiles file AI/Detour/Source/DetourNode.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Detour/Source/DetourNode.d
gccSTDGameRelease/AI/Detour/Source/DetourNode.o: AI/Detour/Source/DetourNode.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Detour/Source/DetourNode.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Detour/Source/DetourNode.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Detour/Source/DetourNode.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Detour/Source/DetourNode.d

# Compiles file AI/Recast/Source/Recast.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/Recast.d
gccSTDGameRelease/AI/Recast/Source/Recast.o: AI/Recast/Source/Recast.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/Recast.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/Recast.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/Recast.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/Recast.d

# Compiles file AI/Recast/Source/RecastAlloc.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastAlloc.d
gccSTDGameRelease/AI/Recast/Source/RecastAlloc.o: AI/Recast/Source/RecastAlloc.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastAlloc.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastAlloc.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastAlloc.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastAlloc.d

# Compiles file AI/Recast/Source/RecastArea.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastArea.d
gccSTDGameRelease/AI/Recast/Source/RecastArea.o: AI/Recast/Source/RecastArea.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastArea.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastArea.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastArea.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastArea.d

# Compiles file AI/Recast/Source/RecastContour.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastContour.d
gccSTDGameRelease/AI/Recast/Source/RecastContour.o: AI/Recast/Source/RecastContour.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastContour.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastContour.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastContour.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastContour.d

# Compiles file AI/Recast/Source/RecastFilter.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastFilter.d
gccSTDGameRelease/AI/Recast/Source/RecastFilter.o: AI/Recast/Source/RecastFilter.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastFilter.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastFilter.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastFilter.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastFilter.d

# Compiles file AI/Recast/Source/RecastLayers.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastLayers.d
gccSTDGameRelease/AI/Recast/Source/RecastLayers.o: AI/Recast/Source/RecastLayers.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastLayers.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastLayers.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastLayers.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastLayers.d

# Compiles file AI/Recast/Source/RecastMesh.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastMesh.d
gccSTDGameRelease/AI/Recast/Source/RecastMesh.o: AI/Recast/Source/RecastMesh.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastMesh.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastMesh.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastMesh.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastMesh.d

# Compiles file AI/Recast/Source/RecastMeshDetail.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastMeshDetail.d
gccSTDGameRelease/AI/Recast/Source/RecastMeshDetail.o: AI/Recast/Source/RecastMeshDetail.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastMeshDetail.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastMeshDetail.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastMeshDetail.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastMeshDetail.d

# Compiles file AI/Recast/Source/RecastRasterization.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastRasterization.d
gccSTDGameRelease/AI/Recast/Source/RecastRasterization.o: AI/Recast/Source/RecastRasterization.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastRasterization.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastRasterization.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastRasterization.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastRasterization.d

# Compiles file AI/Recast/Source/RecastRegion.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/Recast/Source/RecastRegion.d
gccSTDGameRelease/AI/Recast/Source/RecastRegion.o: AI/Recast/Source/RecastRegion.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/Recast/Source/RecastRegion.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/Recast/Source/RecastRegion.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/Recast/Source/RecastRegion.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/Recast/Source/RecastRegion.d

# Compiles file AI/AIMemoryBase.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AIMemoryBase.d
gccSTDGameRelease/AI/AIMemoryBase.o: AI/AIMemoryBase.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AIMemoryBase.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AIMemoryBase.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AIMemoryBase.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AIMemoryBase.d

# Compiles file AI/AIScriptTask.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AIScriptTask.d
gccSTDGameRelease/AI/AIScriptTask.o: AI/AIScriptTask.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AIScriptTask.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AIScriptTask.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AIScriptTask.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AIScriptTask.d

# Compiles file AI/AITaskActionBase.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AI/AITaskActionBase.d
gccSTDGameRelease/AI/AITaskActionBase.o: AI/AITaskActionBase.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AI/AITaskActionBase.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AI/AITaskActionBase.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AI/AITaskActionBase.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AI/AITaskActionBase.d

# Compiles file AmmoType.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/AmmoType.d
gccSTDGameRelease/AmmoType.o: AmmoType.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c AmmoType.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/AmmoType.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM AmmoType.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/AmmoType.d

# Compiles file BaseEntity.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/BaseEntity.d
gccSTDGameRelease/BaseEntity.o: BaseEntity.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c BaseEntity.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/BaseEntity.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM BaseEntity.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/BaseEntity.d

# Compiles file InfoCamera.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/InfoCamera.d
gccSTDGameRelease/InfoCamera.o: InfoCamera.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c InfoCamera.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/InfoCamera.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM InfoCamera.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/InfoCamera.d

# Compiles file Ladder.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/Ladder.d
gccSTDGameRelease/Ladder.o: Ladder.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c Ladder.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/Ladder.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM Ladder.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/Ladder.d

# Compiles file LogicEntities.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/LogicEntities.d
gccSTDGameRelease/LogicEntities.o: LogicEntities.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c LogicEntities.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/LogicEntities.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM LogicEntities.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/LogicEntities.d

# Compiles file BulletSimulator.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/BulletSimulator.d
gccSTDGameRelease/BulletSimulator.o: BulletSimulator.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c BulletSimulator.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/BulletSimulator.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM BulletSimulator.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/BulletSimulator.d

# Compiles file BaseActor.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/BaseActor.d
gccSTDGameRelease/BaseActor.o: BaseActor.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c BaseActor.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/BaseActor.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM BaseActor.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/BaseActor.d

# Compiles file BaseAnimating.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/BaseAnimating.d
gccSTDGameRelease/BaseAnimating.o: BaseAnimating.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c BaseAnimating.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/BaseAnimating.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM BaseAnimating.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/BaseAnimating.d

# Compiles file BaseWeapon.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/BaseWeapon.d
gccSTDGameRelease/BaseWeapon.o: BaseWeapon.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c BaseWeapon.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/BaseWeapon.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM BaseWeapon.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/BaseWeapon.d

# Compiles file EffectEntities.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EffectEntities.d
gccSTDGameRelease/EffectEntities.o: EffectEntities.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EffectEntities.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EffectEntities.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EffectEntities.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EffectEntities.d

# Compiles file EngineEntities.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EngineEntities.d
gccSTDGameRelease/EngineEntities.o: EngineEntities.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EngineEntities.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EngineEntities.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EngineEntities.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EngineEntities.d

# Compiles file PlayerSpawn.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/PlayerSpawn.d
gccSTDGameRelease/PlayerSpawn.o: PlayerSpawn.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c PlayerSpawn.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/PlayerSpawn.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM PlayerSpawn.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/PlayerSpawn.d

# Compiles file ModelEntities.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/ModelEntities.d
gccSTDGameRelease/ModelEntities.o: ModelEntities.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ModelEntities.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/ModelEntities.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ModelEntities.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/ModelEntities.d

# Compiles file SoundEntities.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/SoundEntities.d
gccSTDGameRelease/SoundEntities.o: SoundEntities.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c SoundEntities.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/SoundEntities.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM SoundEntities.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/SoundEntities.d

# Compiles file Triggers.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/Triggers.d
gccSTDGameRelease/Triggers.o: Triggers.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c Triggers.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/Triggers.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM Triggers.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/Triggers.d

# Compiles file ../public/anim_activity.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/anim_activity.d
gccSTDGameRelease/../public/anim_activity.o: ../public/anim_activity.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/anim_activity.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/anim_activity.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/anim_activity.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/anim_activity.d

# Compiles file ../public/anim_events.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/anim_events.d
gccSTDGameRelease/../public/anim_events.o: ../public/anim_events.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/anim_events.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/anim_events.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/anim_events.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/anim_events.d

# Compiles file ../public/BoneSetup.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/BoneSetup.d
gccSTDGameRelease/../public/BoneSetup.o: ../public/BoneSetup.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/BoneSetup.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/BoneSetup.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/BoneSetup.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/BoneSetup.d

# Compiles file ../public/ragdoll.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/ragdoll.d
gccSTDGameRelease/../public/ragdoll.o: ../public/ragdoll.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/ragdoll.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/ragdoll.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/ragdoll.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/ragdoll.d

# Compiles file EntityQueue.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EntityQueue.d
gccSTDGameRelease/EntityQueue.o: EntityQueue.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EntityQueue.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EntityQueue.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EntityQueue.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EntityQueue.d

# Compiles file EntityDataField.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EntityDataField.d
gccSTDGameRelease/EntityDataField.o: EntityDataField.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EntityDataField.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EntityDataField.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EntityDataField.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EntityDataField.d

# Compiles file GameTrace.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GameTrace.d
gccSTDGameRelease/GameTrace.o: GameTrace.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GameTrace.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GameTrace.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GameTrace.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GameTrace.d

# Compiles file Effects.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/Effects.d
gccSTDGameRelease/Effects.o: Effects.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c Effects.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/Effects.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM Effects.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/Effects.d

# Compiles file GameInput.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GameInput.d
gccSTDGameRelease/GameInput.o: GameInput.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GameInput.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GameInput.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GameInput.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GameInput.d

# Compiles file GlobalVarsGame.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GlobalVarsGame.d
gccSTDGameRelease/GlobalVarsGame.o: GlobalVarsGame.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GlobalVarsGame.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GlobalVarsGame.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GlobalVarsGame.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GlobalVarsGame.d

# Compiles file MaterialProxy.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/MaterialProxy.d
gccSTDGameRelease/MaterialProxy.o: MaterialProxy.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c MaterialProxy.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/MaterialProxy.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM MaterialProxy.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/MaterialProxy.d

# Compiles file Rain.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/Rain.d
gccSTDGameRelease/Rain.o: Rain.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c Rain.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/Rain.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM Rain.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/Rain.d

# Compiles file Snow.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/Snow.d
gccSTDGameRelease/Snow.o: Snow.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c Snow.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/Snow.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM Snow.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/Snow.d

# Compiles file SaveGame_Events.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/SaveGame_Events.d
gccSTDGameRelease/SaveGame_Events.o: SaveGame_Events.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c SaveGame_Events.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/SaveGame_Events.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM SaveGame_Events.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/SaveGame_Events.d

# Compiles file SaveRestoreManager.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/SaveRestoreManager.d
gccSTDGameRelease/SaveRestoreManager.o: SaveRestoreManager.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c SaveRestoreManager.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/SaveRestoreManager.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM SaveRestoreManager.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/SaveRestoreManager.d

# Compiles file EffectRender.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EffectRender.d
gccSTDGameRelease/EffectRender.o: EffectRender.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EffectRender.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EffectRender.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EffectRender.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EffectRender.d

# Compiles file FilterPipelineBase.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/FilterPipelineBase.d
gccSTDGameRelease/FilterPipelineBase.o: FilterPipelineBase.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c FilterPipelineBase.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/FilterPipelineBase.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM FilterPipelineBase.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/FilterPipelineBase.d

# Compiles file GameRenderer.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GameRenderer.d
gccSTDGameRelease/GameRenderer.o: GameRenderer.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GameRenderer.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GameRenderer.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GameRenderer.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GameRenderer.d

# Compiles file ParticleRenderer.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/ParticleRenderer.d
gccSTDGameRelease/ParticleRenderer.o: ParticleRenderer.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ParticleRenderer.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/ParticleRenderer.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ParticleRenderer.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/ParticleRenderer.d

# Compiles file RenderDefs.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/RenderDefs.d
gccSTDGameRelease/RenderDefs.o: RenderDefs.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c RenderDefs.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/RenderDefs.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM RenderDefs.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/RenderDefs.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/../public/GameSoundEmitterSystem.d
gccSTDGameRelease/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/../public/GameSoundEmitterSystem.d

# Compiles file PigeonGame/BasePigeon.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/PigeonGame/BasePigeon.d
gccSTDGameRelease/PigeonGame/BasePigeon.o: PigeonGame/BasePigeon.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c PigeonGame/BasePigeon.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/PigeonGame/BasePigeon.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM PigeonGame/BasePigeon.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/PigeonGame/BasePigeon.d

# Compiles file PigeonGame/GameRules_Pigeon.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/PigeonGame/GameRules_Pigeon.d
gccSTDGameRelease/PigeonGame/GameRules_Pigeon.o: PigeonGame/GameRules_Pigeon.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c PigeonGame/GameRules_Pigeon.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/PigeonGame/GameRules_Pigeon.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM PigeonGame/GameRules_Pigeon.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/PigeonGame/GameRules_Pigeon.d

# Compiles file PigeonGame/Pigeon_Player.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/PigeonGame/Pigeon_Player.d
gccSTDGameRelease/PigeonGame/Pigeon_Player.o: PigeonGame/Pigeon_Player.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c PigeonGame/Pigeon_Player.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/PigeonGame/Pigeon_Player.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM PigeonGame/Pigeon_Player.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/PigeonGame/Pigeon_Player.d

# Compiles file GameRules_WhiteCage.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/GameRules_WhiteCage.d
gccSTDGameRelease/GameRules_WhiteCage.o: GameRules_WhiteCage.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c GameRules_WhiteCage.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/GameRules_WhiteCage.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM GameRules_WhiteCage.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/GameRules_WhiteCage.d

# Compiles file npc_cyborg.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/npc_cyborg.d
gccSTDGameRelease/npc_cyborg.o: npc_cyborg.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c npc_cyborg.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/npc_cyborg.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM npc_cyborg.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/npc_cyborg.d

# Compiles file npc_soldier.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/npc_soldier.d
gccSTDGameRelease/npc_soldier.o: npc_soldier.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c npc_soldier.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/npc_soldier.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM npc_soldier.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/npc_soldier.d

# Compiles file Player.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/Player.d
gccSTDGameRelease/Player.o: Player.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c Player.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/Player.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM Player.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/Player.d

# Compiles file viewmodel.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/viewmodel.d
gccSTDGameRelease/viewmodel.o: viewmodel.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c viewmodel.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/viewmodel.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM viewmodel.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/viewmodel.d

# Compiles file weapon_ak74.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/weapon_ak74.d
gccSTDGameRelease/weapon_ak74.o: weapon_ak74.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c weapon_ak74.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/weapon_ak74.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM weapon_ak74.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/weapon_ak74.d

# Compiles file weapon_deagle.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/weapon_deagle.d
gccSTDGameRelease/weapon_deagle.o: weapon_deagle.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c weapon_deagle.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/weapon_deagle.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM weapon_deagle.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/weapon_deagle.d

# Compiles file weapon_f1.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/weapon_f1.d
gccSTDGameRelease/weapon_f1.o: weapon_f1.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c weapon_f1.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/weapon_f1.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM weapon_f1.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/weapon_f1.d

# Compiles file weapon_wiremine.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/weapon_wiremine.d
gccSTDGameRelease/weapon_wiremine.o: weapon_wiremine.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c weapon_wiremine.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/weapon_wiremine.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM weapon_wiremine.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/weapon_wiremine.d

# Compiles file EGUI/EQUI_Manager.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EGUI/EQUI_Manager.d
gccSTDGameRelease/EGUI/EQUI_Manager.o: EGUI/EQUI_Manager.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EGUI/EQUI_Manager.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EGUI/EQUI_Manager.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EGUI/EQUI_Manager.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EGUI/EQUI_Manager.d

# Compiles file EGUI/EqUI_Panel.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EGUI/EqUI_Panel.d
gccSTDGameRelease/EGUI/EqUI_Panel.o: EGUI/EqUI_Panel.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EGUI/EqUI_Panel.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EGUI/EqUI_Panel.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EGUI/EqUI_Panel.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EGUI/EqUI_Panel.d

# Compiles file EntityBind.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EntityBind.d
gccSTDGameRelease/EntityBind.o: EntityBind.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EntityBind.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EntityBind.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EntityBind.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EntityBind.d

# Compiles file EqGMS.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EqGMS.d
gccSTDGameRelease/EqGMS.o: EqGMS.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EqGMS.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EqGMS.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EqGMS.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EqGMS.d

# Compiles file EngineScriptBind.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/EngineScriptBind.d
gccSTDGameRelease/EngineScriptBind.o: EngineScriptBind.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c EngineScriptBind.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/EngineScriptBind.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM EngineScriptBind.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/EngineScriptBind.d

# Compiles file gmScript/gm/gmArraySimple.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmArraySimple.d
gccSTDGameRelease/gmScript/gm/gmArraySimple.o: gmScript/gm/gmArraySimple.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmArraySimple.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmArraySimple.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmArraySimple.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmArraySimple.d

# Compiles file gmScript/gm/gmByteCode.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmByteCode.d
gccSTDGameRelease/gmScript/gm/gmByteCode.o: gmScript/gm/gmByteCode.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmByteCode.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmByteCode.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmByteCode.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmByteCode.d

# Compiles file gmScript/gm/gmByteCodeGen.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmByteCodeGen.d
gccSTDGameRelease/gmScript/gm/gmByteCodeGen.o: gmScript/gm/gmByteCodeGen.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmByteCodeGen.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmByteCodeGen.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmByteCodeGen.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmByteCodeGen.d

# Compiles file gmScript/gm/gmCodeGen.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmCodeGen.d
gccSTDGameRelease/gmScript/gm/gmCodeGen.o: gmScript/gm/gmCodeGen.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmCodeGen.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmCodeGen.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmCodeGen.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmCodeGen.d

# Compiles file gmScript/gm/gmCodeGenHooks.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmCodeGenHooks.d
gccSTDGameRelease/gmScript/gm/gmCodeGenHooks.o: gmScript/gm/gmCodeGenHooks.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmCodeGenHooks.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmCodeGenHooks.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmCodeGenHooks.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmCodeGenHooks.d

# Compiles file gmScript/gm/gmCodeTree.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmCodeTree.d
gccSTDGameRelease/gmScript/gm/gmCodeTree.o: gmScript/gm/gmCodeTree.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmCodeTree.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmCodeTree.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmCodeTree.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmCodeTree.d

# Compiles file gmScript/gm/gmCrc.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmCrc.d
gccSTDGameRelease/gmScript/gm/gmCrc.o: gmScript/gm/gmCrc.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmCrc.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmCrc.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmCrc.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmCrc.d

# Compiles file gmScript/gm/gmDebug.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmDebug.d
gccSTDGameRelease/gmScript/gm/gmDebug.o: gmScript/gm/gmDebug.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmDebug.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmDebug.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmDebug.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmDebug.d

# Compiles file gmScript/gm/gmFunctionObject.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmFunctionObject.d
gccSTDGameRelease/gmScript/gm/gmFunctionObject.o: gmScript/gm/gmFunctionObject.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmFunctionObject.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmFunctionObject.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmFunctionObject.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmFunctionObject.d

# Compiles file gmScript/gm/gmHash.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmHash.d
gccSTDGameRelease/gmScript/gm/gmHash.o: gmScript/gm/gmHash.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmHash.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmHash.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmHash.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmHash.d

# Compiles file gmScript/gm/gmIncGC.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmIncGC.d
gccSTDGameRelease/gmScript/gm/gmIncGC.o: gmScript/gm/gmIncGC.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmIncGC.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmIncGC.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmIncGC.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmIncGC.d

# Compiles file gmScript/gm/gmLibHooks.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmLibHooks.d
gccSTDGameRelease/gmScript/gm/gmLibHooks.o: gmScript/gm/gmLibHooks.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmLibHooks.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmLibHooks.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmLibHooks.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmLibHooks.d

# Compiles file gmScript/gm/gmListDouble.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmListDouble.d
gccSTDGameRelease/gmScript/gm/gmListDouble.o: gmScript/gm/gmListDouble.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmListDouble.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmListDouble.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmListDouble.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmListDouble.d

# Compiles file gmScript/gm/gmLog.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmLog.d
gccSTDGameRelease/gmScript/gm/gmLog.o: gmScript/gm/gmLog.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmLog.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmLog.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmLog.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmLog.d

# Compiles file gmScript/gm/gmMachine.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmMachine.d
gccSTDGameRelease/gmScript/gm/gmMachine.o: gmScript/gm/gmMachine.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmMachine.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmMachine.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmMachine.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmMachine.d

# Compiles file gmScript/gm/gmMachineLib.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmMachineLib.d
gccSTDGameRelease/gmScript/gm/gmMachineLib.o: gmScript/gm/gmMachineLib.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmMachineLib.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmMachineLib.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmMachineLib.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmMachineLib.d

# Compiles file gmScript/gm/gmMem.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmMem.d
gccSTDGameRelease/gmScript/gm/gmMem.o: gmScript/gm/gmMem.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmMem.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmMem.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmMem.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmMem.d

# Compiles file gmScript/gm/gmMemChain.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmMemChain.d
gccSTDGameRelease/gmScript/gm/gmMemChain.o: gmScript/gm/gmMemChain.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmMemChain.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmMemChain.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmMemChain.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmMemChain.d

# Compiles file gmScript/gm/gmMemFixed.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmMemFixed.d
gccSTDGameRelease/gmScript/gm/gmMemFixed.o: gmScript/gm/gmMemFixed.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmMemFixed.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmMemFixed.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmMemFixed.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmMemFixed.d

# Compiles file gmScript/gm/gmMemFixedSet.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmMemFixedSet.d
gccSTDGameRelease/gmScript/gm/gmMemFixedSet.o: gmScript/gm/gmMemFixedSet.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmMemFixedSet.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmMemFixedSet.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmMemFixedSet.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmMemFixedSet.d

# Compiles file gmScript/gm/gmOperators.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmOperators.d
gccSTDGameRelease/gmScript/gm/gmOperators.o: gmScript/gm/gmOperators.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmOperators.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmOperators.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmOperators.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmOperators.d

# Compiles file gmScript/gm/gmParser.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmParser.d
gccSTDGameRelease/gmScript/gm/gmParser.o: gmScript/gm/gmParser.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmParser.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmParser.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmParser.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmParser.d

# Compiles file gmScript/gm/gmScanner.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmScanner.d
gccSTDGameRelease/gmScript/gm/gmScanner.o: gmScript/gm/gmScanner.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmScanner.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmScanner.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmScanner.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmScanner.d

# Compiles file gmScript/gm/gmStream.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmStream.d
gccSTDGameRelease/gmScript/gm/gmStream.o: gmScript/gm/gmStream.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmStream.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmStream.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmStream.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmStream.d

# Compiles file gmScript/gm/gmStreamBuffer.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmStreamBuffer.d
gccSTDGameRelease/gmScript/gm/gmStreamBuffer.o: gmScript/gm/gmStreamBuffer.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmStreamBuffer.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmStreamBuffer.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmStreamBuffer.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmStreamBuffer.d

# Compiles file gmScript/gm/gmStringObject.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmStringObject.d
gccSTDGameRelease/gmScript/gm/gmStringObject.o: gmScript/gm/gmStringObject.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmStringObject.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmStringObject.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmStringObject.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmStringObject.d

# Compiles file gmScript/gm/gmTableObject.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmTableObject.d
gccSTDGameRelease/gmScript/gm/gmTableObject.o: gmScript/gm/gmTableObject.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmTableObject.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmTableObject.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmTableObject.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmTableObject.d

# Compiles file gmScript/gm/gmThread.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmThread.d
gccSTDGameRelease/gmScript/gm/gmThread.o: gmScript/gm/gmThread.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmThread.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmThread.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmThread.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmThread.d

# Compiles file gmScript/gm/gmUserObject.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmUserObject.d
gccSTDGameRelease/gmScript/gm/gmUserObject.o: gmScript/gm/gmUserObject.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmUserObject.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmUserObject.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmUserObject.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmUserObject.d

# Compiles file gmScript/gm/gmUtil.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmUtil.d
gccSTDGameRelease/gmScript/gm/gmUtil.o: gmScript/gm/gmUtil.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmUtil.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmUtil.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmUtil.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmUtil.d

# Compiles file gmScript/gm/gmVariable.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/gm/gmVariable.d
gccSTDGameRelease/gmScript/gm/gmVariable.o: gmScript/gm/gmVariable.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/gm/gmVariable.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/gm/gmVariable.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/gm/gmVariable.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/gm/gmVariable.d

# Compiles file gmScript/binds/gmArrayLib.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmArrayLib.d
gccSTDGameRelease/gmScript/binds/gmArrayLib.o: gmScript/binds/gmArrayLib.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmArrayLib.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmArrayLib.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmArrayLib.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmArrayLib.d

# Compiles file gmScript/binds/gmCall.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmCall.d
gccSTDGameRelease/gmScript/binds/gmCall.o: gmScript/binds/gmCall.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmCall.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmCall.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmCall.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmCall.d

# Compiles file gmScript/binds/gmGCRoot.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmGCRoot.d
gccSTDGameRelease/gmScript/binds/gmGCRoot.o: gmScript/binds/gmGCRoot.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmGCRoot.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmGCRoot.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmGCRoot.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmGCRoot.d

# Compiles file gmScript/binds/gmGCRootUtil.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmGCRootUtil.d
gccSTDGameRelease/gmScript/binds/gmGCRootUtil.o: gmScript/binds/gmGCRootUtil.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmGCRootUtil.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmGCRootUtil.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmGCRootUtil.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmGCRootUtil.d

# Compiles file gmScript/binds/gmHelpers.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmHelpers.d
gccSTDGameRelease/gmScript/binds/gmHelpers.o: gmScript/binds/gmHelpers.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmHelpers.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmHelpers.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmHelpers.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmHelpers.d

# Compiles file gmScript/binds/gmMathLib.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmMathLib.d
gccSTDGameRelease/gmScript/binds/gmMathLib.o: gmScript/binds/gmMathLib.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmMathLib.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmMathLib.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmMathLib.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmMathLib.d

# Compiles file gmScript/binds/gmStringLib.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmStringLib.d
gccSTDGameRelease/gmScript/binds/gmStringLib.o: gmScript/binds/gmStringLib.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmStringLib.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmStringLib.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmStringLib.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmStringLib.d

# Compiles file gmScript/binds/gmSystemLib.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmSystemLib.d
gccSTDGameRelease/gmScript/binds/gmSystemLib.o: gmScript/binds/gmSystemLib.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmSystemLib.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmSystemLib.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmSystemLib.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmSystemLib.d

# Compiles file gmScript/binds/gmVector3Lib.cpp for the STDGameRelease configuration...
-include gccSTDGameRelease/gmScript/binds/gmVector3Lib.d
gccSTDGameRelease/gmScript/binds/gmVector3Lib.o: gmScript/binds/gmVector3Lib.cpp
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -c gmScript/binds/gmVector3Lib.cpp $(STDGameRelease_Include_Path) -o gccSTDGameRelease/gmScript/binds/gmVector3Lib.o
	$(CPP_COMPILER) $(STDGameRelease_Preprocessor_Definitions) $(STDGameRelease_Compiler_Flags) -MM gmScript/binds/gmVector3Lib.cpp $(STDGameRelease_Include_Path) > gccSTDGameRelease/gmScript/binds/gmVector3Lib.d

# Creates the intermediate and output folders for each configuration...
.PHONY: create_folders
create_folders:
	mkdir -p gccDebug/source
	mkdir -p gccPigeonGameRelease/source
	mkdir -p gccRelease/source
	mkdir -p gccSTDGameRelease/source

# Cleans intermediate and output files (objects, libraries, executables)...
.PHONY: clean
clean:
	rm -f gccDebug/*.o
	rm -f gccDebug/*.d
	rm -f gccDebug/*.a
	rm -f gccDebug/*.so
	rm -f gccDebug/*.dll
	rm -f gccDebug/*.exe
	rm -f gccPigeonGameRelease/*.o
	rm -f gccPigeonGameRelease/*.d
	rm -f gccPigeonGameRelease/*.a
	rm -f gccPigeonGameRelease/*.so
	rm -f gccPigeonGameRelease/*.dll
	rm -f gccPigeonGameRelease/*.exe
	rm -f gccRelease/*.o
	rm -f gccRelease/*.d
	rm -f gccRelease/*.a
	rm -f gccRelease/*.so
	rm -f gccRelease/*.dll
	rm -f gccRelease/*.exe
	rm -f gccSTDGameRelease/*.o
	rm -f gccSTDGameRelease/*.d
	rm -f gccSTDGameRelease/*.a
	rm -f gccSTDGameRelease/*.so
	rm -f gccSTDGameRelease/*.dll
	rm -f gccSTDGameRelease/*.exe

