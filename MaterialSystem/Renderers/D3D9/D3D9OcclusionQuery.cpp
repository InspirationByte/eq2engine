//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Occlusion query 
//////////////////////////////////////////////////////////////////////////////////

#include "D3D9OcclusionQuery.h"


CD3D9OcclusionQuery::CD3D9OcclusionQuery( LPDIRECT3DDEVICE9 dev )
{
	m_ready = false;
	m_pixelsVisible = 0;
	m_query = NULL;

	dev->CreateQuery( D3DQUERYTYPE_OCCLUSION, &m_query );
}

CD3D9OcclusionQuery::~CD3D9OcclusionQuery()
{
	if(m_query)
		m_query->Release();
}

// begins the occlusion query issue
void CD3D9OcclusionQuery::Begin()
{
	m_query->Issue( D3DISSUE_BEGIN );
	m_ready = false;
}

// ends the occlusion query issue
void CD3D9OcclusionQuery::End()
{
	m_query->Issue( D3DISSUE_END );
}

// returns status
bool CD3D9OcclusionQuery::IsReady()
{
	if(m_ready)
		return true;

	m_pixelsVisible = 0;
	m_ready = (m_query->GetData((void*)&m_pixelsVisible, sizeof(DWORD), D3DGETDATA_FLUSH) != S_FALSE);

	return m_ready;
}

// after End() and IsReady() == true it does matter
uint32 CD3D9OcclusionQuery::GetVisiblePixels()
{
	return m_pixelsVisible;
}