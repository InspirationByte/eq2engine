//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine version
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENGVERSION_H
#define ENGVERSION_H

//#define ENGINE_NAME "Muscle"
//#define ENGINE_NAME "DarkTech 'Altereality'"
#define ENGINE_NAME			"Equilibrium 2"
#define ENGINE_NAME_SHORT	"Eq"

#define ENGINE_VERSION	"1"

#define COMPILE_DATE __DATE__
#define COMPILE_TIME __TIME__

#ifdef ENGINE_RELEASEBUILD
#	define ENGINE_DEVSTATE "Release"
#else
#	define ENGINE_DEVSTATE "Beta"
#endif

int GetEngineBuildNumber();

//Don't Change! The BuildNumberIncreator will done this!
#define BUILD_NUMBER_ENGINE			GetEngineBuildNumber() // 15943 times it was compiled from 28 feb 2009 to 2 nov 2013

#endif // ENGVERSION_H