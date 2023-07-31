//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Occlusion query 
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9OcclusionQuery.h"

#include "ShaderAPID3D9.h"
extern ShaderAPID3D9 s_shaderApi;

CD3D9OcclusionQuery::CD3D9OcclusionQuery(IDirect3DDevice9* dev)
{
	m_ready = false;
	m_pixelsVisible = 0;
	m_query = nullptr;

	m_dev = dev;
	m_dev->AddRef();

	Init();
}

CD3D9OcclusionQuery::~CD3D9OcclusionQuery()
{
	Destroy();
	m_dev->Release();
}

void CD3D9OcclusionQuery::Init()
{
	if(!m_query)
		m_dev->CreateQuery( D3DQUERYTYPE_OCCLUSION, &m_query );
}

void CD3D9OcclusionQuery::Destroy()
{
	if(m_query)
		m_query->Release();

	m_query = nullptr;
}

// begins the occlusion query issue
void CD3D9OcclusionQuery::Begin()
{
	if(!m_query)
		return;

	m_query->Issue( D3DISSUE_BEGIN );
	m_ready = false;
}

// ends the occlusion query issue
void CD3D9OcclusionQuery::End()
{
	if(!m_query)
		return;

	m_query->Issue( D3DISSUE_END );
}

// returns status
bool CD3D9OcclusionQuery::IsReady()
{
	if(m_ready)
		return true;

	if(!m_query)
		return true;

	m_pixelsVisible = 0;
	m_ready = (m_query->GetData((void*)&m_pixelsVisible, sizeof(DWORD), D3DGETDATA_FLUSH) != S_FALSE);

	const int msaa = s_shaderApi.m_params.multiSamplingMode;
	if(msaa > 1)
		m_pixelsVisible /= msaa;

	return m_ready;
}

// after End() and IsReady() == true it does matter
uint32 CD3D9OcclusionQuery::GetVisiblePixels()
{
	return m_pixelsVisible;
}