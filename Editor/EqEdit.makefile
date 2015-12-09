# Compiler flags...
CPP_COMPILER = g++
C_COMPILER = gcc

# Include paths...
Debug_Include_Path=-I"../Game" -I"../FileSystem" -I"../Engine" -I"../public" -I"../public/platform" -I"../Physics/bullet/src" -I"../Physics/bullet/src/BulletSoftBody" -I"../utils/eq_physgen" -I"../utils/wx/include" -I"../utils/wx/include/msvc" 
Release_Include_Path=-I"../Game" -I"../FileSystem" -I"../Engine" -I"../public" -I"../public/platform" -I"../src_dependency/bullet/src" -I"../src_dependency/bullet/src/BulletSoftBody" -I"../utils/eq_physgen" -I"../src_dependency/wx/include" -I"../src_dependency/wx/include/msvc" -I"../src_dependency/sdl2/include" 

# Library paths...
Debug_Library_Path=-L"../utils/wx/lib/gccvc_lib" 
Release_Library_Path=-L"../src_dependency/wx/lib/gccvc_lib" 

# Additional libraries...
Debug_Libraries=-Wl,--start-group -luser32 -lcomctl32 -lwinmm  -Wl,--end-group
Release_Libraries=-Wl,--start-group -luser32 -lcomctl32 -lwinmm  -Wl,--end-group

# Preprocessor definitions...
Debug_Preprocessor_Definitions=-D GCC_BUILD -D _DEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D EDITOR -D NO_ENGINE -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 
Release_Preprocessor_Definitions=-D GCC_BUILD -D NDEBUG -D _WINDOWS -D _USRDLL -D ENGINE_EXPORT -D PHYSICS_EXPORT -D EDITOR -D NO_ENGINE -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE 

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
Debug: create_folders gccDebug/main.o gccDebug/../public/ConCommand.o gccDebug/../public/ConCommandBase.o gccDebug/../public/ConVar.o gccDebug/../public/Utils/GeomTools.o gccDebug/../public/Math/math_util.o gccDebug/../Engine/studio_egf.o gccDebug/../Engine/modelloader_shared.o gccDebug/MaterialListFrame.o gccDebug/RenderWindows.o gccDebug/EditorMainFrame.o gccDebug/EditorLevel.o gccDebug/EntityDef.o gccDebug/UndoObserver.o gccDebug/wxFourWaySplitter.o gccDebug/../public/anim_activity.o gccDebug/../public/anim_events.o gccDebug/../public/BoneSetup.o gccDebug/EditableDecal.o gccDebug/EditableEntityParameters.o gccDebug/GroupEditable.o gccDebug/EditableEntity.o gccDebug/EditableSurface.o gccDebug/BaseEditableObject.o gccDebug/EqBrush.o gccDebug/BrushWinding.o gccDebug/../public/dsm_esm_loader.o gccDebug/../public/dsm_loader.o gccDebug/../public/dsm_obj_loader.o gccDebug/../public/mtriangle_framework.o gccDebug/BlockTool.o gccDebug/DecalTool.o gccDebug/ClipperTool.o gccDebug/EntityTool.o gccDebug/SelectionEditor.o gccDebug/TerrainPatchTool.o gccDebug/VertexTool.o gccDebug/EntityList.o gccDebug/LayerPanel.o gccDebug/BuildOptionsDialog.o gccDebug/EntityProperties.o gccDebug/LoadLevelDialog.o gccDebug/OptionsDialog.o gccDebug/SoundList.o gccDebug/SurfaceDialog.o gccDebug/TransformSelectionDialog.o gccDebug/../Engine/ViewRenderer.o gccDebug/../public/cmd_pacifier.o gccDebug/../public/Font.o gccDebug/DCRender2D.o gccDebug/EditorCamera.o gccDebug/grid.o gccDebug/../public/ViewParams.o gccDebug/../public/RenderList.o gccDebug/../public/BaseRenderableObject.o gccDebug/../Engine/DebugOverlay.o gccDebug/../public/BaseShader.o gccDebug/EditorFlatColor.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/../Engine/Audio/alsnd_ambient.o gccDebug/../Engine/Audio/alsnd_emitter.o gccDebug/../Engine/Audio/alsnd_sample.o gccDebug/../Engine/Audio/alsound_local.o gccDebug/../Engine/Audio/soundzero.o 
	g++ -fPIC -shared -Wl,-soname,libEqEdit.so -o gccDebug/libEqEdit.so gccDebug/main.o gccDebug/../public/ConCommand.o gccDebug/../public/ConCommandBase.o gccDebug/../public/ConVar.o gccDebug/../public/Utils/GeomTools.o gccDebug/../public/Math/math_util.o gccDebug/../Engine/studio_egf.o gccDebug/../Engine/modelloader_shared.o gccDebug/MaterialListFrame.o gccDebug/RenderWindows.o gccDebug/EditorMainFrame.o gccDebug/EditorLevel.o gccDebug/EntityDef.o gccDebug/UndoObserver.o gccDebug/wxFourWaySplitter.o gccDebug/../public/anim_activity.o gccDebug/../public/anim_events.o gccDebug/../public/BoneSetup.o gccDebug/EditableDecal.o gccDebug/EditableEntityParameters.o gccDebug/GroupEditable.o gccDebug/EditableEntity.o gccDebug/EditableSurface.o gccDebug/BaseEditableObject.o gccDebug/EqBrush.o gccDebug/BrushWinding.o gccDebug/../public/dsm_esm_loader.o gccDebug/../public/dsm_loader.o gccDebug/../public/dsm_obj_loader.o gccDebug/../public/mtriangle_framework.o gccDebug/BlockTool.o gccDebug/DecalTool.o gccDebug/ClipperTool.o gccDebug/EntityTool.o gccDebug/SelectionEditor.o gccDebug/TerrainPatchTool.o gccDebug/VertexTool.o gccDebug/EntityList.o gccDebug/LayerPanel.o gccDebug/BuildOptionsDialog.o gccDebug/EntityProperties.o gccDebug/LoadLevelDialog.o gccDebug/OptionsDialog.o gccDebug/SoundList.o gccDebug/SurfaceDialog.o gccDebug/TransformSelectionDialog.o gccDebug/../Engine/ViewRenderer.o gccDebug/../public/cmd_pacifier.o gccDebug/../public/Font.o gccDebug/DCRender2D.o gccDebug/EditorCamera.o gccDebug/grid.o gccDebug/../public/ViewParams.o gccDebug/../public/RenderList.o gccDebug/../public/BaseRenderableObject.o gccDebug/../Engine/DebugOverlay.o gccDebug/../public/BaseShader.o gccDebug/EditorFlatColor.o gccDebug/../public/GameSoundEmitterSystem.o gccDebug/../Engine/Audio/alsnd_ambient.o gccDebug/../Engine/Audio/alsnd_emitter.o gccDebug/../Engine/Audio/alsnd_sample.o gccDebug/../Engine/Audio/alsound_local.o gccDebug/../Engine/Audio/soundzero.o  $(Debug_Implicitly_Linked_Objects)

# Compiles file main.cpp for the Debug configuration...
-include gccDebug/main.d
gccDebug/main.o: main.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c main.cpp $(Debug_Include_Path) -o gccDebug/main.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM main.cpp $(Debug_Include_Path) > gccDebug/main.d

# Compiles file ../public/ConCommand.cpp for the Debug configuration...
-include gccDebug/../public/ConCommand.d
gccDebug/../public/ConCommand.o: ../public/ConCommand.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ConCommand.cpp $(Debug_Include_Path) -o gccDebug/../public/ConCommand.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ConCommand.cpp $(Debug_Include_Path) > gccDebug/../public/ConCommand.d

# Compiles file ../public/ConCommandBase.cpp for the Debug configuration...
-include gccDebug/../public/ConCommandBase.d
gccDebug/../public/ConCommandBase.o: ../public/ConCommandBase.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ConCommandBase.cpp $(Debug_Include_Path) -o gccDebug/../public/ConCommandBase.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ConCommandBase.cpp $(Debug_Include_Path) > gccDebug/../public/ConCommandBase.d

# Compiles file ../public/ConVar.cpp for the Debug configuration...
-include gccDebug/../public/ConVar.d
gccDebug/../public/ConVar.o: ../public/ConVar.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ConVar.cpp $(Debug_Include_Path) -o gccDebug/../public/ConVar.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ConVar.cpp $(Debug_Include_Path) > gccDebug/../public/ConVar.d

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

# Compiles file MaterialListFrame.cpp for the Debug configuration...
-include gccDebug/MaterialListFrame.d
gccDebug/MaterialListFrame.o: MaterialListFrame.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c MaterialListFrame.cpp $(Debug_Include_Path) -o gccDebug/MaterialListFrame.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM MaterialListFrame.cpp $(Debug_Include_Path) > gccDebug/MaterialListFrame.d

# Compiles file RenderWindows.cpp for the Debug configuration...
-include gccDebug/RenderWindows.d
gccDebug/RenderWindows.o: RenderWindows.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c RenderWindows.cpp $(Debug_Include_Path) -o gccDebug/RenderWindows.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM RenderWindows.cpp $(Debug_Include_Path) > gccDebug/RenderWindows.d

# Compiles file EditorMainFrame.cpp for the Debug configuration...
-include gccDebug/EditorMainFrame.d
gccDebug/EditorMainFrame.o: EditorMainFrame.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditorMainFrame.cpp $(Debug_Include_Path) -o gccDebug/EditorMainFrame.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditorMainFrame.cpp $(Debug_Include_Path) > gccDebug/EditorMainFrame.d

# Compiles file EditorLevel.cpp for the Debug configuration...
-include gccDebug/EditorLevel.d
gccDebug/EditorLevel.o: EditorLevel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditorLevel.cpp $(Debug_Include_Path) -o gccDebug/EditorLevel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditorLevel.cpp $(Debug_Include_Path) > gccDebug/EditorLevel.d

# Compiles file EntityDef.cpp for the Debug configuration...
-include gccDebug/EntityDef.d
gccDebug/EntityDef.o: EntityDef.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityDef.cpp $(Debug_Include_Path) -o gccDebug/EntityDef.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityDef.cpp $(Debug_Include_Path) > gccDebug/EntityDef.d

# Compiles file UndoObserver.cpp for the Debug configuration...
-include gccDebug/UndoObserver.d
gccDebug/UndoObserver.o: UndoObserver.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c UndoObserver.cpp $(Debug_Include_Path) -o gccDebug/UndoObserver.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM UndoObserver.cpp $(Debug_Include_Path) > gccDebug/UndoObserver.d

# Compiles file wxFourWaySplitter.cpp for the Debug configuration...
-include gccDebug/wxFourWaySplitter.d
gccDebug/wxFourWaySplitter.o: wxFourWaySplitter.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c wxFourWaySplitter.cpp $(Debug_Include_Path) -o gccDebug/wxFourWaySplitter.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM wxFourWaySplitter.cpp $(Debug_Include_Path) > gccDebug/wxFourWaySplitter.d

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

# Compiles file EditableDecal.cpp for the Debug configuration...
-include gccDebug/EditableDecal.d
gccDebug/EditableDecal.o: EditableDecal.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditableDecal.cpp $(Debug_Include_Path) -o gccDebug/EditableDecal.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditableDecal.cpp $(Debug_Include_Path) > gccDebug/EditableDecal.d

# Compiles file EditableEntityParameters.cpp for the Debug configuration...
-include gccDebug/EditableEntityParameters.d
gccDebug/EditableEntityParameters.o: EditableEntityParameters.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditableEntityParameters.cpp $(Debug_Include_Path) -o gccDebug/EditableEntityParameters.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditableEntityParameters.cpp $(Debug_Include_Path) > gccDebug/EditableEntityParameters.d

# Compiles file GroupEditable.cpp for the Debug configuration...
-include gccDebug/GroupEditable.d
gccDebug/GroupEditable.o: GroupEditable.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c GroupEditable.cpp $(Debug_Include_Path) -o gccDebug/GroupEditable.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM GroupEditable.cpp $(Debug_Include_Path) > gccDebug/GroupEditable.d

# Compiles file EditableEntity.cpp for the Debug configuration...
-include gccDebug/EditableEntity.d
gccDebug/EditableEntity.o: EditableEntity.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditableEntity.cpp $(Debug_Include_Path) -o gccDebug/EditableEntity.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditableEntity.cpp $(Debug_Include_Path) > gccDebug/EditableEntity.d

# Compiles file EditableSurface.cpp for the Debug configuration...
-include gccDebug/EditableSurface.d
gccDebug/EditableSurface.o: EditableSurface.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditableSurface.cpp $(Debug_Include_Path) -o gccDebug/EditableSurface.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditableSurface.cpp $(Debug_Include_Path) > gccDebug/EditableSurface.d

# Compiles file BaseEditableObject.cpp for the Debug configuration...
-include gccDebug/BaseEditableObject.d
gccDebug/BaseEditableObject.o: BaseEditableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BaseEditableObject.cpp $(Debug_Include_Path) -o gccDebug/BaseEditableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BaseEditableObject.cpp $(Debug_Include_Path) > gccDebug/BaseEditableObject.d

# Compiles file EqBrush.cpp for the Debug configuration...
-include gccDebug/EqBrush.d
gccDebug/EqBrush.o: EqBrush.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EqBrush.cpp $(Debug_Include_Path) -o gccDebug/EqBrush.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EqBrush.cpp $(Debug_Include_Path) > gccDebug/EqBrush.d

# Compiles file BrushWinding.cpp for the Debug configuration...
-include gccDebug/BrushWinding.d
gccDebug/BrushWinding.o: BrushWinding.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BrushWinding.cpp $(Debug_Include_Path) -o gccDebug/BrushWinding.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BrushWinding.cpp $(Debug_Include_Path) > gccDebug/BrushWinding.d

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

# Compiles file ../public/mtriangle_framework.cpp for the Debug configuration...
-include gccDebug/../public/mtriangle_framework.d
gccDebug/../public/mtriangle_framework.o: ../public/mtriangle_framework.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/mtriangle_framework.cpp $(Debug_Include_Path) -o gccDebug/../public/mtriangle_framework.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/mtriangle_framework.cpp $(Debug_Include_Path) > gccDebug/../public/mtriangle_framework.d

# Compiles file BlockTool.cpp for the Debug configuration...
-include gccDebug/BlockTool.d
gccDebug/BlockTool.o: BlockTool.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BlockTool.cpp $(Debug_Include_Path) -o gccDebug/BlockTool.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BlockTool.cpp $(Debug_Include_Path) > gccDebug/BlockTool.d

# Compiles file DecalTool.cpp for the Debug configuration...
-include gccDebug/DecalTool.d
gccDebug/DecalTool.o: DecalTool.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DecalTool.cpp $(Debug_Include_Path) -o gccDebug/DecalTool.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DecalTool.cpp $(Debug_Include_Path) > gccDebug/DecalTool.d

# Compiles file ClipperTool.cpp for the Debug configuration...
-include gccDebug/ClipperTool.d
gccDebug/ClipperTool.o: ClipperTool.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ClipperTool.cpp $(Debug_Include_Path) -o gccDebug/ClipperTool.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ClipperTool.cpp $(Debug_Include_Path) > gccDebug/ClipperTool.d

# Compiles file EntityTool.cpp for the Debug configuration...
-include gccDebug/EntityTool.d
gccDebug/EntityTool.o: EntityTool.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityTool.cpp $(Debug_Include_Path) -o gccDebug/EntityTool.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityTool.cpp $(Debug_Include_Path) > gccDebug/EntityTool.d

# Compiles file SelectionEditor.cpp for the Debug configuration...
-include gccDebug/SelectionEditor.d
gccDebug/SelectionEditor.o: SelectionEditor.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SelectionEditor.cpp $(Debug_Include_Path) -o gccDebug/SelectionEditor.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SelectionEditor.cpp $(Debug_Include_Path) > gccDebug/SelectionEditor.d

# Compiles file TerrainPatchTool.cpp for the Debug configuration...
-include gccDebug/TerrainPatchTool.d
gccDebug/TerrainPatchTool.o: TerrainPatchTool.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c TerrainPatchTool.cpp $(Debug_Include_Path) -o gccDebug/TerrainPatchTool.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM TerrainPatchTool.cpp $(Debug_Include_Path) > gccDebug/TerrainPatchTool.d

# Compiles file VertexTool.cpp for the Debug configuration...
-include gccDebug/VertexTool.d
gccDebug/VertexTool.o: VertexTool.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c VertexTool.cpp $(Debug_Include_Path) -o gccDebug/VertexTool.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM VertexTool.cpp $(Debug_Include_Path) > gccDebug/VertexTool.d

# Compiles file EntityList.cpp for the Debug configuration...
-include gccDebug/EntityList.d
gccDebug/EntityList.o: EntityList.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityList.cpp $(Debug_Include_Path) -o gccDebug/EntityList.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityList.cpp $(Debug_Include_Path) > gccDebug/EntityList.d

# Compiles file LayerPanel.cpp for the Debug configuration...
-include gccDebug/LayerPanel.d
gccDebug/LayerPanel.o: LayerPanel.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c LayerPanel.cpp $(Debug_Include_Path) -o gccDebug/LayerPanel.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM LayerPanel.cpp $(Debug_Include_Path) > gccDebug/LayerPanel.d

# Compiles file BuildOptionsDialog.cpp for the Debug configuration...
-include gccDebug/BuildOptionsDialog.d
gccDebug/BuildOptionsDialog.o: BuildOptionsDialog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c BuildOptionsDialog.cpp $(Debug_Include_Path) -o gccDebug/BuildOptionsDialog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM BuildOptionsDialog.cpp $(Debug_Include_Path) > gccDebug/BuildOptionsDialog.d

# Compiles file EntityProperties.cpp for the Debug configuration...
-include gccDebug/EntityProperties.d
gccDebug/EntityProperties.o: EntityProperties.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EntityProperties.cpp $(Debug_Include_Path) -o gccDebug/EntityProperties.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EntityProperties.cpp $(Debug_Include_Path) > gccDebug/EntityProperties.d

# Compiles file LoadLevelDialog.cpp for the Debug configuration...
-include gccDebug/LoadLevelDialog.d
gccDebug/LoadLevelDialog.o: LoadLevelDialog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c LoadLevelDialog.cpp $(Debug_Include_Path) -o gccDebug/LoadLevelDialog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM LoadLevelDialog.cpp $(Debug_Include_Path) > gccDebug/LoadLevelDialog.d

# Compiles file OptionsDialog.cpp for the Debug configuration...
-include gccDebug/OptionsDialog.d
gccDebug/OptionsDialog.o: OptionsDialog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c OptionsDialog.cpp $(Debug_Include_Path) -o gccDebug/OptionsDialog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM OptionsDialog.cpp $(Debug_Include_Path) > gccDebug/OptionsDialog.d

# Compiles file SoundList.cpp for the Debug configuration...
-include gccDebug/SoundList.d
gccDebug/SoundList.o: SoundList.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SoundList.cpp $(Debug_Include_Path) -o gccDebug/SoundList.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SoundList.cpp $(Debug_Include_Path) > gccDebug/SoundList.d

# Compiles file SurfaceDialog.cpp for the Debug configuration...
-include gccDebug/SurfaceDialog.d
gccDebug/SurfaceDialog.o: SurfaceDialog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c SurfaceDialog.cpp $(Debug_Include_Path) -o gccDebug/SurfaceDialog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM SurfaceDialog.cpp $(Debug_Include_Path) > gccDebug/SurfaceDialog.d

# Compiles file TransformSelectionDialog.cpp for the Debug configuration...
-include gccDebug/TransformSelectionDialog.d
gccDebug/TransformSelectionDialog.o: TransformSelectionDialog.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c TransformSelectionDialog.cpp $(Debug_Include_Path) -o gccDebug/TransformSelectionDialog.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM TransformSelectionDialog.cpp $(Debug_Include_Path) > gccDebug/TransformSelectionDialog.d

# Compiles file ../Engine/ViewRenderer.cpp for the Debug configuration...
-include gccDebug/../Engine/ViewRenderer.d
gccDebug/../Engine/ViewRenderer.o: ../Engine/ViewRenderer.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/ViewRenderer.cpp $(Debug_Include_Path) -o gccDebug/../Engine/ViewRenderer.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/ViewRenderer.cpp $(Debug_Include_Path) > gccDebug/../Engine/ViewRenderer.d

# Compiles file ../public/cmd_pacifier.cpp for the Debug configuration...
-include gccDebug/../public/cmd_pacifier.d
gccDebug/../public/cmd_pacifier.o: ../public/cmd_pacifier.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/cmd_pacifier.cpp $(Debug_Include_Path) -o gccDebug/../public/cmd_pacifier.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/cmd_pacifier.cpp $(Debug_Include_Path) > gccDebug/../public/cmd_pacifier.d

# Compiles file ../public/Font.cpp for the Debug configuration...
-include gccDebug/../public/Font.d
gccDebug/../public/Font.o: ../public/Font.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/Font.cpp $(Debug_Include_Path) -o gccDebug/../public/Font.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/Font.cpp $(Debug_Include_Path) > gccDebug/../public/Font.d

# Compiles file DCRender2D.cpp for the Debug configuration...
-include gccDebug/DCRender2D.d
gccDebug/DCRender2D.o: DCRender2D.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c DCRender2D.cpp $(Debug_Include_Path) -o gccDebug/DCRender2D.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM DCRender2D.cpp $(Debug_Include_Path) > gccDebug/DCRender2D.d

# Compiles file EditorCamera.cpp for the Debug configuration...
-include gccDebug/EditorCamera.d
gccDebug/EditorCamera.o: EditorCamera.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditorCamera.cpp $(Debug_Include_Path) -o gccDebug/EditorCamera.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditorCamera.cpp $(Debug_Include_Path) > gccDebug/EditorCamera.d

# Compiles file grid.cpp for the Debug configuration...
-include gccDebug/grid.d
gccDebug/grid.o: grid.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c grid.cpp $(Debug_Include_Path) -o gccDebug/grid.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM grid.cpp $(Debug_Include_Path) > gccDebug/grid.d

# Compiles file ../public/ViewParams.cpp for the Debug configuration...
-include gccDebug/../public/ViewParams.d
gccDebug/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/ViewParams.cpp $(Debug_Include_Path) -o gccDebug/../public/ViewParams.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/ViewParams.cpp $(Debug_Include_Path) > gccDebug/../public/ViewParams.d

# Compiles file ../public/RenderList.cpp for the Debug configuration...
-include gccDebug/../public/RenderList.d
gccDebug/../public/RenderList.o: ../public/RenderList.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/RenderList.cpp $(Debug_Include_Path) -o gccDebug/../public/RenderList.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/RenderList.cpp $(Debug_Include_Path) > gccDebug/../public/RenderList.d

# Compiles file ../public/BaseRenderableObject.cpp for the Debug configuration...
-include gccDebug/../public/BaseRenderableObject.d
gccDebug/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(Debug_Include_Path) -o gccDebug/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(Debug_Include_Path) > gccDebug/../public/BaseRenderableObject.d

# Compiles file ../Engine/DebugOverlay.cpp for the Debug configuration...
-include gccDebug/../Engine/DebugOverlay.d
gccDebug/../Engine/DebugOverlay.o: ../Engine/DebugOverlay.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../Engine/DebugOverlay.cpp $(Debug_Include_Path) -o gccDebug/../Engine/DebugOverlay.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../Engine/DebugOverlay.cpp $(Debug_Include_Path) > gccDebug/../Engine/DebugOverlay.d

# Compiles file ../public/BaseShader.cpp for the Debug configuration...
-include gccDebug/../public/BaseShader.d
gccDebug/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/BaseShader.cpp $(Debug_Include_Path) -o gccDebug/../public/BaseShader.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/BaseShader.cpp $(Debug_Include_Path) > gccDebug/../public/BaseShader.d

# Compiles file EditorFlatColor.cpp for the Debug configuration...
-include gccDebug/EditorFlatColor.d
gccDebug/EditorFlatColor.o: EditorFlatColor.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c EditorFlatColor.cpp $(Debug_Include_Path) -o gccDebug/EditorFlatColor.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM EditorFlatColor.cpp $(Debug_Include_Path) > gccDebug/EditorFlatColor.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the Debug configuration...
-include gccDebug/../public/GameSoundEmitterSystem.d
gccDebug/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(Debug_Include_Path) -o gccDebug/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(Debug_Preprocessor_Definitions) $(Debug_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(Debug_Include_Path) > gccDebug/../public/GameSoundEmitterSystem.d

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

# Builds the Release configuration...
.PHONY: Release
Release: create_folders gccRelease/main.o gccRelease/../public/ConCommand.o gccRelease/../public/ConCommandBase.o gccRelease/../public/ConVar.o gccRelease/../public/Utils/GeomTools.o gccRelease/../public/Math/math_util.o gccRelease/../Engine/studio_egf.o gccRelease/../Engine/modelloader_shared.o gccRelease/MaterialListFrame.o gccRelease/RenderWindows.o gccRelease/EditorMainFrame.o gccRelease/EditorLevel.o gccRelease/EntityDef.o gccRelease/UndoObserver.o gccRelease/wxFourWaySplitter.o gccRelease/../public/anim_activity.o gccRelease/../public/anim_events.o gccRelease/../public/BoneSetup.o gccRelease/EditableDecal.o gccRelease/EditableEntityParameters.o gccRelease/GroupEditable.o gccRelease/EditableEntity.o gccRelease/EditableSurface.o gccRelease/BaseEditableObject.o gccRelease/EqBrush.o gccRelease/BrushWinding.o gccRelease/../public/dsm_esm_loader.o gccRelease/../public/dsm_loader.o gccRelease/../public/dsm_obj_loader.o gccRelease/../public/mtriangle_framework.o gccRelease/BlockTool.o gccRelease/DecalTool.o gccRelease/ClipperTool.o gccRelease/EntityTool.o gccRelease/SelectionEditor.o gccRelease/TerrainPatchTool.o gccRelease/VertexTool.o gccRelease/EntityList.o gccRelease/LayerPanel.o gccRelease/BuildOptionsDialog.o gccRelease/EntityProperties.o gccRelease/LoadLevelDialog.o gccRelease/OptionsDialog.o gccRelease/SoundList.o gccRelease/SurfaceDialog.o gccRelease/TransformSelectionDialog.o gccRelease/../Engine/ViewRenderer.o gccRelease/../public/cmd_pacifier.o gccRelease/../public/Font.o gccRelease/DCRender2D.o gccRelease/EditorCamera.o gccRelease/grid.o gccRelease/../public/ViewParams.o gccRelease/../public/RenderList.o gccRelease/../public/BaseRenderableObject.o gccRelease/../Engine/DebugOverlay.o gccRelease/../public/BaseShader.o gccRelease/EditorFlatColor.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/../Engine/Audio/alsnd_ambient.o gccRelease/../Engine/Audio/alsnd_emitter.o gccRelease/../Engine/Audio/alsnd_sample.o gccRelease/../Engine/Audio/alsound_local.o gccRelease/../Engine/Audio/soundzero.o 
	g++ -fPIC -shared -Wl,-soname,libEqEdit.so -o gccRelease/libEqEdit.so gccRelease/main.o gccRelease/../public/ConCommand.o gccRelease/../public/ConCommandBase.o gccRelease/../public/ConVar.o gccRelease/../public/Utils/GeomTools.o gccRelease/../public/Math/math_util.o gccRelease/../Engine/studio_egf.o gccRelease/../Engine/modelloader_shared.o gccRelease/MaterialListFrame.o gccRelease/RenderWindows.o gccRelease/EditorMainFrame.o gccRelease/EditorLevel.o gccRelease/EntityDef.o gccRelease/UndoObserver.o gccRelease/wxFourWaySplitter.o gccRelease/../public/anim_activity.o gccRelease/../public/anim_events.o gccRelease/../public/BoneSetup.o gccRelease/EditableDecal.o gccRelease/EditableEntityParameters.o gccRelease/GroupEditable.o gccRelease/EditableEntity.o gccRelease/EditableSurface.o gccRelease/BaseEditableObject.o gccRelease/EqBrush.o gccRelease/BrushWinding.o gccRelease/../public/dsm_esm_loader.o gccRelease/../public/dsm_loader.o gccRelease/../public/dsm_obj_loader.o gccRelease/../public/mtriangle_framework.o gccRelease/BlockTool.o gccRelease/DecalTool.o gccRelease/ClipperTool.o gccRelease/EntityTool.o gccRelease/SelectionEditor.o gccRelease/TerrainPatchTool.o gccRelease/VertexTool.o gccRelease/EntityList.o gccRelease/LayerPanel.o gccRelease/BuildOptionsDialog.o gccRelease/EntityProperties.o gccRelease/LoadLevelDialog.o gccRelease/OptionsDialog.o gccRelease/SoundList.o gccRelease/SurfaceDialog.o gccRelease/TransformSelectionDialog.o gccRelease/../Engine/ViewRenderer.o gccRelease/../public/cmd_pacifier.o gccRelease/../public/Font.o gccRelease/DCRender2D.o gccRelease/EditorCamera.o gccRelease/grid.o gccRelease/../public/ViewParams.o gccRelease/../public/RenderList.o gccRelease/../public/BaseRenderableObject.o gccRelease/../Engine/DebugOverlay.o gccRelease/../public/BaseShader.o gccRelease/EditorFlatColor.o gccRelease/../public/GameSoundEmitterSystem.o gccRelease/../Engine/Audio/alsnd_ambient.o gccRelease/../Engine/Audio/alsnd_emitter.o gccRelease/../Engine/Audio/alsnd_sample.o gccRelease/../Engine/Audio/alsound_local.o gccRelease/../Engine/Audio/soundzero.o  $(Release_Implicitly_Linked_Objects)

# Compiles file main.cpp for the Release configuration...
-include gccRelease/main.d
gccRelease/main.o: main.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c main.cpp $(Release_Include_Path) -o gccRelease/main.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM main.cpp $(Release_Include_Path) > gccRelease/main.d

# Compiles file ../public/ConCommand.cpp for the Release configuration...
-include gccRelease/../public/ConCommand.d
gccRelease/../public/ConCommand.o: ../public/ConCommand.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ConCommand.cpp $(Release_Include_Path) -o gccRelease/../public/ConCommand.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ConCommand.cpp $(Release_Include_Path) > gccRelease/../public/ConCommand.d

# Compiles file ../public/ConCommandBase.cpp for the Release configuration...
-include gccRelease/../public/ConCommandBase.d
gccRelease/../public/ConCommandBase.o: ../public/ConCommandBase.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ConCommandBase.cpp $(Release_Include_Path) -o gccRelease/../public/ConCommandBase.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ConCommandBase.cpp $(Release_Include_Path) > gccRelease/../public/ConCommandBase.d

# Compiles file ../public/ConVar.cpp for the Release configuration...
-include gccRelease/../public/ConVar.d
gccRelease/../public/ConVar.o: ../public/ConVar.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ConVar.cpp $(Release_Include_Path) -o gccRelease/../public/ConVar.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ConVar.cpp $(Release_Include_Path) > gccRelease/../public/ConVar.d

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

# Compiles file MaterialListFrame.cpp for the Release configuration...
-include gccRelease/MaterialListFrame.d
gccRelease/MaterialListFrame.o: MaterialListFrame.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c MaterialListFrame.cpp $(Release_Include_Path) -o gccRelease/MaterialListFrame.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM MaterialListFrame.cpp $(Release_Include_Path) > gccRelease/MaterialListFrame.d

# Compiles file RenderWindows.cpp for the Release configuration...
-include gccRelease/RenderWindows.d
gccRelease/RenderWindows.o: RenderWindows.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c RenderWindows.cpp $(Release_Include_Path) -o gccRelease/RenderWindows.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM RenderWindows.cpp $(Release_Include_Path) > gccRelease/RenderWindows.d

# Compiles file EditorMainFrame.cpp for the Release configuration...
-include gccRelease/EditorMainFrame.d
gccRelease/EditorMainFrame.o: EditorMainFrame.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditorMainFrame.cpp $(Release_Include_Path) -o gccRelease/EditorMainFrame.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditorMainFrame.cpp $(Release_Include_Path) > gccRelease/EditorMainFrame.d

# Compiles file EditorLevel.cpp for the Release configuration...
-include gccRelease/EditorLevel.d
gccRelease/EditorLevel.o: EditorLevel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditorLevel.cpp $(Release_Include_Path) -o gccRelease/EditorLevel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditorLevel.cpp $(Release_Include_Path) > gccRelease/EditorLevel.d

# Compiles file EntityDef.cpp for the Release configuration...
-include gccRelease/EntityDef.d
gccRelease/EntityDef.o: EntityDef.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityDef.cpp $(Release_Include_Path) -o gccRelease/EntityDef.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityDef.cpp $(Release_Include_Path) > gccRelease/EntityDef.d

# Compiles file UndoObserver.cpp for the Release configuration...
-include gccRelease/UndoObserver.d
gccRelease/UndoObserver.o: UndoObserver.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c UndoObserver.cpp $(Release_Include_Path) -o gccRelease/UndoObserver.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM UndoObserver.cpp $(Release_Include_Path) > gccRelease/UndoObserver.d

# Compiles file wxFourWaySplitter.cpp for the Release configuration...
-include gccRelease/wxFourWaySplitter.d
gccRelease/wxFourWaySplitter.o: wxFourWaySplitter.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c wxFourWaySplitter.cpp $(Release_Include_Path) -o gccRelease/wxFourWaySplitter.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM wxFourWaySplitter.cpp $(Release_Include_Path) > gccRelease/wxFourWaySplitter.d

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

# Compiles file EditableDecal.cpp for the Release configuration...
-include gccRelease/EditableDecal.d
gccRelease/EditableDecal.o: EditableDecal.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditableDecal.cpp $(Release_Include_Path) -o gccRelease/EditableDecal.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditableDecal.cpp $(Release_Include_Path) > gccRelease/EditableDecal.d

# Compiles file EditableEntityParameters.cpp for the Release configuration...
-include gccRelease/EditableEntityParameters.d
gccRelease/EditableEntityParameters.o: EditableEntityParameters.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditableEntityParameters.cpp $(Release_Include_Path) -o gccRelease/EditableEntityParameters.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditableEntityParameters.cpp $(Release_Include_Path) > gccRelease/EditableEntityParameters.d

# Compiles file GroupEditable.cpp for the Release configuration...
-include gccRelease/GroupEditable.d
gccRelease/GroupEditable.o: GroupEditable.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c GroupEditable.cpp $(Release_Include_Path) -o gccRelease/GroupEditable.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM GroupEditable.cpp $(Release_Include_Path) > gccRelease/GroupEditable.d

# Compiles file EditableEntity.cpp for the Release configuration...
-include gccRelease/EditableEntity.d
gccRelease/EditableEntity.o: EditableEntity.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditableEntity.cpp $(Release_Include_Path) -o gccRelease/EditableEntity.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditableEntity.cpp $(Release_Include_Path) > gccRelease/EditableEntity.d

# Compiles file EditableSurface.cpp for the Release configuration...
-include gccRelease/EditableSurface.d
gccRelease/EditableSurface.o: EditableSurface.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditableSurface.cpp $(Release_Include_Path) -o gccRelease/EditableSurface.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditableSurface.cpp $(Release_Include_Path) > gccRelease/EditableSurface.d

# Compiles file BaseEditableObject.cpp for the Release configuration...
-include gccRelease/BaseEditableObject.d
gccRelease/BaseEditableObject.o: BaseEditableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BaseEditableObject.cpp $(Release_Include_Path) -o gccRelease/BaseEditableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BaseEditableObject.cpp $(Release_Include_Path) > gccRelease/BaseEditableObject.d

# Compiles file EqBrush.cpp for the Release configuration...
-include gccRelease/EqBrush.d
gccRelease/EqBrush.o: EqBrush.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EqBrush.cpp $(Release_Include_Path) -o gccRelease/EqBrush.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EqBrush.cpp $(Release_Include_Path) > gccRelease/EqBrush.d

# Compiles file BrushWinding.cpp for the Release configuration...
-include gccRelease/BrushWinding.d
gccRelease/BrushWinding.o: BrushWinding.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BrushWinding.cpp $(Release_Include_Path) -o gccRelease/BrushWinding.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BrushWinding.cpp $(Release_Include_Path) > gccRelease/BrushWinding.d

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

# Compiles file ../public/mtriangle_framework.cpp for the Release configuration...
-include gccRelease/../public/mtriangle_framework.d
gccRelease/../public/mtriangle_framework.o: ../public/mtriangle_framework.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/mtriangle_framework.cpp $(Release_Include_Path) -o gccRelease/../public/mtriangle_framework.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/mtriangle_framework.cpp $(Release_Include_Path) > gccRelease/../public/mtriangle_framework.d

# Compiles file BlockTool.cpp for the Release configuration...
-include gccRelease/BlockTool.d
gccRelease/BlockTool.o: BlockTool.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BlockTool.cpp $(Release_Include_Path) -o gccRelease/BlockTool.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BlockTool.cpp $(Release_Include_Path) > gccRelease/BlockTool.d

# Compiles file DecalTool.cpp for the Release configuration...
-include gccRelease/DecalTool.d
gccRelease/DecalTool.o: DecalTool.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DecalTool.cpp $(Release_Include_Path) -o gccRelease/DecalTool.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DecalTool.cpp $(Release_Include_Path) > gccRelease/DecalTool.d

# Compiles file ClipperTool.cpp for the Release configuration...
-include gccRelease/ClipperTool.d
gccRelease/ClipperTool.o: ClipperTool.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ClipperTool.cpp $(Release_Include_Path) -o gccRelease/ClipperTool.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ClipperTool.cpp $(Release_Include_Path) > gccRelease/ClipperTool.d

# Compiles file EntityTool.cpp for the Release configuration...
-include gccRelease/EntityTool.d
gccRelease/EntityTool.o: EntityTool.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityTool.cpp $(Release_Include_Path) -o gccRelease/EntityTool.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityTool.cpp $(Release_Include_Path) > gccRelease/EntityTool.d

# Compiles file SelectionEditor.cpp for the Release configuration...
-include gccRelease/SelectionEditor.d
gccRelease/SelectionEditor.o: SelectionEditor.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SelectionEditor.cpp $(Release_Include_Path) -o gccRelease/SelectionEditor.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SelectionEditor.cpp $(Release_Include_Path) > gccRelease/SelectionEditor.d

# Compiles file TerrainPatchTool.cpp for the Release configuration...
-include gccRelease/TerrainPatchTool.d
gccRelease/TerrainPatchTool.o: TerrainPatchTool.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c TerrainPatchTool.cpp $(Release_Include_Path) -o gccRelease/TerrainPatchTool.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM TerrainPatchTool.cpp $(Release_Include_Path) > gccRelease/TerrainPatchTool.d

# Compiles file VertexTool.cpp for the Release configuration...
-include gccRelease/VertexTool.d
gccRelease/VertexTool.o: VertexTool.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c VertexTool.cpp $(Release_Include_Path) -o gccRelease/VertexTool.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM VertexTool.cpp $(Release_Include_Path) > gccRelease/VertexTool.d

# Compiles file EntityList.cpp for the Release configuration...
-include gccRelease/EntityList.d
gccRelease/EntityList.o: EntityList.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityList.cpp $(Release_Include_Path) -o gccRelease/EntityList.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityList.cpp $(Release_Include_Path) > gccRelease/EntityList.d

# Compiles file LayerPanel.cpp for the Release configuration...
-include gccRelease/LayerPanel.d
gccRelease/LayerPanel.o: LayerPanel.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c LayerPanel.cpp $(Release_Include_Path) -o gccRelease/LayerPanel.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM LayerPanel.cpp $(Release_Include_Path) > gccRelease/LayerPanel.d

# Compiles file BuildOptionsDialog.cpp for the Release configuration...
-include gccRelease/BuildOptionsDialog.d
gccRelease/BuildOptionsDialog.o: BuildOptionsDialog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c BuildOptionsDialog.cpp $(Release_Include_Path) -o gccRelease/BuildOptionsDialog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM BuildOptionsDialog.cpp $(Release_Include_Path) > gccRelease/BuildOptionsDialog.d

# Compiles file EntityProperties.cpp for the Release configuration...
-include gccRelease/EntityProperties.d
gccRelease/EntityProperties.o: EntityProperties.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EntityProperties.cpp $(Release_Include_Path) -o gccRelease/EntityProperties.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EntityProperties.cpp $(Release_Include_Path) > gccRelease/EntityProperties.d

# Compiles file LoadLevelDialog.cpp for the Release configuration...
-include gccRelease/LoadLevelDialog.d
gccRelease/LoadLevelDialog.o: LoadLevelDialog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c LoadLevelDialog.cpp $(Release_Include_Path) -o gccRelease/LoadLevelDialog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM LoadLevelDialog.cpp $(Release_Include_Path) > gccRelease/LoadLevelDialog.d

# Compiles file OptionsDialog.cpp for the Release configuration...
-include gccRelease/OptionsDialog.d
gccRelease/OptionsDialog.o: OptionsDialog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c OptionsDialog.cpp $(Release_Include_Path) -o gccRelease/OptionsDialog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM OptionsDialog.cpp $(Release_Include_Path) > gccRelease/OptionsDialog.d

# Compiles file SoundList.cpp for the Release configuration...
-include gccRelease/SoundList.d
gccRelease/SoundList.o: SoundList.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SoundList.cpp $(Release_Include_Path) -o gccRelease/SoundList.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SoundList.cpp $(Release_Include_Path) > gccRelease/SoundList.d

# Compiles file SurfaceDialog.cpp for the Release configuration...
-include gccRelease/SurfaceDialog.d
gccRelease/SurfaceDialog.o: SurfaceDialog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c SurfaceDialog.cpp $(Release_Include_Path) -o gccRelease/SurfaceDialog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM SurfaceDialog.cpp $(Release_Include_Path) > gccRelease/SurfaceDialog.d

# Compiles file TransformSelectionDialog.cpp for the Release configuration...
-include gccRelease/TransformSelectionDialog.d
gccRelease/TransformSelectionDialog.o: TransformSelectionDialog.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c TransformSelectionDialog.cpp $(Release_Include_Path) -o gccRelease/TransformSelectionDialog.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM TransformSelectionDialog.cpp $(Release_Include_Path) > gccRelease/TransformSelectionDialog.d

# Compiles file ../Engine/ViewRenderer.cpp for the Release configuration...
-include gccRelease/../Engine/ViewRenderer.d
gccRelease/../Engine/ViewRenderer.o: ../Engine/ViewRenderer.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/ViewRenderer.cpp $(Release_Include_Path) -o gccRelease/../Engine/ViewRenderer.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/ViewRenderer.cpp $(Release_Include_Path) > gccRelease/../Engine/ViewRenderer.d

# Compiles file ../public/cmd_pacifier.cpp for the Release configuration...
-include gccRelease/../public/cmd_pacifier.d
gccRelease/../public/cmd_pacifier.o: ../public/cmd_pacifier.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/cmd_pacifier.cpp $(Release_Include_Path) -o gccRelease/../public/cmd_pacifier.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/cmd_pacifier.cpp $(Release_Include_Path) > gccRelease/../public/cmd_pacifier.d

# Compiles file ../public/Font.cpp for the Release configuration...
-include gccRelease/../public/Font.d
gccRelease/../public/Font.o: ../public/Font.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/Font.cpp $(Release_Include_Path) -o gccRelease/../public/Font.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/Font.cpp $(Release_Include_Path) > gccRelease/../public/Font.d

# Compiles file DCRender2D.cpp for the Release configuration...
-include gccRelease/DCRender2D.d
gccRelease/DCRender2D.o: DCRender2D.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c DCRender2D.cpp $(Release_Include_Path) -o gccRelease/DCRender2D.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM DCRender2D.cpp $(Release_Include_Path) > gccRelease/DCRender2D.d

# Compiles file EditorCamera.cpp for the Release configuration...
-include gccRelease/EditorCamera.d
gccRelease/EditorCamera.o: EditorCamera.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditorCamera.cpp $(Release_Include_Path) -o gccRelease/EditorCamera.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditorCamera.cpp $(Release_Include_Path) > gccRelease/EditorCamera.d

# Compiles file grid.cpp for the Release configuration...
-include gccRelease/grid.d
gccRelease/grid.o: grid.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c grid.cpp $(Release_Include_Path) -o gccRelease/grid.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM grid.cpp $(Release_Include_Path) > gccRelease/grid.d

# Compiles file ../public/ViewParams.cpp for the Release configuration...
-include gccRelease/../public/ViewParams.d
gccRelease/../public/ViewParams.o: ../public/ViewParams.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/ViewParams.cpp $(Release_Include_Path) -o gccRelease/../public/ViewParams.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/ViewParams.cpp $(Release_Include_Path) > gccRelease/../public/ViewParams.d

# Compiles file ../public/RenderList.cpp for the Release configuration...
-include gccRelease/../public/RenderList.d
gccRelease/../public/RenderList.o: ../public/RenderList.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/RenderList.cpp $(Release_Include_Path) -o gccRelease/../public/RenderList.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/RenderList.cpp $(Release_Include_Path) > gccRelease/../public/RenderList.d

# Compiles file ../public/BaseRenderableObject.cpp for the Release configuration...
-include gccRelease/../public/BaseRenderableObject.d
gccRelease/../public/BaseRenderableObject.o: ../public/BaseRenderableObject.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BaseRenderableObject.cpp $(Release_Include_Path) -o gccRelease/../public/BaseRenderableObject.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BaseRenderableObject.cpp $(Release_Include_Path) > gccRelease/../public/BaseRenderableObject.d

# Compiles file ../Engine/DebugOverlay.cpp for the Release configuration...
-include gccRelease/../Engine/DebugOverlay.d
gccRelease/../Engine/DebugOverlay.o: ../Engine/DebugOverlay.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../Engine/DebugOverlay.cpp $(Release_Include_Path) -o gccRelease/../Engine/DebugOverlay.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../Engine/DebugOverlay.cpp $(Release_Include_Path) > gccRelease/../Engine/DebugOverlay.d

# Compiles file ../public/BaseShader.cpp for the Release configuration...
-include gccRelease/../public/BaseShader.d
gccRelease/../public/BaseShader.o: ../public/BaseShader.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/BaseShader.cpp $(Release_Include_Path) -o gccRelease/../public/BaseShader.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/BaseShader.cpp $(Release_Include_Path) > gccRelease/../public/BaseShader.d

# Compiles file EditorFlatColor.cpp for the Release configuration...
-include gccRelease/EditorFlatColor.d
gccRelease/EditorFlatColor.o: EditorFlatColor.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c EditorFlatColor.cpp $(Release_Include_Path) -o gccRelease/EditorFlatColor.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM EditorFlatColor.cpp $(Release_Include_Path) > gccRelease/EditorFlatColor.d

# Compiles file ../public/GameSoundEmitterSystem.cpp for the Release configuration...
-include gccRelease/../public/GameSoundEmitterSystem.d
gccRelease/../public/GameSoundEmitterSystem.o: ../public/GameSoundEmitterSystem.cpp
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -c ../public/GameSoundEmitterSystem.cpp $(Release_Include_Path) -o gccRelease/../public/GameSoundEmitterSystem.o
	$(CPP_COMPILER) $(Release_Preprocessor_Definitions) $(Release_Compiler_Flags) -MM ../public/GameSoundEmitterSystem.cpp $(Release_Include_Path) > gccRelease/../public/GameSoundEmitterSystem.d

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

