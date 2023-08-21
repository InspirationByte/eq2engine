//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr const int MAX_VISIBLE_EFFECTS = 4096;

class CParticleBatch;
struct AtlasEntry;

class IEffect
{
public:
					IEffect();
	virtual			~IEffect() {}

	void			SetSortOrigin(const Vector3D &origin);
	const Vector3D& GetOrigin() const { return m_vOrigin; }

	virtual void	DestroyEffect() {};

	// Draws effect. required for overriding
	virtual bool	DrawEffect(float dTime) = 0;

	float			GetLifetime() const			{return m_fLifeTime;}
	float			GetStartLifetime() const	{return m_fStartLifeTime;}
	float			GetLifetimePercent() const	{return m_fLifeTime / m_fStartLifeTime;}
	float			GetDistanceToCamera() const	{return m_fDistanceToView;}

protected:

	void InternalInit(const Vector3D &origin, float lifetime, CParticleBatch* group, AtlasEntry* entry)
	{
		m_vOrigin = origin;
		SetSortOrigin(origin);

		m_fStartLifeTime = lifetime;
		m_fLifeTime = lifetime;

		m_atlGroup = group;
		m_atlEntry = entry;
	}

	Vector3D			m_vOrigin;
	CParticleBatch*		m_atlGroup;
	AtlasEntry*			m_atlEntry;

	float				m_fStartLifeTime;
	float				m_fLifeTime;
	float				m_fDistanceToView;
};

//-------------------------------------------------------------------------------------
// Effect Renderer class
// Used to render variuos effects, programmer can define own rendering algorithm
//-------------------------------------------------------------------------------------

class CEffectRenderer
{
	friend class IEffect;
public:
				CEffectRenderer();

	void		AddEffect(IEffect* pEffect);
	void		DrawEffects(float dt);

	void		RemoveAllEffects();

	void		SetViewSortPosition(const Vector3D& origin);
	Vector3D	GetViewSortPosition() const;

protected:
	void		RemoveEffect(int index);

private:
	FixedArray<IEffect*, MAX_VISIBLE_EFFECTS>	m_effectList;
	Vector3D	m_viewPos;
};

extern CStaticAutoPtr<CEffectRenderer> effectrenderer;
