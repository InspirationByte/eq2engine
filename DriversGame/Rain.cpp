//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rain emitter
//////////////////////////////////////////////////////////////////////////////////

#include "Rain.h"
#include "world.h"

extern CPFXAtlasGroup* g_translParticles;

static RainEmitter s_RainEmitter;
RainEmitter* g_pRainEmitter = &s_RainEmitter;

//---------------------------------------------------------------------------------------------------------

bool CRippleEffect::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if(m_fLifeTime <= 0)
		return false;

	float lifeTimePerc = GetLifetimePercent();

	fCurSize = lerp(fStartSize, fEndSize, 1.0f - lifeTimePerc);

	ColorRGB lighting = g_pGameWorld->m_envConfig.rainBrightness;

	Vector4D col = ColorRGBA(lighting,lifeTimePerc*2.0f);
	/*
	PFXBillboard_t effect;

	effect.vOrigin = m_vOrigin+Vector3D(0,(1.0f - lifeTimePerc)*5.0f,0);
	effect.vColor = col;
	effect.group = g_translParticles;
	effect.tex = emitter->m_rippleEntry;
	//effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
	effect.fZAngle = lifeTimePerc*150.0f;

	effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
	effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, &g_pGameWorld->m_frustum);
	*/
	
	PFXBillboard_t effect2;

	effect2.vOrigin = m_vOrigin+Vector3D(0,(1.0f - lifeTimePerc)*0.1f,0);
	effect2.vColor = col;
	effect2.group = g_translParticles;
	effect2.tex = emitter->m_rippleEntry;
	//effect2.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
	effect2.fZAngle = lifeTimePerc*-50.0f;

	effect2.fWide = lerp(fStartSize, fEndSize*4.0f, 1.0f-lifeTimePerc);
	effect2.fTall = lerp(fStartSize, fEndSize*4.0f, 1.0f-lifeTimePerc);

	Effects_DrawBillboard(&effect2, g_pGameWorld->m_matrices[MATRIXMODE_VIEW], &g_pGameWorld->m_frustum);
	
	SetSortOrigin(m_vOrigin);

	return true;
}


DECLARE_CVAR(rain_radius,24,"Rain radius",0);
DECLARE_CVAR(rain_start_h,30,"Rain start height",0);
DECLARE_CVAR(rain_max_particles,1500,"Max rain particles",0);
DECLARE_CVAR(rain_length,0.8,"Rain length",0);
DECLARE_CVAR(rain_width,0.02,"Rain width",0);
DECLARE_CVAR(rain_wind_x,90,"Rain wind",0);
DECLARE_CVAR(rain_wind_z,90,"Rain wind",0);
DECLARE_CVAR(rain_viewVelocityScale,10.0,"Rain view velocity scale",0);
DECLARE_CVAR(rain_viewVelocityOfsScale,2.5f,"Rain view velocity offset scale",0);

DECLARE_CVAR(rain_advancedeffects,0,"Rain advanced effects (slow)",CV_ARCHIVE);

//---------------------------------------------------------------------------------------------------------

void FX_DrawRainTracer(Vector3D &origin, Vector3D &line_dir, float width, float length, ColorRGBA &color, TexAtlasEntry_t* atlEntry)
{
	PFXVertex_t* verts;
	if( g_translParticles->AllocateGeom( 4, 4, &verts, NULL, true ) < 0 )
		return;

	Vector3D viewDir = origin - g_pGameWorld->m_CameraParams.GetOrigin();
	Vector3D lineDir = line_dir;

	Vector3D ccross = fastNormalize(cross(lineDir, viewDir));

	Vector3D vEnd = origin + fastNormalize(line_dir)*length;

	Vector3D temp;

	VectorMA( vEnd, width, ccross, temp );

	verts[0].point = temp;
	verts[0].texcoord = atlEntry->rect.GetLeftTop();
	verts[0].color = color;

	VectorMA( vEnd, -width, ccross, temp );

	verts[1].point = temp;
	verts[1].texcoord = atlEntry->rect.GetLeftBottom();
	verts[1].color = color;

	VectorMA( origin, width, ccross, temp );

	verts[2].point = temp;
	verts[2].texcoord = atlEntry->rect.GetRightTop();
	verts[2].color = color;

	VectorMA( origin, -width, ccross, temp );

	verts[3].point = temp;
	verts[3].texcoord = atlEntry->rect.GetRightBottom();
	verts[3].color = color;

}

void RainParticle::Update(float dt)
{
	ColorRGB lighting = g_pGameWorld->m_envConfig.rainBrightness;

	//TexAtlasEntry_t* rain = g_translParticles->FindAtlasTexture();

	FX_DrawRainTracer(origin, velocity, rain_width.GetFloat(), rain_length.GetFloat(), Vector4D(lighting,1), g_pRainEmitter->m_rainEntry);

	origin += velocity * dt;
}

void RainEmitter::Init()
{
	m_curTime = 0.0f;

	m_rainEntry = g_translParticles->FindEntry("rain");
	m_rippleEntry = g_translParticles->FindEntry("rain_ripple");
	m_viewVelocity = vec3_zero;
}

void RainEmitter::Clear()
{
	for( int i = 0; i < rParticles.numElem();i++)
	{
		RainParticle* pParticle = rParticles[i];
		delete pParticle;
	}

	rParticles.clear();
}

void RainEmitter::SetViewVelocity( const Vector3D& vel )
{
	m_viewVelocity = vel;
}

void RainEmitter::Update_Draw(float dt, float emit_rate, float rain_speed)
{
	Vector3D viewOrigin = g_pGameWorld->m_CameraParams.GetOrigin();

	Vector3D viewDir;
	AngleVectors(g_pGameWorld->m_CameraParams.GetAngles(),&viewDir);

	float startH = rain_start_h.GetFloat()+viewOrigin.y;
	
	EmitParticles(dt, emit_rate, rain_speed);

	eqPhysCollisionFilter collFilter;
	collFilter.flags = EQPHYS_FILTER_FLAG_DISALLOW_DYNAMIC;

	for( int i = 0; i < rParticles.numElem();i++)
	{
		RainParticle* pParticle = rParticles[i];

		float height = startH - rParticles[i]->origin.y;

		Vector3D trace_start(pParticle->origin.x,startH,pParticle->origin.z);

		CollisionData_t rain_trace;

		if(dot(viewDir, fastNormalize(trace_start-viewOrigin)) > 0)
		{
			g_pPhysics->TestLine(trace_start,trace_start - Vector3D(0,height,0), rain_trace, OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS, &collFilter);
		}
		
		if(dt > 0.0f && (pParticle->origin.y < (viewOrigin.y - 10.0f) || rain_trace.fract < 1.0f))
		{
			float fSpeed = length(rParticles[i]->velocity)*0.001f;
			
			if(rain_advancedeffects.GetBool() && rain_trace.fract < 1.0f)
			{
				CRippleEffect* pEffect = new CRippleEffect(	Vector3D(rain_trace.position), 
															rain_trace.normal,
															0.04f+RandomFloat(-0.07f, 0.07f)*fSpeed, 0.04f+RandomFloat(-0.07f, 0.07f)*fSpeed, 0.08f,
															-1,-1);

				pEffect->emitter = this;

				effectrenderer->RegisterEffectForRender(pEffect);
			}

			delete pParticle;
			rParticles.fastRemoveIndex(i);
			i--;
		}
		else
		{
			rParticles[i]->Update(dt);
		}
	}
	
	m_curTime += dt;
}

void RainEmitter::EmitParticles(float dt, float rate, float rain_speed)
{
	for(int i = 0;i < (int)rate;i++)
	{
		if(rParticles.numElem() >= rain_max_particles.GetInt())
			break;

		Vector3D viewOrigin = g_pGameWorld->m_CameraParams.GetOrigin() + m_viewVelocity*rain_viewVelocityOfsScale.GetFloat();

		RainParticle *pNewParticle = new RainParticle();

		float fRainDirX = sin(m_curTime*0.02f)*rain_wind_x.GetFloat();
		float fRainDirZ = sin(m_curTime*0.026f)+cos(m_curTime*0.036f)*rain_wind_z.GetFloat();

		Vector3D rainVelWindRnd = Vector3D(RandomFloat(fRainDirX*0.5f, fRainDirX),0,RandomFloat(fRainDirZ*0.5f, fRainDirZ));

		pNewParticle->origin = (viewOrigin - rainVelWindRnd*0.25f) + Vector3D(RandomFloat(-rain_radius.GetFloat(),rain_radius.GetFloat()), rain_start_h.GetFloat(), RandomFloat(-rain_radius.GetFloat(),rain_radius.GetFloat()));

		pNewParticle->velocity = rainVelWindRnd + Vector3D(0,-abs(RandomFloat(rain_speed *0.5,rain_speed)),0) - m_viewVelocity*rain_viewVelocityScale.GetFloat();

		pNewParticle->emit = this;

		rParticles.append(pNewParticle);
	}
}