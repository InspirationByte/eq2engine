//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate shadow renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef SHADOWRENDERER_H
#define SHADOWRENDERER_H


#include "utils/RectanglePacker.h"
#include "DrvSynDecals.h"

class CGameObject;

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

	void	AddShadowCaster( CGameObject* object, struct shadowListObject_t* addTo = NULL );

	void	Clear();

	void	RenderShadowCasters();
	void	Draw();

	void	SetShadowAngles( const Vector3D& angles );

protected:
	bool	CanCastShadows(CGameObject* object);

	void	RenderShadow(CGameObject* object, ubyte bodyGroups, int mode);

	CRectanglePacker	m_packer;

	Vector2D			m_shadowTextureSize;
	Vector2D			m_shadowTexelSize; // 1.0 / shadowTextureSize

	Vector3D			m_shadowAngles;

	ITexture*			m_shadowTexture;
	//ITexture*			m_shadowRt;

	IMaterial*			m_matVehicle;
	IMaterial*			m_matSkinned;
	IMaterial*			m_matSimple;

	IMaterial*			m_shadowResult;

	bool				m_isInit;

	bool				m_invalidateAllDecals;
};

#endif // SHADOWRENDERER_H