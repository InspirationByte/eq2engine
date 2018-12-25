//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle and decal effects
//////////////////////////////////////////////////////////////////////////////////

#include "effects.h"
#include "GameRenderer.h"

DkList<IEffectCache*> g_pEffectCacheList;

struct particle_spark_t
{
	Vector3D position;
	Vector3D normal;
};

class CQuadEffect : public IEffect
{
public:
	CQuadEffect(Vector3D &position, Vector3D &normal, float StartSize, float EndSize, float lifetime, int nMaterial)
	{
		InternalInit(position, lifetime, nullptr, nullptr);

		Vector3D binorm, tang;
		VectorVectors(normal, tang, binorm);

		normal		= normal;
		binormal	= binorm;
		tangent		= tang;

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;


		rotation = 0;

		m_matindex = nMaterial;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		float lifeTimePerc = GetLifetimePercent();

		fCurSize = lerp(fStartSize, fEndSize, 1.0f - lifeTimePerc);

		ParticleVertex_t verts[4];

		Vector4D col = Vector4D(1)*lifeTimePerc;

		// top right
		verts[0].point = m_vOrigin + binormal*fCurSize + tangent*fCurSize;
		verts[0].texcoord = Vector2D(1,1);
		verts[0].color = col;

		// top left
		verts[1].point = m_vOrigin + binormal*fCurSize - tangent*fCurSize;
		verts[1].texcoord = Vector2D(1,0);
		verts[1].color = col;

		// bottom right
		verts[2].point = m_vOrigin - binormal*fCurSize + tangent*fCurSize;
		verts[2].texcoord = Vector2D(0,1);
		verts[2].color = col;

		// bottom left
		verts[3].point = m_vOrigin - binormal*fCurSize - tangent*fCurSize;
		verts[3].texcoord = Vector2D(0,0);
		verts[3].color = col;

		AddParticleQuad(verts, m_matindex);

		SetSortOrigin(m_vOrigin);

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;

	float		rotation;

	int			m_matindex;
};

class CFleckEffect : public IEffect
{
public:
	CFleckEffect(Vector3D &position, Vector3D &velocity, Vector3D &gravity, float StartSize, float lifetime, int nMaterial, float rotation)
	{
		InternalInit(position, lifetime, nullptr, nullptr);

		fStartSize = StartSize;

		m_vVelocity = velocity;

		rotate = rotation;

		nDraws = 0;

		m_vGravity = gravity;

		m_vLastColor = ComputeLightingForPoint(m_vOrigin, false);

		m_matindex = nMaterial;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime*1.25f;

		m_vVelocity += m_vGravity*dTime*1.25f;

		SetSortOrigin(m_vOrigin);

		if(nDraws > 5)
		{
			m_vLastColor = ComputeLightingForPoint(m_vOrigin, false);
			nDraws = 0;
		}

		float lifeTimePerc = GetLifetimePercent();

		PFXBillboard_t effect;

		effect.vOrigin = m_vOrigin;
		effect.vColor = Vector4D(m_vLastColor,lifeTimePerc*2);
		effect.nGroupIndex = m_matindex;
		effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;

		effect.fZAngle = lifeTimePerc*rotate;

		internaltrace_t tr;
		physics->InternalTraceLine(m_vOrigin,m_vOrigin+normalize(m_vVelocity), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &tr);

		if(tr.fraction < 1.0f)
		{
			m_vVelocity = reflect(m_vVelocity, tr.normal)*0.25f;
			m_fLifeTime -= dTime*8;

			effect.fZAngle = 0.0f;
		}

		effect.fWide = fStartSize;
		effect.fTall = fStartSize;

		GR_DrawBillboard(&effect);

		nDraws++;

		return true;
	}

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

	int			m_matindex;
};

// base engine effect group
DECLARE_EFFECT_GROUP(ConcreteDecals)
{
	// decals must be first of the first
	PRECACHE_DECAL_MATERIAL( "decals/bullets/decal_conc1");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/decal_conc2");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/decal_conc3");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/decal_conc4");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/decal_conc5");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/decal_conc6");
	PRECACHE_DECAL_MATERIAL( "decals/default_decal");

	PRECACHE_DECAL_MATERIAL( "decals/blood");
}

DECLARE_EFFECT_GROUP(MetalDecals)
{
	PRECACHE_DECAL_MATERIAL( "decals/bullets/metal/decal_metal1");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/metal/decal_metal2");
	PRECACHE_DECAL_MATERIAL( "decals/bullets/metal/decal_metal3");
}

DECLARE_EFFECT_GROUP(GlassDecals)
{
	PRECACHE_DECAL_MATERIAL( "decals/default_decal");
}

DECLARE_EFFECT_GROUP(WoodDecals)
{
	PRECACHE_DECAL_MATERIAL( "decals/default_decal");
}

DECLARE_EFFECT_GROUP(Smoke)
{
	PRECACHE_PARTICLE_MATERIAL( "dev/particle");
	PRECACHE_PARTICLE_MATERIAL( "effects/smoke_anim/smoke1");
	PRECACHE_PARTICLE_MATERIAL( "effects/smoke_anim/smoke2");
	PRECACHE_PARTICLE_MATERIAL( "effects/smoke_anim/smoke3");
	PRECACHE_PARTICLE_MATERIAL( "effects/smoke_anim/smoke4");
	PRECACHE_PARTICLE_MATERIAL( "effects/smoke_anim/smoke5");
	PRECACHE_PARTICLE_MATERIAL( "effects/smoke_anim/smoke6");
}

DECLARE_EFFECT_GROUP(MetalImpacts)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/yellowflare");
	PRECACHE_PARTICLE_MATERIAL( "effects/tracer");
}

DECLARE_EFFECT_GROUP(Blood)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/blood_tr");
	PRECACHE_PARTICLE_MATERIAL( "effects/blood");
	PRECACHE_PARTICLE_MATERIAL( "decals/blood");
}

DECLARE_EFFECT_GROUP(BloodDecals)
{
	PRECACHE_DECAL_MATERIAL( "decals/blood");
}

DECLARE_EFFECT_GROUP(ConcreteFlecks)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/fleck_conc1");
	PRECACHE_PARTICLE_MATERIAL( "effects/fleck_conc2");
}

DECLARE_EFFECT_GROUP(GlassFlecks)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/fleck_conc1");
	PRECACHE_PARTICLE_MATERIAL( "effects/fleck_conc2");
}

DECLARE_EFFECT_GROUP(WoodFlecks)
{
	PRECACHE_PARTICLE_MATERIAL( "effects/fleck_conc1");
	PRECACHE_PARTICLE_MATERIAL( "effects/fleck_conc2");
}

// always precache in needed sequence because depth buffering is not supported for various effects
void PrecacheEffects()
{
	for(int i = 0; i < g_pEffectCacheList.numElem(); i++)
	{
		g_pEffectCacheList[i]->Precache();
	}
}

void UnloadEffects()
{
	for(int i = 0; i < g_pEffectCacheList.numElem(); i++)
	{
		g_pEffectCacheList[i]->Unload();
	}
}

void FX_DrawTracer(Vector3D &origin, Vector3D &line_dir, float width, float length, ColorRGBA &color, int nMaterial)
{
	if(nMaterial == -1)
	{
		nMaterial = EFFECT_GROUP_ID( MetalImpacts, 1);
	}

	Vector3D viewDir = origin - g_pViewEntity->GetEyeOrigin();
	Vector3D lineDir = line_dir;

	Vector3D ccross = normalize(cross(lineDir, viewDir));

	Vector3D vEnd = origin + normalize(line_dir)*length;

	Vector3D temp;

	VectorMA( vEnd, width, ccross, temp );

	ParticleVertex_t verts[4];

	verts[0].point = temp;
	verts[0].texcoord = Vector2D(0,0);
	verts[0].color = color;

	VectorMA( vEnd, -width, ccross, temp );

	verts[1].point = temp;
	verts[1].texcoord = Vector2D(0,1);
	verts[1].color = color;

	VectorMA( origin, width, ccross, temp );

	verts[2].point = temp;
	verts[2].texcoord = Vector2D(1,0);
	verts[2].color = color;

	VectorMA( origin, -width, ccross, temp );

	verts[3].point = temp;
	verts[3].texcoord = Vector2D(1,1);
	verts[3].color = color;

	AddParticleQuad(verts, nMaterial);
}

//ConVar r_maxTempDecals("r_maxTempDecals", "1000", "Maximum of temp decals in game");

class CDecalEffect : public IEffect//, public CBaseRenderableObject
{
public:
	CDecalEffect(tempdecal_t* decal, Vector3D &sortPos, float lifetime, int matIndex)
	{
		InternalInit(sortPos, lifetime, nullptr, nullptr);
		m_pDecal = decal;
		m_matindex = matIndex;

		//g_transparentsList->AddRenderable( this );

		m_nDraws = 0;
	}

	void DestroyEffect()
	{
		delete m_pDecal;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_nDraws = 0;

		SetSortOrigin(m_vOrigin);

		if(viewrenderer->GetViewFrustum()->IsSphereInside(m_vOrigin, length(m_pDecal->bbox.GetSize())))
		{
			//firstIndex = GetParticleIndexCount(m_nMaterialGroup);
			AddDecalGeom( (eqlevelvertex_t*)m_pDecal->verts, (uint16*)m_pDecal->indices, m_pDecal->numVerts, m_pDecal->numIndices, m_matindex);
		}

		m_nDraws++;

		return true;
	}

protected:
	tempdecal_t* m_pDecal;

	Vector3D	m_vLastColor;

	int			m_matindex;

	int			m_nDraws;
};

ConVar r_impactsvisibledist("r_impactsvisibledist", "1600");

void UTIL_Impact( trace_t& tr, Vector3D &impactDirection )
{
	if(g_pViewEntity && length(tr.traceEnd - g_pViewEntity->GetEyeOrigin()) > r_impactsvisibledist.GetFloat())
		return;

	// REWRITE_GAME_EFFECTS
	if(tr.hitmaterial)
	{
		EmitSound_t rico;

		rico.fPitch = 1.0;
		rico.fRadiusMultiplier = 1.0f;
		rico.fVolume = 1.0f;
		rico.origin = tr.traceEnd;

		rico.name = tr.hitmaterial->bulletImpactSound;

		ses->EmitSound(&rico);

		if(tr.hitmaterial->surfaceword == 'C' || tr.hitmaterial->surfaceword == 'T') // concrete or tile
		{
			if(!tr.hitEnt)
			{
				int gID = EFFECT_GROUP_RANDOMMATERIAL_ID(ConcreteDecals);

				float fDecScale = 1.0f;

				IMaterial* pDecalMaterial = GetParticleMaterial(EFFECT_GROUP_ID(ConcreteDecals, gID));
				IMatVar* decal_scale = pDecalMaterial->GetMaterialVar("scale", "1.0f");

				fDecScale = decal_scale->GetFloat();

				decalmakeinfo_t decal;
				decal.normal = -tr.normal;
				decal.origin = tr.traceEnd - tr.normal*0.05f;
				decal.size = Vector3D(fDecScale);
				decal.material = pDecalMaterial;

				tempdecal_t* pDecal = eqlevel->MakeTempDecal(decal);
			
				if(pDecal)
				{
					CDecalEffect* pEffect = new CDecalEffect(pDecal, tr.traceEnd+tr.normal*2.0f, 60.0f, EFFECT_GROUP_ID(ConcreteDecals, gID));
					effectrenderer->RegisterEffectForRender(pEffect);
				}
			}
			
			for(int i = 0; i < 6; i++)
			{
				Vector3D rnd_ang = VectorAngles(tr.normal) + Vector3D(RandomFloat(-25,25),RandomFloat(-25,25),RandomFloat(-25,25));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CSmokeEffect* pSmokeEffect = new CSmokeEffect(tr.traceEnd+tr.normal*2.0f, n*RandomFloat(18,54), RandomFloat(0.8,3), RandomFloat(14,19), RandomFloat(0.45,0.75), EFFECT_GROUP_ID( Smoke, 0), RandomFloat(10,105), Vector3D(0,9,0));
				effectrenderer->RegisterEffectForRender(pSmokeEffect);
			}

			for(int i = 0; i < 4; i++)
			{
				CSmokeEffect* pEffect = new CSmokeEffect(tr.traceEnd+tr.normal*4.0f, tr.normal*RandomFloat(10,25), RandomFloat(0.8,3), RandomFloat(14,19), RandomFloat(0.95,1.55), EFFECT_GROUP_ID( Smoke, RandomInt(1,6)), RandomFloat(10,105));
				effectrenderer->RegisterEffectForRender(pEffect);
			}

			for(int i = 0; i < 15; i++)
			{
				Vector3D reflDir = reflect(impactDirection, tr.normal);

				Vector3D rnd_ang = VectorAngles(reflDir) + Vector3D(RandomFloat(-75,75),RandomFloat(-75,75),RandomFloat(-75,75));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CFleckEffect* pEffect = new CFleckEffect(tr.traceEnd, n*RandomFloat(10,140), Vector3D(0,RandomFloat(-400,-1000),0), RandomFloat(0.9f,1.7f),3.0f, EFFECT_GROUP_ID(ConcreteFlecks, RandomInt(0,1)), RandomFloat(200,360));
				effectrenderer->RegisterEffectForRender(pEffect);
			}
		}
		else if(tr.hitmaterial->surfaceword == 'S') // glass
		{
			if(!tr.hitEnt)
			{
				int gID = EFFECT_GROUP_RANDOMMATERIAL_ID(GlassDecals);

				float fDecScale = 1.0f;

				IMaterial* pDecalMaterial = GetParticleMaterial(EFFECT_GROUP_ID(GlassDecals, gID));
				IMatVar* decal_scale = pDecalMaterial->GetMaterialVar("scale", "1.0f");

				fDecScale = decal_scale->GetFloat();

				decalmakeinfo_t decal;
				decal.normal = -tr.normal;
				decal.origin = tr.traceEnd - tr.normal*0.05f;
				decal.size = Vector3D(fDecScale);
				decal.material = pDecalMaterial;

				tempdecal_t* pDecal = eqlevel->MakeTempDecal(decal);
				if(pDecal)
				{
					CDecalEffect* pEffect = new CDecalEffect(pDecal, tr.traceEnd+tr.normal*2.0f, 60.0f, EFFECT_GROUP_ID(GlassDecals, gID));
					effectrenderer->RegisterEffectForRender(pEffect);
				}
			}
			
			for(int i = 0; i < 3; i++)
			{
				Vector3D rnd_ang = VectorAngles(tr.normal) + Vector3D(RandomFloat(-45,45),RandomFloat(-45,45),RandomFloat(-45,45));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CSmokeEffect* pSmokeEffect = new CSmokeEffect(tr.traceEnd+tr.normal*2.0f, n*RandomFloat(8,14), RandomFloat(0.8,3), RandomFloat(14,19), RandomFloat(0.25,0.65), EFFECT_GROUP_ID( Smoke, 0), RandomFloat(10,105), Vector3D(0,9,0));
				effectrenderer->RegisterEffectForRender(pSmokeEffect);
			}

			for(int i = 0; i < 4; i++)
			{
				CSmokeEffect* pEffect = new CSmokeEffect(tr.traceEnd+tr.normal*4.0f, tr.normal*RandomFloat(10,25), RandomFloat(0.8,3), RandomFloat(14,19), RandomFloat(0.35,0.75), EFFECT_GROUP_ID( Smoke, RandomInt(1,6)), RandomFloat(10,105));
				effectrenderer->RegisterEffectForRender(pEffect);
			}

			for(int i = 0; i < 15; i++)
			{
				Vector3D reflDir = reflect(impactDirection, tr.normal);

				Vector3D rnd_ang = VectorAngles(reflDir) + Vector3D(RandomFloat(-75,75),RandomFloat(-75,75),RandomFloat(-75,75));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CFleckEffect* pEffect = new CFleckEffect(tr.traceEnd, n*RandomFloat(10,140), Vector3D(0,RandomFloat(-400,-1000),0), RandomFloat(0.2f,0.7f),3.0f, EFFECT_GROUP_ID(GlassFlecks, RandomInt(0,1)), RandomFloat(200,360));
				effectrenderer->RegisterEffectForRender(pEffect);
			}
		}
		else if(tr.hitmaterial->surfaceword == 'W') // wood
		{
			if(!tr.hitEnt)
			{
				int gID = EFFECT_GROUP_RANDOMMATERIAL_ID(WoodDecals);

				float fDecScale = 1.0f;

				IMaterial* pDecalMaterial = GetParticleMaterial(EFFECT_GROUP_ID(WoodDecals, gID));
				IMatVar* decal_scale = pDecalMaterial->GetMaterialVar("scale", "1.0f");

				fDecScale = decal_scale->GetFloat();

				decalmakeinfo_t decal;
				decal.normal = -tr.normal;
				decal.origin = tr.traceEnd - tr.normal*0.05f;
				decal.size = Vector3D(fDecScale);
				decal.material = pDecalMaterial;

				tempdecal_t* pDecal = eqlevel->MakeTempDecal(decal);
				if(pDecal)
				{
					CDecalEffect* pEffect = new CDecalEffect(pDecal, tr.traceEnd+tr.normal*2.0f, 60, EFFECT_GROUP_ID(WoodDecals, gID));
					effectrenderer->RegisterEffectForRender(pEffect);
				}
			}
			/*
			for(int i = 0; i < 15; i++)
			{
				Vector3D reflDir = reflect(impactDirection, tr.normal);

				Vector3D rnd_ang = VectorAngles(reflDir) + Vector3D(RandomFloat(-75,75),RandomFloat(-75,75),RandomFloat(-75,75));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CFleckEffect* pEffect = new CFleckEffect(tr.traceEnd, n*RandomFloat(10,140), Vector3D(0,RandomFloat(-400,-1000),0), RandomFloat(0.9f,1.7f),3.0f, EFFECT_GROUP_ID(WoodFlecks, RandomInt(0,1)), RandomFloat(200,360));
				effectrenderer->RegisterEffectForRender(pEffect);
			}*/
		}
		else if(tr.hitmaterial->surfaceword == 'M') // metal
		{
			dlight_t *light = viewrenderer->AllocLight();
			SetLightDefaults(light);

			light->position = tr.traceEnd + tr.normal*5.0f;
			light->color = Vector3D(0.9,0.4,0.25);
			light->intensity = 2;
			light->radius = 30;
			light->dietime = 0.1f;
			light->fadetime = 0.1f;
			light->nFlags = LFLAG_MISCLIGHT | LFLAG_NOSHADOWS;

			viewrenderer->AddLight(light);

			if(!tr.hitEnt)
			{
				int gID = EFFECT_GROUP_RANDOMMATERIAL_ID(MetalDecals);

				float fDecScale = 1.0f;

				IMaterial* pDecalMaterial = GetParticleMaterial(EFFECT_GROUP_ID(MetalDecals, gID));
				IMatVar* decal_scale = pDecalMaterial->GetMaterialVar("scale", "1.0f");

				fDecScale = decal_scale->GetFloat();

				decalmakeinfo_t decal;
				decal.normal = -tr.normal;
				decal.origin = tr.traceEnd - tr.normal*0.05f;
				decal.size = Vector3D(fDecScale);
				decal.material = pDecalMaterial;

				tempdecal_t* pDecal = eqlevel->MakeTempDecal(decal);
				if(pDecal)
				{
					CDecalEffect* pEffect = new CDecalEffect(pDecal, tr.traceEnd+tr.normal*2.0f, 60, EFFECT_GROUP_ID(MetalDecals, gID));
					effectrenderer->RegisterEffectForRender(pEffect);
				}
			}
			else
			{
				int gID = EFFECT_GROUP_RANDOMMATERIAL_ID(MetalDecals);

				float fDecScale = 1.0f;

				IMaterial* pDecalMaterial = GetParticleMaterial(EFFECT_GROUP_ID(MetalDecals, gID));
				IMatVar* decal_scale = pDecalMaterial->GetMaterialVar("scale", "1.0f");

				fDecScale = decal_scale->GetFloat();

				decalmakeinfo_t decal;
				decal.normal = -tr.normal;
				decal.origin = tr.traceEnd - tr.normal*0.05f;
				decal.size = Vector3D(fDecScale);
				decal.material = pDecalMaterial;

				// make decal on entity
				tr.hitEnt->MakeDecal( decal );
			}
			
			if(dot(tr.normal, -impactDirection) < 0.8f)
			{
				CQuadEffect* quad = new CQuadEffect(tr.traceEnd+tr.normal,tr.normal, RandomFloat(4.4f,7.9f), RandomFloat(16.4f,16.9f), RandomFloat(0.1f,0.2f),EFFECT_GROUP_ID( MetalImpacts, 0));
				effectrenderer->RegisterEffectForRender(quad);

				bool longlife = tr.normal.y < -0.5f;

				for(int i = 0; i < 15; i++)
				{
					Vector3D reflDir = reflect(impactDirection, tr.normal);

					Vector3D rnd_ang = VectorAngles(reflDir) + Vector3D(RandomFloat(-25,25),RandomFloat(-25,25),RandomFloat(-25,25));
					Vector3D n;
					AngleVectors(rnd_ang, &n);

					CSparkLine* effect = new CSparkLine(tr.traceEnd, n*RandomFloat(100,460), Vector3D(0,-300,0), 100, RandomFloat(0.2,0.4),RandomFloat(0.1,0.3),longlife ? 1.0f : 0.3f,EFFECT_GROUP_ID( MetalImpacts, 1));
					effectrenderer->RegisterEffectForRender(effect);
				}
			}
		}
		if(tr.hitmaterial->surfaceword == 'F') // flesh
		{
			for(int i = 0; i < 5; i++)
			{
				Vector3D rnd_ang = VectorAngles(tr.normal) + Vector3D(RandomFloat(-45,45),RandomFloat(-45,45),RandomFloat(-45,45));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CSmokeEffect* pSmokeEffect = new CSmokeEffect(tr.traceEnd+tr.normal, n*RandomFloat(10,22), RandomFloat(0.8,3), RandomFloat(14,19), RandomFloat(0.18,0.38), EFFECT_GROUP_ID( Blood, 1), RandomFloat(10,105), Vector3D(0,9,0));
				effectrenderer->RegisterEffectForRender(pSmokeEffect);
			}

			for(int i = 0; i < 15; i++)
			{
				Vector3D reflDir = reflect(impactDirection, tr.normal);

				Vector3D rnd_ang = VectorAngles(reflDir) + Vector3D(RandomFloat(-45,45),RandomFloat(-45,45),RandomFloat(-45,45));
				Vector3D n;
				AngleVectors(rnd_ang, &n);

				CSparkLine* effect = new CSparkLine(tr.traceEnd, n*RandomFloat(25,80), Vector3D(0,-200,0), 45, RandomFloat(0.7,1.2), RandomFloat(0.9,0.6), 0.9f, EFFECT_GROUP_ID( Blood, 0), false);
				effectrenderer->RegisterEffectForRender(effect);
			}
			
			IPhysicsObject* pObj = tr.hitEnt ? tr.hitEnt->PhysicsGetObject() : NULL;

			internaltrace_t decalTrace;
			physics->InternalTraceLine(tr.traceEnd, tr.traceEnd + impactDirection * 128.0f, COLLISION_GROUP_WORLD, &decalTrace, &pObj, 1);

			if(decalTrace.fraction < 1.0f)
			{
				float bloodSize = RandomFloat(25.0f, 75.0f) * decalTrace.fraction;

				decalmakeinfo_t decal;
				decal.normal = lerp(impactDirection, -decalTrace.normal, 0.5f);
				decal.origin = decalTrace.traceEnd - decalTrace.normal*0.05f;
				decal.size = Vector3D(bloodSize);
				decal.material = GetParticleMaterial(EFFECT_GROUP_ID(BloodDecals, 0));
				decal.flags = MAKEDECAL_FLAG_TEX_NORMAL | MAKEDECAL_FLAG_CLIPPING;

				decal.texRotation = RandomFloat(-90, 90);

				// try to create decal at far distance
				tempdecal_t* pDecal = eqlevel->MakeTempDecal(decal);
			
				if(pDecal)
				{
					CDecalEffect* pEffect = new CDecalEffect(pDecal, decalTrace.traceEnd+decalTrace.normal*2.0f, 60, EFFECT_GROUP_ID(BloodDecals, 0));
					effectrenderer->RegisterEffectForRender(pEffect);
				}
			}
		}
	}
}

void UTIL_ExplosionEffect(Vector3D &origin, Vector3D &direction, bool onGround)
{
	/* REWRITE_GAME_EFFECTS */

	int gID = 6;

	float fDecScale = 45.0f;

	IMaterial* pDecalMaterial = GetParticleMaterial(EFFECT_GROUP_ID(ConcreteDecals, gID));

	decalmakeinfo_t decal;
	decal.normal = direction;
	decal.origin = origin;
	decal.size = Vector3D(fDecScale);
	decal.material = pDecalMaterial;
	decal.flags = MAKEDECAL_FLAG_CLIPPING;

	tempdecal_t* pDecal = eqlevel->MakeTempDecal(decal);

	if(pDecal)
	{
		CDecalEffect* pEffect = new CDecalEffect(pDecal, decal.origin, 60, EFFECT_GROUP_ID(ConcreteDecals, gID));
		effectrenderer->RegisterEffectForRender(pEffect);
	}

	for(int i = 0; i < 3; i++)
	{
		Vector3D rnd_ang = VectorAngles(direction) + Vector3D(RandomFloat(-45,45),RandomFloat(-45,45),RandomFloat(-45,45));
		Vector3D n;
		AngleVectors(rnd_ang, &n);

		Vector3D random_vec(RandomFloat(-5,5),RandomFloat(0,20), RandomFloat(-5,5));

		CSmokeEffect* pSmokeEffect = new CSmokeEffect(origin+direction*float(i)*2.0f+random_vec*float(i), n*RandomFloat(18,44), RandomFloat(10.8,14), RandomFloat(94,129), RandomFloat(0.15,0.25), EFFECT_GROUP_ID( MetalImpacts, 0), RandomFloat(10,105), Vector3D(0,9,0), ColorRGB(1), ColorRGB(0,0,0));
		effectrenderer->RegisterEffectForRender(pSmokeEffect);
	}

	for(int i = 0; i < 3; i++)
	{
		Vector3D rnd_ang = VectorAngles(direction) + Vector3D(RandomFloat(-45,45),RandomFloat(-45,45),RandomFloat(-45,45));
		Vector3D n;
		AngleVectors(rnd_ang, &n);

		CSmokeEffect* pSmokeEffect = new CSmokeEffect(origin+direction*2.0f, n*RandomFloat(18,44), RandomFloat(0.8,3), RandomFloat(64,79), RandomFloat(0.85,1.25), EFFECT_GROUP_ID( Smoke, 0), RandomFloat(10,105), Vector3D(0,9,0), ColorRGB(0,0,0), ColorRGB(0,0,0));
		effectrenderer->RegisterEffectForRender(pSmokeEffect);
	}

	for(int i = 0; i < 4; i++)
	{
		CSmokeEffect* pEffect = new CSmokeEffect(origin+direction*4.0f, direction*RandomFloat(20,55), RandomFloat(30.8,39), RandomFloat(84,99), RandomFloat(1.95,2.55), EFFECT_GROUP_ID( Smoke, RandomInt(1,6)), RandomFloat(10,105), Vector3D(0,15,0));
		effectrenderer->RegisterEffectForRender(pEffect);
	}

	for(int i = 0; i < 35; i++)
	{
		Vector3D rnd_ang = VectorAngles(direction) + Vector3D(RandomFloat(-75,75),RandomFloat(-75,75),RandomFloat(-75,75));
		Vector3D n;
		AngleVectors(rnd_ang, &n);

		Vector3D random_vec(RandomFloat(-20,20),RandomFloat(0,20), RandomFloat(-20,20));

		CFleckEffect* pEffect = new CFleckEffect(origin+random_vec, n*RandomFloat(110,740), Vector3D(0,RandomFloat(-300,-800),0), RandomFloat(1.9f,4.7f),3.0f, EFFECT_GROUP_ID(ConcreteFlecks, RandomInt(0,1)), RandomFloat(200,360));
		effectrenderer->RegisterEffectForRender(pEffect);

		CSparkLine* effect = new CSparkLine(origin+random_vec, n*RandomFloat(110,740), Vector3D(0,-300,0), 50, RandomFloat(0.2,0.4),RandomFloat(0.1,0.3),0.9f,EFFECT_GROUP_ID( MetalImpacts, 1));
		effectrenderer->RegisterEffectForRender(effect);
	}

#define SMOKE_AROUND 20

	for(int i = 0; i < 18; i++)
	{
		Vector3D angle(RandomFloat(-10,10), i*SMOKE_AROUND+RandomFloat(-5,5), 0.0f);

		Vector3D dir;
		AngleVectors(angle, &dir);

		CSmokeEffect* pEffect = new CSmokeEffect(origin+dir*5.0f, dir*RandomFloat(180,200), RandomFloat(40,49), RandomFloat(94,119), RandomFloat(1.25,2.05), EFFECT_GROUP_ID( Smoke, 0), RandomFloat(10,105), Vector3D(0,15,0), ColorRGB(0,0,0), ColorRGB(0,0,0));
		effectrenderer->RegisterEffectForRender(pEffect);
	}
}

ConVar ph_contact_min_momentum("ph_contact_min_momentum", "60", "Minimum contact momentum for playing sound and making effects", CV_CHEAT);

void ObjectSoundEvents(IPhysicsObject* pObject, BaseEntity* pOwner)
{
	for (int i = 0; i < pObject->GetContactEventCount(); i++)
	{
		physicsContactEvent_t* contact = pObject->GetContactEvent(i);

		float fContactImpulse = contact->fImpulse;

		if(fContactImpulse < ph_contact_min_momentum.GetFloat())
			continue;

		//debugoverlay->Box3D(contact->vWorldHitOriginB-Vector3D(fContactImpulse), contact->vWorldHitOriginB+Vector3D(fContactImpulse), ColorRGBA(1,1,0,1), 3.0f);

		float fImpulse = contact->fImpulse;// * contact->fImpulse;//*0.1f;

		float fVolume = fImpulse * pObject->GetInvMass();//(1.0f/(1320.0f*1320.0f));

		if(fVolume > 1.0f)
			fVolume = 1.0f;

		/*
		if(contact->pHitB && contact->pHitB->GetEntityObjectPtr())
		{
			BaseEntity* pEnt = (BaseEntity*)contact->pHitB->GetEntityObjectPtr();

			//Msg("Damage to %s = %g\n", pEnt->GetClassname(), fContactImpulse);

			pEnt->ApplyDamage(contact->vWorldHitOriginA, normalize(pObject->GetVelocity()), contact->vWorldHitNormal, pOwner, fContactImpulse, fContactImpulse);
		}
		*/
		//Msg("Impact volume: %f\n", fVolume);

		// play sound, make effects
		if(pObject->GetMaterial())
		{
			EmitSound_t ep;
			if(fVolume > 0.5f)
			{
				ep.name = pObject->GetMaterial()->heavyImpactSound;
			}
			else
			{
				ep.name = pObject->GetMaterial()->lightImpactSound;
				fVolume += 0.45f;
			}

			ep.fPitch = 1.0;
			ep.fRadiusMultiplier = clamp(fVolume,0,1);
			ep.fVolume = clamp(fVolume,0,1);
			ep.origin = contact->vWorldHitOriginB - pOwner->GetAbsOrigin();

			ep.pObject = pOwner;

			pOwner->EmitSoundWithParams(&ep);
		}
	}
}