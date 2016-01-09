//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rain emitter
//////////////////////////////////////////////////////////////////////////////////

#ifndef RAIN_H
#define RAIN_H

#include "math/DkMath.h"
#include "DebugInterface.h"
#include "EqParticles.h"

#define MAX_RAIN_PARTICLES 700
#define RAIN_RADIUS 500
#define RAIN_START_HEIGHT 200

//-------------------------------------------------------------------

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

class RainEmitter;

class CRippleEffect : public IEffect
{
public:
	CRippleEffect(const Vector3D &position, const Vector3D &normal, float StartSize, float EndSize, float lifetime);

	bool DrawEffect(float dTime);

protected:
	Vector3D		normal;

	float			fCurSize;

	float			fStartSize;
	float			fEndSize;
};

//------------------------------------------------------------------------------------------------------

class RainParticle
{
public:
	void Update(float dt);

	Vector3D origin;
	Vector3D velocity;

	RainEmitter* emit;
};

//-----------------------------------------------

class RainEmitter
{
	friend class RainParticle;

public:
	void Init();
	void Clear();

	void Update_Draw(float dt, float emit_rate, float rain_speed);

	void SetViewVelocity( const Vector3D& vel );

	TexAtlasEntry_t*		m_rainEntry;
	TexAtlasEntry_t*		m_rippleEntry;

protected:

	void EmitParticles(float dt, float rate, float rain_speed);

	DkList<RainParticle*>	rParticles;

	Vector3D				m_viewVelocity;

	float					m_curTime;
};

extern RainEmitter* g_pRainEmitter;

#endif
