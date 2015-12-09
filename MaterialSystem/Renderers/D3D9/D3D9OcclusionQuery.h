//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Occlusion query object
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D9OCCLUSIONQUERY_H
#define D3D9OCCLUSIONQUERY_H

#include "IOcclusionQuery.h"
#include <d3d9.h>

class CD3D9OcclusionQuery : public IOcclusionQuery
{
public:
						CD3D9OcclusionQuery( LPDIRECT3DDEVICE9 dev );
						~CD3D9OcclusionQuery();

	// begins the occlusion query issue
	void				Begin();

	// ends the occlusion query issue
	void				End();

	// returns status
	bool				IsReady();

	// after End() and IsReady() == true it does matter
	uint32				GetVisiblePixels();

protected:

	LPDIRECT3DQUERY9		m_query;
	DWORD					m_pixelsVisible;
	bool					m_ready;
};

#endif // D3D9OCCLUSIONQUERY_H