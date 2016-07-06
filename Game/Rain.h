//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rain emitter
//////////////////////////////////////////////////////////////////////////////////

#ifndef RAIN_H
#define RAIN_H

#include "math\DkMath.h"
#include "DebugInterface.h"
#include "ParticleRenderer.h"

#define MAX_RAIN_PARTICLES 700
#define RAIN_RADIUS 500
#define RAIN_START_HEIGHT 200

class RainEmitter;

class RainParticle
{
public:
	void Update(float dt);

	Vector3D origin;
	Vector3D velocity;

	RainEmitter* emit;

};

struct ripple_t
{
	Vector3D position;
	Vector3D tangent;
	Vector3D binormal;
	Vector3D normal;

	float startsize;
	float endsize;

	float cur_size;

	float startlifetime;
	float lifetime;
};

/*
class CRippleEffect : public IEffect
{
public:
	CRippleEffect(Vector3D &position, Vector3D &normal, float StartSize, float EndSize, float lifetime, int nMaterial, int nDropMaterial)
	{
		InternalInit(position, lifetime, nMaterial);

		Vector3D binorm, tang;
		VectorVectors(normal, tang, binorm);

		normal		= normal;
		binormal	= binorm;
		tangent		= tang;

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		drop_material = nDropMaterial;
	}

	bool DrawEffect(float dTime);

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;

	int			drop_material;
};
*/
class RainEmitter
{
public:
	void Init();

	void Update_Draw(float dt, float emit_rate, float rain_speed);
	void MakeRipple(Vector3D &origin, Vector3D &normal, float startsize, float endsize, float lifetime);
	void RemoveRipple(int index);

protected:

	void EmitParticles(float rate, float rain_speed);

	DkList<RainParticle*> rParticles;

	ripple_t*	m_pRipples[256];
	int			numRipples;	
};

#endif