//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Occlusion query object
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IOcclusionQuery.h"

class CD3D9OcclusionQuery : public IOcclusionQuery
{
public:
						CD3D9OcclusionQuery(IDirect3DDevice9* dev);
						~CD3D9OcclusionQuery();

	void				Init();
	void				Destroy();

	// begins the occlusion query issue
	void				Begin();

	// ends the occlusion query issue
	void				End();

	// returns status
	bool				IsReady();

	// after End() and IsReady() == true it does matter
	uint32				GetVisiblePixels();

protected:

	IDirect3DQuery9*		m_query;
	IDirect3DDevice9*		m_dev;
	DWORD					m_pixelsVisible;
	bool					m_ready;
};