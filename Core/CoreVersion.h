//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Two Dark Core version
//////////////////////////////////////////////////////////////////////////////////

#ifndef COREVERSION_H
#define COREVERSION_H

#include "core_base_header.h"

#define ENGINE_NAME		"Equilibrium"
#define ENGINE_VERSION	"2"

#define COMPILE_DATE __DATE__
#define COMPILE_TIME __TIME__

#ifdef ENGINE_RELEASEBUILD
#	define ENGINE_DEVSTATE "Release"
#else
#	define ENGINE_DEVSTATE "Beta"
#endif

void CoreMessage();

//Don't Change! The BuildNumberIncreator will done this!
#define BUILD_NUMBER 5467

#endif // COREVERSION_H