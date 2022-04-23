//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides all shared definitions of engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef STACKALLOC_H
#define STACKALLOC_H

#include "PlatformDef.h"

#ifdef PLAT_WIN
#define  stackalloc( _size ) _alloca( _size )
#define  stackfree( _p )   0
#elif PLAT_POSIX
#define  stackalloc( _size ) alloca( _size )
#define  stackfree( _p )   0
#endif

#endif // STACKALLOC_H