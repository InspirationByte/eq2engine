//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////


#if defined(_DEBUG) || defined(DEBUG)
#define EQ_DEBUG
#endif

#ifdef EQ_USE_SDL
#	define PLAT_SDL 1
#endif // EQ_USE_SDL

#ifdef _WIN32

#define PLAT_WIN 1

#ifdef _WIN64
#	define PLAT_WIN64 1
#else
#	define PLAT_WIN32 1
#endif // _WIN64

#else

#define PLAT_POSIX 1

#ifdef __APPLE__
#	define PLAT_OSX 1
#elif __LINUX__
#	define PLAT_LINUX 1
#elif __ANDROID__
#	define PLAT_ANDROID 1
#endif // __APPLE__

// TODO: ios
#endif // _WIN32