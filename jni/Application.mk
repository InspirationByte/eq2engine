APP_PROJECT_PATH		:= $(call my-dir)
APP_ABI         		:= armeabi armeabi-v7a # x86 x86_64
APP_STL			 		:= gnustl_static
APP_BUILD_SCRIPT 		:= $(APP_PROJECT_PATH)/Android.mk
APP_OPTIM 				:= release
APP_PLATFORM	 		:= android-16
NDK_APP_SHORT_COMMANDS	:= true
APP_MODULES := \
	android_native_app_glue \
	jpeg	\
	png		\
	coreLib	\
	prevLib	\
	eqCore	\
	ogg		\
	vorbis	\


