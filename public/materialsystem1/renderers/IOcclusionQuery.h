//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Occlusion query object
//////////////////////////////////////////////////////////////////////////////////

#ifndef IOCCLUSIONQUERY_H
#define IOCCLUSIONQUERY_H

#include "dktypes.h"
#include "ppmem.h"

class IOcclusionQuery
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual ~IOcclusionQuery() {}

	// begins the occlusion query issue
	virtual void			Begin() = 0;

	// ends the occlusion query issue
	virtual void			End() = 0;

	// returns status
	virtual bool			IsReady() = 0;

	// after End() and IsReady() == true it does matter
	virtual uint32			GetVisiblePixels() = 0;
};

#endif // IOCCLUSIONQUERY_H