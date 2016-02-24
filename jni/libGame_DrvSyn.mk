##
## libGame dynamic library
## containts main function ( android_main )
## Runs Driver Syndicate game loop itself
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= game
LOCAL_MODULE_FILENAME	:= libGame
LOCAL_CFLAGS    		:= -g -DANDROID -DNO_ENGINE -DBIG_REPLAYS -DNOENGINE -DGAME_DRIVERS -DEQ_USE_SDL -DSHINY_IS_COMPILED=FALSE -std=c++11 -pthread -fexceptions -Wno-invalid-offsetof
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
	$(LOCAL_PATH)/src_dependency/Shiny/include

LOCAL_SRC_FILES := \
	DriversGame/input.cpp\
	DriversGame/game_multiplayer.cpp\
	DriversGame/game_singleplayer.cpp\
	DriversGame/heightfield.cpp\
	DriversGame/level.cpp\
	DriversGame/levelobject.cpp\
	DriversGame/object_debris.cpp\
	DriversGame/object_physics.cpp\
	DriversGame/eqPhysics/eqBulletIndexedMesh.cpp\
	DriversGame/ShaderOverrides.cpp\
	DriversGame/Shader_StaticObjs.cpp\
	DriversGame/Shader_VehicleBody.cpp\
	DriversGame/Shader_Water.cpp\
	DriversGame/StateManager.cpp\
	DriversGame/car.cpp\
	DriversGame/eqPhysics/eqCollision_Object.cpp\
	DriversGame/eqPhysics/eqCollision_ObjectGrid.cpp\
	DriversGame/eqPhysics/eqPhysics.cpp\
	DriversGame/eqPhysics/eqPhysics_Body.cpp\
	DriversGame/state_title.cpp\
	DriversGame/session_stuff.cpp\
	DriversGame/state_game.cpp\
	DriversGame/state_lobby.cpp\
	DriversGame/state_mainmenu.cpp\
	DriversGame/replay.cpp\
	DriversGame/system.cpp\
	DriversGame/ui_luaMenu.cpp\
	DriversGame/window.cpp\
	DriversGame/world.cpp\
	DriversGame/region.cpp\
	DriversGame/object_scripted.cpp\
	DriversGame/object_sheets.cpp\
	DriversGame/object_static.cpp\
	DriversGame/object_trafficlight.cpp\
	DriversGame/object_tree.cpp\
	DriversGame/occluderSet.cpp\
	DriversGame/physics.cpp\
	DriversGame/predictable_object.cpp\
	DriversGame/AICarManager.cpp\
	DriversGame/AIPursuerCar.cpp\
	DriversGame/AITrafficCar.cpp\
	DriversGame/BillboardList.cpp\
	DriversGame/CameraAnimator.cpp\
	DriversGame/DrvSynHUD.cpp\
	DriversGame/EventFSM.cpp\
	DriversGame/GameObject.cpp\
	DriversGame/LuaBinding_Drivers.cpp\
	DriversGame/NetPlayer.cpp\
	DriversGame/Rain.cpp\
	shared_engine/DebugOverlay.cpp\
	shared_engine/EGFInstancer.cpp\
	shared_engine/EffectRender.cpp\
	shared_engine/EngineSpew.cpp\
	shared_engine/EngineVersion.cpp\
	shared_engine/EqParticles_Render.cpp\
	shared_engine/Font.cpp\
	shared_engine/FontCache.cpp\
	shared_engine/FontLayoutBuilders.cpp\
	shared_engine/KeyBinding/Keys.cpp\
	shared_engine/Network/NETThread.cpp\
	shared_engine/cfgloader.cpp\
	shared_engine/eqGlobalMutex.cpp\
	shared_engine/eqParallelJobs.cpp\
	shared_engine/materialsystem/MaterialProxy.cpp\
	shared_engine/modelloader_shared.cpp\
	shared_engine/physics/BulletShapeCache.cpp\
	shared_engine/studio_egf.cpp\
	shared_engine/sys_console.cpp\
	shared_game/GameSoundEmitterSystem.cpp\
	shared_engine/Audio/alsound_local.cpp\
	shared_engine/Audio/alsnd_emitter.cpp\
	shared_engine/Audio/alsnd_sample.cpp\
	shared_engine/Audio/alsnd_stream.cpp\
	shared_engine/Audio/soundzero.cpp\
	public/ViewParams.cpp\
	public/luabinding/LuaBinding.cpp\
	public/luabinding/LuaBinding_Engine.cpp\
	public/materialsystem/BaseShader.cpp\
	public/TextureAtlas.cpp\
	DriversGame/main.cpp\
	src_dependency/SDL2/src/main/android/SDL_android_main.c

LOCAL_SHARED_LIBRARIES := eqCore

LOCAL_STATIC_LIBRARIES := \
	android_native_app_glue\
	coreLib prevLib\
	jpeg\
	png\
	oolua\
	luajit\
	ogg\
	vorbis\
	bullet

LOCAL_STATIC_LIBRARIES += SDL2
# LOCAL_STATIC_LIBRARIES += Shiny
LOCAL_STATIC_LIBRARIES += OpenAL-MOB

include $(BUILD_SHARED_LIBRARY)
