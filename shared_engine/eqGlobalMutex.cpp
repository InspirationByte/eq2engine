//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqEngine mutex storage
//////////////////////////////////////////////////////////////////////////////////

#include "eqGlobalMutex.h"

namespace Threading
{

static Threading::CEqMutex s_mutexPurposes[MUTEXPURPOSE_USED];

Threading::CEqMutex& GetGlobalMutex( EMutexPurpose purpose )
{
	return s_mutexPurposes[purpose];
}

};