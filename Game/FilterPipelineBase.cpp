//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader Image filtering pipeline for giving the atmosphere to game
//////////////////////////////////////////////////////////////////////////////////

#include "FilterPipelineBase.h"
#include "GameRenderer.h"

//--------------------
// filter layer
//--------------------

CFilterLayer::CFilterLayer()
{
	m_pFilterMaterial = NULL;
	m_fAmount = 1.0f;
	m_vColor = ColorRGB(1);
	m_szLayerName = "InvalidFilterName";
	m_char = 0;
}

char* CFilterLayer::GetName()
{
	return (char*)m_szLayerName.GetData();
}

IMaterial* CFilterLayer::GetMaterial()
{
	return m_pFilterMaterial;
}

bool CFilterLayer::Init( kvkeybase_t* pSection )
{
	return false;
}

void CFilterLayer::Shutdown()
{

}

char CFilterLayer::GetChar()
{
	return m_char;
}

void CFilterLayer::Render(float fAmount)
{
	// copy colorbuffer to _rt_framebuffer
	GR_CopyFramebuffer();

	// bind this material
	materials->BindMaterial(m_pFilterMaterial, false);

	// setup colors and amounts

}

//--------------------
// the whole filter
//--------------------

bool CFilter::Init(const char* pszFileName)
{
	return false;
}

void CFilter::Render( float fAmount )
{

}