//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Snow emitter
//////////////////////////////////////////////////////////////////////////////////

#ifndef Snow_H
#define Snow_H

#include "math\DkMath.h"
#include "DebugInterface.h"
#include "ParticleRenderer.h"

#define MAX_SNOW_PARTICLES 700
#define SNOW_RADIUS 500
#define SNOW_START_HEIGHT 200

class SnowEmitter;

class SnowParticle
{
public:
	SnowParticle();

	void Update(float dt);

	Vector3D origin;
	Vector3D velocity;

	SnowEmitter* emit;

	float		m_fAngle;

};

class SnowEmitter
{
public:
	void Init();

	void Update_Draw(float dt, float emit_rate, float Snow_speed, Vector3D &angle);

protected:

	void EmitParticles(float rate, float snow_speed, Vector3D &angle);

	DkList<SnowParticle*> rParticles;
	
};

#endif