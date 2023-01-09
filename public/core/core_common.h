//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Common core definitions
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#define CORE_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <limits.h>
#include <malloc.h>
#include <new>
#include <memory>
#include <type_traits>

#if defined(_INC_MINMAX)
#error Please remove minmax from includes
#endif

//---------------
// config vars

#if defined(_RETAIL) || defined(__ANDROID__)
#define PPMEM_DISABLE
#endif // _RETAIL

#if defined(_WIN32) && !defined(_RETAIL)	// Sorry, only Binbows atm
#define PROFILE_ENABLE
#endif

//---------------

#include "common_types.h"

#include "InterfaceManager.h"
#include "Logger.h"
#include "profiler.h"
#include "ppmem.h"
#include "cmdlib.h"

#include "platform/platform.h"
#include "platform/assert.h"
#include "platform/messagebox.h"
#include "platform/stackalloc.h"

#include "ds/eqstring.h"
#include "ds/eqwstring.h"

#include "platform/eqtimer.h"
#include "platform/eqthread.h"
#include "platform/eqatomic.h"

#include "ds/refcounted.h"
#include "ds/weakptr.h"
#include "ds/singleton.h"

#include "ds/Array.h"
#include "ds/BitArray.h"
#include "ds/Map.h"
#include "ds/List.h"
#include "ds/function.h"
#include "ds/future.h"

#include "ds/IVirtualStream.h"
#include "ds/MemoryStream.h"

#include "utils/strtools.h"
#include "utils/CRC32.h"

#include "math/math_common.h"
