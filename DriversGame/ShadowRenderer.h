//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate shadow renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef SHADOWRENDERER_H
#define SHADOWRENDERER_H

#include "GameObject.h"
#include "utils/RectanglePacker.h"
#include "DrvSynDecals.h"

//
// Shadow rendering it's like particles rendering
//
class CShadowRenderer : public CSpriteBuilder<PFXVertex_t>
{
public:
	CShadowRenderer();
	~CShadowRenderer();

	void	Init();
	void	Shutdown();

	void	AddShadowCaster( CGameObject* object );

	void	Clear();

	void	Render();

protected:
	CRectanglePacker	m_texAtlasPacker;

	Vector2D			m_shadowTextureSize;
	Vector2D			m_shadowTexelSize; // 1.0 / shadowTextureSize

	ITexture*			m_shadowTexture;
};

#endif // SHADOWRENDERER_H