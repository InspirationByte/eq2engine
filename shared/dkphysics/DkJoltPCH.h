#pragma once

#if !defined(_RETAIL) || !defined(_PROFILE)
#define JPH_ENABLE_ASSERTS
#endif

#ifdef PPMEM_DISABLED
#define JPH_DISABLE_TEMP_ALLOCATOR
#define JPH_DISABLE_CUSTOM_ALLOCATOR
#endif

#define JPH_INSBYTE_NO_VEHICLES
#define JPH_INSBYTE_NO_SOFTBODY

#include <Jolt/Jolt.h>
#include "DkJoltConvert.h"