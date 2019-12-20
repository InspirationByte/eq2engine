##
## libGame dynamic library
## containts main function ( android_main )
## Runs Driver Syndicate game loop itself
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= game
LOCAL_MODULE_FILENAME	:= libGame
LOCAL_CFLAGS    		:= -g -DANDROID -DNO_ENGINE -DBIG_REPLAYS -DNOENGINE -DGAME_DRIVERS -DEQ_USE_SDL -DSHINY_STATIC_LINK=TRUE -std=c++11 -pthread -fexceptions -Wno-invalid-offsetof
LOCAL_LDFLAGS			:= -pthread

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public		\
	$(LOCAL_PATH)/public/core 	\
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public/math\
	$(LOCAL_PATH)/public/materialsystem\
	$(LOCAL_PATH)/shared_engine\
	$(LOCAL_PATH)/shared_game\
	$(LOCAL_PATH)/src_dependency/luajit/src\
	$(LOCAL_PATH)/src_dependency/luautf8\
	$(LOCAL_PATH)/src_dependency/Shiny/include

LOCAL_SRC_FILES := \
	DriversGame/input.cpp\
	DriversGame/session_base.cpp\
	DriversGame/game_multiplayer.cpp\
	DriversGame/game_singleplayer.cpp\
	DriversGame/heightfield.cpp\
	DriversGame/level.cpp\
	DriversGame/levelobject.cpp\
	DriversGame/eqPhysics/eqBulletIndexedMesh.cpp\
	DriversGame/eqPhysics/eqCollision_Object.cpp\
	DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp\
	DriversGame/eqPhysics/eqPhysics.cpp\
	DriversGame/eqPhysics/eqPhysics_Body.cpp\
	DriversGame/eqPhysics/eqPhysics_HingeJoint.cpp\
	DriversGame/eqPhysics/eqPhysics_MaxDistConstraint.cpp\
	DriversGame/eqPhysics/eqPhysics_PointConstraint.cpp\
	DriversGame/session_stuff.cpp\
	DriversGame/DrvSynStates.cpp\
	DriversGame/state_title.cpp\
	DriversGame/state_game.cpp\
	DriversGame/state_lobby.cpp\
	DriversGame/state_mainmenu.cpp\
	DriversGame/replay.cpp\
	DriversGame/director.cpp\
	DriversGame/ui_luaMenu.cpp\
	DriversGame/world.cpp\
	DriversGame/region.cpp\
	DriversGame/GameDefs.cpp\
	DriversGame/ControllableObject.cpp\
	DriversGame/car.cpp\
	DriversGame/pedestrian.cpp\
	DriversGame/object_debris.cpp\
	DriversGame/object_physics.cpp\
	DriversGame/object_scripted.cpp\
	DriversGame/object_sheets.cpp\
	DriversGame/object_static.cpp\
	DriversGame/object_trafficlight.cpp\
	DriversGame/object_tree.cpp\
	DriversGame/object_waterflow.cpp\
	DriversGame/occluderSet.cpp\
	DriversGame/physics.cpp\
	DriversGame/predictable_object.cpp\
	DriversGame/AIManager.cpp\
	DriversGame/AITrafficCar.cpp\
	DriversGame/AIPursuerCar.cpp\
	DriversGame/AIHandling.cpp\
	DriversGame/AIHornSequencer.cpp\
	DriversGame/AIManipulator_CollisionAvoidance.cpp\
	DriversGame/AIManipulator_Navigation.cpp\
	DriversGame/AIManipulator_StabilityControl.cpp\
	DriversGame/AIManipulator_TargetAvoidance.cpp\
	DriversGame/AIManipulator_TargetChaser.cpp\
	DriversGame/AIManipulator_Traffic.cpp\
	DriversGame/BillboardList.cpp\
	DriversGame/CameraAnimator.cpp\
	DriversGame/DrvSynHUD.cpp\
	DriversGame/EqUI_DrvSynTimer.cpp\
	DriversGame/DrvSynEqUIControls.cpp\
	DriversGame/EventFSM.cpp\
	DriversGame/GameObject.cpp\
	DriversGame/LuaBinding_Drivers.cpp\
	DriversGame/NetPlayer.cpp\
	DriversGame/Player.cpp\
	DriversGame/Rain.cpp\
	DriversGame/ParticleEffects.cpp\
	DriversGame/DrvSynDecals.cpp\
	DriversGame/ShadowRenderer.cpp\
	DriversGame/ShaderOverrides.cpp\
	DriversGame/Shader_Shadow.cpp\
	DriversGame/Shader_ShadowBuild.cpp\
	DriversGame/Shader_Skinned.cpp\
	DriversGame/Shader_Sky.cpp\
	DriversGame/Shader_StaticObjs.cpp\
	DriversGame/Shader_VehicleBody.cpp\
	DriversGame/Shader_Water.cpp\
	shared_engine/DebugOverlay.cpp\
	shared_engine/EGFInstancer.cpp\
	shared_engine/EffectRender.cpp\
	shared_engine/EngineVersion.cpp\
	shared_engine/EqParticles_Render.cpp\
	shared_engine/Font.cpp\
	shared_engine/FontCache.cpp\
	shared_engine/FontLayoutBuilders.cpp\
	shared_engine/KeyBinding/InputCommandBinder.cpp\
	shared_engine/Network/NETThread.cpp\
	shared_engine/cfgloader.cpp\
	shared_engine/eqGlobalMutex.cpp\
	shared_engine/eqParallelJobs.cpp\
	shared_engine/modelloader_shared.cpp\
	shared_engine/physics/BulletShapeCache.cpp\
	shared_engine/studio_egf.cpp\
	shared_engine/SpriteBuilder.cpp\
	shared_engine/ViewParams.cpp\
	shared_engine/Audio/alsound_local.cpp\
	shared_engine/Audio/alsnd_emitter.cpp\
	shared_engine/Audio/alsnd_sample.cpp\
	shared_engine/Audio/alsnd_stream.cpp\
	shared_engine/Audio/soundzero.cpp\
	shared_engine/EqUI/EqUI_Manager.cpp\
	shared_engine/EqUI/EqUI_Panel.cpp\
	shared_engine/EqUI/EqUI_Label.cpp\
	shared_engine/EqUI/EqUI_Button.cpp\
	shared_engine/EqUI/EqUI_Image.cpp\
	shared_engine/EqUI/IEqUI_Control.cpp\
	shared_engine/sys_host.cpp\
	shared_engine/sys_window.cpp\
	shared_engine/sys_main.cpp\
	shared_engine/sys_in_joystick.cpp\
	shared_engine/sys_state.cpp\
	shared_engine/sys_in_console.cpp\
	shared_game/GameSoundEmitterSystem.cpp\
	shared_game/anim_activity.cpp\
	shared_game/anim_events.cpp\
	shared_game/Animating.cpp\
	shared_game/BoneSetup.cpp\
	public/utils/RectanglePacker.cpp\
	public/luabinding/LuaBinding.cpp\
	public/luabinding/LuaBinding_Engine.cpp\
	public/materialsystem/BaseShader.cpp\
	public/TextureAtlas.cpp\
	src_dependency/SDL2/src/main/android/SDL_android_main.c

LOCAL_SHARED_LIBRARIES := eqCore

# android_native_app_glue\		- EXCLUDED
LOCAL_STATIC_LIBRARIES := \
	coreLib prevLib\
	jpeg\
	oolua\
	luajit\
	ogg\
	vorbis\
	bullet

LOCAL_STATIC_LIBRARIES += SDL2
LOCAL_STATIC_LIBRARIES += Shiny
LOCAL_STATIC_LIBRARIES += openal-soft

include $(BUILD_SHARED_LIBRARY)
