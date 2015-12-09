//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef EFFECTRENDER_H
#define EFFECTRENDER_H

#include "TextureAtlas.h"
#include "utils/eqthread.h"

#include "math/DkMath.h"

class IEffect
{
public:
					IEffect();
	virtual			~IEffect() {}

	void			SetSortOrigin(const Vector3D &origin);

	virtual void	DestroyEffect() {};

	// Draws effect. required for overriding
	virtual bool	DrawEffect(float dTime) = 0;

	float			GetLifetime() const			{return m_fLifeTime;}
	float			GetStartLifetime() const	{return m_fStartLifeTime;}

	float			GetLifetimePercent() const	{return m_fLifeTime / m_fStartLifeTime;}

	Vector3D		GetOrigin() const			{return m_vOrigin;}

	float			GetDistanceToCamera() const	{return m_fDistanceToView;}

protected:

	void InternalInit(const Vector3D &origin, float lifetime, int material)
	{
		m_vOrigin = origin;
		SetSortOrigin(origin);

		m_fStartLifeTime = lifetime;
		m_fLifeTime = lifetime;
		//m_fEmitTime = gpGlobals->curtime;

		m_nMaterialGroup = material;
	}

	Vector3D		m_vOrigin;

	//float			m_fEmitTime;		// to use for compare with gpGlobals
	float			m_fStartLifeTime;
	float			m_fLifeTime;

	int				m_nMaterialGroup;
	// int			m_nAtlasIndex;		// atlas index, -1 = no atlas

	float			m_fDistanceToView;
};

//-------------------------------------------------------------------------------------
// Effect Renderer class
// Used to render variuos effects, programmer can define own rendering algorithm
//-------------------------------------------------------------------------------------

#define MAX_VISIBLE_EFFECTS		4096

class CEffectRenderer
{
	friend class IEffect;
public:
				CEffectRenderer();

	void		RegisterEffectForRender(IEffect* pEffect);
	void		DrawEffects(float dt);

	void		RemoveAllEffects();

	void		SetViewSortPosition(const Vector3D& origin);
	Vector3D	GetViewSortPosition() const;

protected:
	void		RemoveEffect(int index);

private:
	IEffect*							m_pEffectList[MAX_VISIBLE_EFFECTS];
	Threading::CEqInterlockedInteger	m_numEffects;

	Vector3D							m_viewPos;
};

extern CEffectRenderer* effectrenderer;

#endif // EFFECTRENDER_H
