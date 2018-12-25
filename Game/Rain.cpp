//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rain emitter
//////////////////////////////////////////////////////////////////////////////////

#include "Rain.h"

#include "BaseEngineHeader.h"
#include "GameRenderer.h"
#include "EngineEntities.h"
#include "Effects.h"

DECLARE_EFFECT_GROUP(Rain)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/rain");
	PRECACHE_PARTICLE_MATERIAL( "effects/rain_ripple");
	PRECACHE_PARTICLE_MATERIAL( "effects/rain_drops");
}

DECLARE_CVAR(rain_radius,800,"Rain radius",0);
DECLARE_CVAR(rain_start_h,900,"Rain start height",0);
DECLARE_CVAR(rain_max_particles,700,"Max rain particles",0);
DECLARE_CVAR(rain_length,170,"Rain length",0);
DECLARE_CVAR(rain_width,0.4,"Rain width",0);
DECLARE_CVAR(rain_wind_x,1000,"Rain width",0);
DECLARE_CVAR(rain_wind_z,2500,"Rain width",0);

void RainParticle::Update(float dt)
{
	ColorRGB lighting = ComputeLightingForPoint(origin, false);

	FX_DrawTracer(origin, velocity, rain_width.GetFloat(), rain_length.GetFloat(), Vector4D(lighting,1), EFFECT_GROUP_ID(Rain, 0));

	origin += velocity * dt;
}

void RainEmitter::Init()
{
	numRipples = 0;

	memset(m_pRipples, 0, sizeof(m_pRipples));
}

extern CWorldInfo* g_pWorldInfo;
/* REWRITE_GAME_EFFECTS
bool CRippleEffect::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if(m_fLifeTime <= 0)
		return false;

	float lifeTimePerc = GetLifetimePercent();

	fCurSize = lerp(fStartSize, fEndSize, 1.0f - lifeTimePerc);

	ColorRGB lighting = ComputeLightingForPoint(m_vOrigin, false);

	Vector4D col = ColorRGBA(lighting,lifeTimePerc*2);

	PFXBillboard_t effect;

	effect.vOrigin = m_vOrigin+Vector3D(0,(1.0f - lifeTimePerc)*5.0f,0);
	effect.vColor = col;
	effect.nGroupIndex = drop_material;
	effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
	effect.fZAngle = lifeTimePerc*150.0f;

	effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
	effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

	GR_DrawBillboard(&effect);

	
	PFXBillboard_t effect2;

	effect2.vOrigin = m_vOrigin+Vector3D(0,(1.0f - lifeTimePerc)*17.0f,0);
	effect2.vColor = col;
	effect2.nGroupIndex = m_nMaterialGroup;
	effect2.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
	effect2.fZAngle = lifeTimePerc*-50.0f;

	effect2.fWide = lerp(fStartSize, fEndSize*8, 1.0f-lifeTimePerc);
	effect2.fTall = lerp(fStartSize, fEndSize*8, 1.0f-lifeTimePerc);

	GR_DrawBillboard(&effect2);
	

	SetSortOrigin(m_vOrigin);

	return true;
}
*/

void RainEmitter::Update_Draw(float dt, float emit_rate, float rain_speed)
{
	if(!viewrenderer->GetView())
		return;

	Vector3D viewOrigin = g_pViewEntity->GetEyeOrigin();

	float startH = rain_start_h.GetFloat()+viewOrigin.y;

	EmitParticles(emit_rate, rain_speed);
	for( int i = 0; i < rParticles.numElem();i++)
	{
		RainParticle* pParticle = rParticles[i];

		float height = startH - rParticles[i]->origin.y;

		internaltrace_t rain_trace;
		Vector3D trace_start(pParticle->origin.x,startH,pParticle->origin.z);
		physics->InternalTraceLine(trace_start,trace_start - Vector3D(0,height+rain_length.GetFloat(),0),COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS,&rain_trace);

		const Volume& pViewFrustum = viewrenderer->GetViewFrustum();

		if(pParticle->origin.y < (viewOrigin.y - 800) || rain_trace.fraction < 1.0)
		{
			float fSpeed = length(rParticles[i]->velocity)*0.001f;

			if(rain_trace.fraction < 1.0f && pViewFrustum.IsPointInside(rain_trace.traceEnd))
			{
				MakeRipple(rain_trace.traceEnd + rain_trace.normal*0.5f, rain_trace.normal, 2+RandomFloat(-0.7f, 0.7f)*fSpeed,4+RandomFloat(-0.7f, 0.7f)*fSpeed,0.08f);
			}

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

void RainEmitter::MakeRipple(Vector3D &origin, Vector3D &normal, float startsize, float endsize, float lifetime)
{
/* REWRITE_GAME_EFFECTS
	CRippleEffect* pEffect = new CRippleEffect(origin, normal, startsize, endsize, lifetime, EFFECT_GROUP_ID( Rain, 1),  EFFECT_GROUP_ID( Rain, 2));

	effectrenderer->RegisterEffectForRender(pEffect);
	*/
}

void RainEmitter::RemoveRipple(int index)
{
	if ( index >= numRipples || index < 0 )
		return;

	ripple_t *ripple = m_pRipples[index];
	numRipples--;

	if ( numRipples > 0 && index != numRipples )
	{
		m_pRipples[index] = m_pRipples[numRipples];
		m_pRipples[numRipples] = NULL;
	}

	delete ripple;
}

void RainEmitter::EmitParticles(float rate, float rain_speed)
{
	if(!g_pViewEntity)
		return;

	for(int i = 0;i < (int)rate;i++)
	{
		if(rParticles.numElem() >= rain_max_particles.GetInt())
			break;

		Vector3D viewOrigin = g_pViewEntity->GetEyeOrigin();

		RainParticle *pNewParticle = new RainParticle();
		pNewParticle->origin = viewOrigin + Vector3D(RandomFloat(-rain_radius.GetFloat(),rain_radius.GetFloat()), rain_start_h.GetFloat(), RandomFloat(-rain_radius.GetFloat(),rain_radius.GetFloat()));
		
		
		float fRainDirX = sin(gpGlobals->curtime*0.02f)*rain_wind_x.GetFloat();
		float fRainDirZ = sin(gpGlobals->curtime*0.026f)+cos(gpGlobals->curtime*0.036f)*rain_wind_z.GetFloat();

		pNewParticle->velocity = Vector3D(RandomFloat(fRainDirX*0.5f, fRainDirX),-fabs(RandomFloat(rain_speed *0.5,rain_speed)),RandomFloat(fRainDirZ*0.5f, fRainDirZ));
		
		
		pNewParticle->emit = this;

		rParticles.append(pNewParticle);
	}
}