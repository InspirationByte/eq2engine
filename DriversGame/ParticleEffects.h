//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle effects
//////////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLEEFFECTS_H
#define PARTICLEEFFECTS_H

#include "EffectRender.h"

class CFleckEffect : public IEffect
{
public:
	CFleckEffect(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity, 
				float StartSize, 
				float lifetime, 
				float rotation, 
				const ColorRGB& color, 
				CPFXAtlasGroup* group, TexAtlasEntry_t* entry);

	bool DrawEffect(float dTime);

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_vVelocity;
	Vector3D	m_vGravity;

	Vector3D	m_vLastColor;

	float		fStartSize;
	float		rotate;

	int			nDraws;

	CPFXAtlasGroup*		m_group;
	TexAtlasEntry_t*	m_entry;
};

class CSmokeEffect : public IEffect
{
public:
	CSmokeEffect(const Vector3D &position, const Vector3D &velocity, 
				float StartSize, float EndSize,
				float lifetime, 
				CPFXAtlasGroup* group, TexAtlasEntry_t* entry,
				float rotation, 
				const Vector3D &gravity = vec3_zero, 
				const Vector3D &color1 = color3_white, const Vector3D &color2 = color3_white, float alpha = 1.0f);

	bool DrawEffect(float dTime);

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_color1;
	Vector3D	m_color2;

	Vector3D	m_vGravity;

	Vector3D	m_vVelocity;

	Vector3D	m_vCurrColor;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		rotate;

	float		m_alpha;
};

class CSparkLine : public IEffect
{
public:
	CSparkLine(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity, 
				float length, float StartSize, float EndSize, 
				float lifetime, 
				CPFXAtlasGroup* group, TexAtlasEntry_t* entry);

	bool DrawEffect(float dTime);

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
};

void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type = 0);
void PoliceSirenEffect(float fCurTime, const ColorRGB& color, const Vector3D& pos, const Vector3D& dir_right, float rDist, float width);

#endif // PARTICLEEFFECTS_H