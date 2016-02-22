APP_PROJECT_PATH		:= $(call my-dir)

# later to build this for x86 x86_x64 armeabi64
APP_ABI         		:= armeabi armeabi-v7a # x86 x86_64

# need to support C++ stl
APP_STL			 	:= gnustl_static

APP_BUILD_SCRIPT 		:= $(APP_PROJECT_PATH)/Android.mk
NDK_APP_LIBS_OUT		:= $(APP_PROJECT_PATH)/../libs_android

APP_OPTIM 			:= release
APP_PLATFORM	 		:= android-16
NDK_APP_SHORT_COMMANDS		:= true

APP_MODULES := \
	android_native_app_glue \
	jpeg	\
	png		\
	coreLib	\
	prevLib	\
	eqCore	\
	ogg		\
	vorbis	\
	eqMatSystem\
	eqBaseShaders\
	eqNullRHI\
	eqGLRHI\
	SDL2_static\
	OpenAL-MOB\
	Shiny\
	luajit\
	oolua\
	game
