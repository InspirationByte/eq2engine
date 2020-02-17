//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle and decal effects
//////////////////////////////////////////////////////////////////////////////////

#ifndef PEFFECTS_H
#define PEFFECTS_H

#include "BaseEngineHeader.h"
#include "ParticleRenderer.h"
#include "GameRenderer.h"

/*
struct effectdispatch_t
{
	char*		effectname;

	Vector3D	origin;
	Vector3D	direction;
	Vector3D	gravity;

	float		speed;

	float		lifetme;
	float		startsize;
	float		endsize;
	float		rotation;

	float		maxalpha;
};
*/

extern Vector3D ComputeLightingForPoint(Vector3D &point, bool doPhysics);

class CSmokeEffect : public IEffect
{
public:
	CSmokeEffect(Vector3D &position, Vector3D &velocity, float StartSize, float EndSize, float lifetime, int nMaterial, float rotation, Vector3D &gravity = vec3_zero, Vector3D &color1 = color3_white, Vector3D &color2 = color3_white, float alpha = 1.0f)
	{
		InternalInit( position, lifetime, nullptr, nullptr );

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		m_vVelocity = velocity;

		rotate = rotation;

		nDraws = 0;

		m_vLastColor = ComputeLightingForPoint(m_vOrigin, false);
		m_vCurrColor = m_vLastColor;

		m_vGravity = gravity;

		m_color1 = color1;
		m_color2 = color2;

		m_alpha = alpha;

		m_matindex = nMaterial;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime;

		if(nDraws > 5)
		{
			m_vLastColor = ComputeLightingForPoint(m_vOrigin, false);
			nDraws = 0;
		}

		m_vCurrColor = m_vLastColor;//lerp(m_vCurrColor, m_vLastColor, gpGlobals->frametime*5);

		float lifeTimePerc = GetLifetimePercent();

		float fStartAlpha = clamp((1.0f-lifeTimePerc) * 50.0f, 0,1);

		Vector3D col_lerp = lerp(m_color2, m_color1, lifeTimePerc);

		m_vVelocity += m_vGravity*dTime;

		SetSortOrigin(m_vOrigin);

		PFXBillboard_t effect;

		effect.vOrigin = m_vOrigin;
		effect.vColor = Vector4D(m_vCurrColor * col_lerp, lifeTimePerc*m_alpha*fStartAlpha);//Vector4D(lerp(m_vCurrColor, col_lerp, 0.25f),lifeTimePerc*m_alpha);
		effect.nGroupIndex = m_matindex;
		effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
		effect.fZAngle = lifeTimePerc*rotate;

		effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
		effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

		GR_DrawBillboard(&effect);

		nDraws++;

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_color1;
	Vector3D	m_color2;

	Vector3D	m_vGravity;

	Vector3D	m_vVelocity;

	Vector3D	m_vLastColor;
	Vector3D	m_vCurrColor;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		rotate;

	float		m_alpha;

	int			nDraws;

	int			m_matindex;
};

class CSparkLine : public IEffect
{
public:
	CSparkLine(Vector3D &position, Vector3D &velocity, Vector3D &gravity, float length, float StartSize, float EndSize, float lifetime, int nMaterial, bool light = true)
	{
		InternalInit(position, lifetime, nullptr, nullptr);

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		m_vVelocity = velocity;

		fLength = length;

		vGravity = gravity;

		lights = light;

		m_matindex = nMaterial;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime*2.0f;

		SetSortOrigin(m_vOrigin);

		float lifeTimePerc = GetLifetimePercent();

		Vector3D viewDir = m_vOrigin - g_pViewEntity->GetEyeOrigin();
		Vector3D lineDir = m_vVelocity;

		Vector3D vEnd = m_vOrigin + normalize(m_vVelocity)*(fLength*length(m_vVelocity)*0.001f);

		Vector3D ccross = normalize(cross(lineDir, viewDir));

		float scale = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

		Vector3D temp;

		ParticleVertex_t verts[4];

		VectorMA( vEnd, scale, ccross, temp );

		Vector4D color(Vector3D(1)*lifeTimePerc, 1);

		verts[0].point = temp;
		verts[0].texcoord = Vector2D(0,0);
		verts[0].color = color;

		VectorMA( vEnd, -scale, ccross, temp );

		verts[1].point = temp;
		verts[1].texcoord = Vector2D(0,1);
		verts[1].color = color;

		VectorMA( m_vOrigin, scale, ccross, temp );

		verts[2].point = temp;
		verts[2].texcoord = Vector2D(1,0);
		verts[2].color = color;

		VectorMA( m_vOrigin, -scale, ccross, temp );

		verts[3].point = temp;
		verts[3].texcoord = Vector2D(1,1);
		verts[3].color = color;

		AddParticleQuad(verts, m_matindex);

		internaltrace_t tr;
		physics->InternalTraceLine(m_vOrigin, vEnd, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &tr);

		if(lights)
		{
			dlight_t *light = viewrenderer->AllocLight();
			SetLightDefaults(light);

			light->position = m_vOrigin;
			light->color = Vector3D(0.9,0.4,0.25);
			light->intensity = lifeTimePerc;
			light->radius = 10*lifeTimePerc;
			light->nFlags = LFLAG_MISCLIGHT | LFLAG_NOSHADOWS;

			viewrenderer->AddLight(light);
		}

		if(tr.fraction < 1.0f)
		{
			m_vVelocity = reflect(m_vVelocity, tr.normal)*0.5f;
			m_fLifeTime -= dTime*2;
		}

		m_vVelocity += vGravity*dTime*2.0f;

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	vGravity;

	Vector3D	m_vVelocity;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		fLength;

	bool		lights;

	int			m_matindex;
};

void PrecacheEffects();
void UnloadEffects();

void UTIL_Impact( trace_t& tr, Vector3D &impactDirection);
void UTIL_ExplosionEffect(Vector3D &origin, Vector3D &direction, bool onGround);

void ObjectSoundEvents(IPhysicsObject* pObject, BaseEntity* pOwner);

// draws a tracer in particle system
void FX_DrawTracer(Vector3D &origin, Vector3D &line_dir, float width, float length, ColorRGBA &color, int nMaterial = -1);

#endif // PEFFECTS_H