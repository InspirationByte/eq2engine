//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader Image filtering pipeline for giving the atmosphere to game
//////////////////////////////////////////////////////////////////////////////////

#ifndef FILTERPIPELINE_H
#define FILTERPIPELINE_H

#include "materialsystem/IMaterialSystem.h"

//--------------------
// filter layer
//--------------------
class CFilterLayer
{
public:
	CFilterLayer();

	bool		Init( kvkeybase_t* pSection );
	void		Shutdown();

	char*		GetName();
	IMaterial*	GetMaterial();
	char		GetChar();

	void		Render(float fAmount);

protected:

	IMaterial*	m_pFilterMaterial;
	float		m_fAmount;
	ColorRGB	m_vColor;
	EqString	m_szLayerName;
	char		m_char;
};

//--------------------
// the whole filter
//--------------------
class CFilter
{
public:
	bool Init(const char* pszFileName);
	void Render( float fAmount );

protected:
	DkList<CFilterLayer*>	m_pLayers;
};

#endif // FILTERPIPELINE_H