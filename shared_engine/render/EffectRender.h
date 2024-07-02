//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CParticleBatch;
using PFXAtlasRef = int;

static constexpr const int MAX_VISIBLE_EFFECTS = 4096;


class IEffect
{
public:
	virtual ~IEffect() = default;

	void			SetSortOrigin(const Vector3D &origin);
	const Vector3D& GetOrigin() const { return m_origin; }

	// Draws effect. required for overriding
	virtual bool	DrawEffect(float dTime) = 0;

	float			GetLifetimePercent() const	{ return m_lifeTime * m_lifeTimeRcp; }
	float			GetDistanceToCamera() const	{ return m_distToView; }

protected:

	void InternalInit(const Vector3D& origin, float lifetime, PFXAtlasRef atlasRef);

	Vector3D			m_origin{ vec3_zero };
	PFXAtlasRef			m_atlasRef;

	float				m_lifeTimeRcp{ 0.0f };
	float				m_lifeTime{ 0.0f };
	float				m_distToView{ 0.0f };
};

//-------------------------------------------------------------------------------------
// Effect Renderer class
// Used to render variuos effects, programmer can define own rendering algorithm
//-------------------------------------------------------------------------------------

class CEffectRenderer
{
	friend class IEffect;
public:
	void		AddEffect(IEffect* pEffect);
	void		DrawEffects(float dt);

	void		RemoveAllEffects();

	void		SetViewSortPosition(const Vector3D& origin);
	Vector3D	GetViewSortPosition() const;

protected:
	void		RemoveEffect(int index);

private:
	FixedArray<IEffect*, MAX_VISIBLE_EFFECTS>	m_effectList;
	Vector3D	m_viewPos{ vec3_zero };
};

extern CStaticAutoPtr<CEffectRenderer> g_effectRenderer;
