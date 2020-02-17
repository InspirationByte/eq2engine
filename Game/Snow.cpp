//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Snow emitter
//////////////////////////////////////////////////////////////////////////////////

#include "Snow.h"

#include "GameRenderer.h"
#include "BaseEngineHeader.h"
#include "EngineEntities.h"
#include "Effects.h"

DECLARE_EFFECT_GROUP(Snow)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/Snow");
}

DECLARE_CVAR(snow_radius,800,"Snow radius",0);
DECLARE_CVAR(snow_start_h,900,"Snow start height",0);
DECLARE_CVAR(snow_max_particles,700,"Max Snow particles",0);
DECLARE_CVAR(snow_length,2,"Snow length",0);
DECLARE_CVAR(snow_width,2,"Snow width",0);
DECLARE_CVAR(snow_wind_speed,650,"Snow wind speed",0);

SnowParticle::SnowParticle()
{
	m_fAngle = RandomFloat(-150,150);
}

void SnowParticle::Update(float dt)
{
	Vector3D forwards = normalize(g_pViewEntity->GetEyeOrigin() - origin);

	// Convert to right vector
	Vector3D right;
	VectorRotate(forwards,Vector3D(0,54,0),&right);

	PFXBillboard_t Snow_particle;
	Snow_particle.vOrigin = origin;
	Snow_particle.vColor = Vector4D(ComputeLightingForPoint(origin, false),1);
	Snow_particle.nGroupIndex = EFFECT_GROUP_ID(Snow, 0);
	Snow_particle.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
	Snow_particle.fZAngle = m_fAngle;
	Snow_particle.fWide = snow_width.GetFloat();
	Snow_particle.fTall = snow_length.GetFloat();

	GR_DrawBillboard(&Snow_particle);

	m_fAngle += 130*gpGlobals->frametime;

	velocity += Vector3D(RandomFloat(-80,80),0,RandomFloat(-80,80));

	origin += velocity * dt;
}

void SnowEmitter::Init()
{
	
}

void SnowEmitter::Update_Draw(float dt, float emit_rate, float snow_speed, Vector3D &angle)
{
	if(!g_pViewEntity)
		return;

	Vector3D viewOrigin = g_pViewEntity->GetEyeOrigin();

	float startH = snow_start_h.GetFloat()+viewOrigin.y;

	EmitParticles(emit_rate, snow_speed, angle);

	for( int i = 0; i < rParticles.numElem();i++)
	{
		SnowParticle* pParticle = rParticles[i];

		float height = startH - rParticles[i]->origin.y;

		internaltrace_t snow_trace;
		Vector3D trace_start(pParticle->origin.x,startH,pParticle->origin.z);
		physics->InternalTraceLine(trace_start,trace_start - Vector3D(0,height+snow_length.GetFloat(),0),COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS,&snow_trace);

		if(pParticle->origin.y < (viewOrigin.y - 800) || snow_trace.fraction < 1.0)
		{
			delete pParticle;
			rParticles.removeIndex(i);
			i--;
		}
		else
		{
			rParticles[i]->Update(dt);
		}
	}
}

void SnowEmitter::EmitParticles(float rate, float snow_speed, Vector3D &angle)
{
	if(viewrenderer->GetView() == NULL)
		return;

	for(int i = 0;i < (int)rate;i++)
	{
		if(rParticles.numElem() >= snow_max_particles.GetInt())
			break;

		Vector3D forward;
		AngleVectors(angle, &forward);

		Vector3D viewOrigin = g_pViewEntity->GetEyeOrigin();

		SnowParticle *pNewParticle = new SnowParticle();
		pNewParticle->origin = viewOrigin + Vector3D(RandomFloat(-snow_radius.GetFloat(),snow_radius.GetFloat()), snow_start_h.GetFloat(), RandomFloat(-snow_radius.GetFloat(),snow_radius.GetFloat())) - Vector3D(forward.x*snow_wind_speed.GetFloat(),0,forward.z*snow_wind_speed.GetFloat());
		pNewParticle->velocity = Vector3D(forward.x*snow_wind_speed.GetFloat(),-fabs(RandomFloat(snow_speed / 2,snow_speed)),forward.z*snow_wind_speed.GetFloat());
		pNewParticle->emit = this;

		rParticles.append(pNewParticle);
	}
}