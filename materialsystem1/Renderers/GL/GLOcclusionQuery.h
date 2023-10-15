//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Occlusion query object
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IOcclusionQuery.h"

class CGLOcclusionQuery : public IOcclusionQuery
{
public:
						CGLOcclusionQuery();
						~CGLOcclusionQuery();

	// begins the occlusion query issue
	void				Begin();

	// ends the occlusion query issue
	void				End();

	// returns status
	bool				IsReady();

	// after End() and IsReady() == true it does matter
	uint32				GetVisiblePixels();

protected:

	uint				m_query{ 0 };
	uint				m_pixelsVisible{ 0 };
	bool				m_ready{ true };
};
