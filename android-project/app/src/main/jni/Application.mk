
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# stlport_static
APP_STL := gnustl_static

APP_ABI := armeabi armeabi-v7a x86
#APP_PLATFORM := android-18
APP_PLATFORM := android-17
APP_CPPFLAGS := -std=c++11 -fexceptions