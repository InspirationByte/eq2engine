//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Base core and framework header
//////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_BASE_H
#define CORE_BASE_H

#ifdef _WIN32
#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif // _WIN32

#include "platform/Platform.h"			// Platform

#include "utils/strtools.h"				// Various string tools
#include "utils/eqtimer.h"				// Time measuring
#include "utils/KeyValues.h"			// json-like object notation

#include "IConCommandFactory.h"			// Console command factory
#include "ICommandAccessor.h"			// Console command factory

#include "InterfaceManager.h"			// Manager of interfaces
#include "IDkCore.h"					// Core class
#include "IEqCPUServices.h"				// CPU
#include "ILocalize.h"					// Localization system
#include "ppmem.h"						// own memory allocator
#include "ICmdLineParser.h"				// commandline parser interface
#include "IFileSystem.h"				// filesystem for handling files and archives

#include "DebugInterface.h"				// debug message interface

#endif //CORE_BASE_H

