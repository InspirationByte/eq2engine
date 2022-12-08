//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine version
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define ENGINE_NAME			"E2Engine"

#define ENGINE_VERSION		"2"

#define COMPILE_DATE		__DATE__
#define COMPILE_TIME		__TIME__

#ifndef COMPILE_CONFIGURATION
#define COMPILE_CONFIGURATION "NO_CONFIGURATION"
#endif

#ifndef COMPILE_PLATFORM
#define COMPILE_PLATFORM "UNK PLATFORM"
#endif

int GetEngineBuildNumber();

//Don't Change! The BuildNumberIncreator will done this!
#define BUILD_NUMBER_ENGINE			GetEngineBuildNumber() // 15943 times it was compiled from 28 feb 2009 to 2 nov 2013
