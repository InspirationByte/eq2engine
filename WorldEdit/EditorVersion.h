//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine version
//////////////////////////////////////////////////////////////////////////////////

#define EDITOR_VERSION "0.7a"

#define COMPILE_DATE __DATE__
#define COMPILE_TIME __TIME__

#ifdef ENGINE_RELEASEBUILD
#	define ENGINE_DEVSTATE "Release"
#else
#	define ENGINE_DEVSTATE "Alpha"
#endif

//Don't Change! The BuildNumberIncreator will done this!
#define BUILD_NUMBER_EDITOR 6234

// Subtract value for alpha, beta, release
#define BUILD_NUMBER_SUBTRACTED 0