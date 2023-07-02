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

#ifndef __has_feature
// GCC does not have __has_feature...
#define __has_feature(feature) 0
#endif

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
// must be disiabled when ASAN is on
#define PPMEM_DISABLED
#endif

#if (defined(_RETAIL) || defined(__ANDROID__)) && !defined(PPMEM_DISABLED)
#define PPMEM_DISABLED
#endif // _RETAIL

#if !defined(_RETAIL)
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
#include "platform/eqatomic.h"
#include "platform/eqtimer.h"

#include "ds/IVirtualStream.h"

#include "ds/eqstring.h"
#include "ds/eqwstring.h"

#include "platform/eqthread.h"

#include "ds/mempool.h"
#include "ds/refcounted.h"
#include "ds/weakptr.h"
#include "ds/singleton.h"
#include "ds/range_for.h"
#include "ds/Array.h"
#include "ds/BitArray.h"
#include "ds/Map.h"
#include "ds/List.h"
#include "ds/function.h"
#include "ds/future.h"
#include "ds/defer.h"

#include "ds/MemoryStream.h"

#include "utils/strtools.h"
#include "utils/CRC32.h"

#include "math/math_common.h"
