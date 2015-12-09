//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium sound sample
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQSOUNDSAMPLE_H
#define IEQSOUNDSAMPLE_H

#include "ppmem.h"
#include "refcounted.h"

//-------------------------------------------------------------------------------

enum ESampleFlags
{
	SAMPLE_FLAG_REMOVEWHENSTOPPED	= (1 << 0),
	SAMPLE_FLAG_STREAMED			= (1 << 1),
	SAMPLE_FLAG_LOOPING				= (1 << 2),
};

class IEqSoundSample : public RefCountedObject
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual				~IEqSoundSample() {}

	virtual	int			GetFlags() const = 0;
};

#endif // IEQSOUNDSAMPLE_H