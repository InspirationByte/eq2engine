//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Occlusion query object
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IOcclusionQuery
{
public:
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