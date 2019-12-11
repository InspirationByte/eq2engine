//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Particle effects
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "ParticleEffects.h"
#include "EqParticles.h"

extern CPFXAtlasGroup* g_vehicleEffects;
extern CPFXAtlasGroup* g_translParticles;
extern CPFXAtlasGroup* g_additPartcles;

//-----------------------------------------------------------------------------------------------
// Fleck particle
//-----------------------------------------------------------------------------------------------
CFleckEffect::CFleckEffect(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity,
							float StartSize,
							float lifetime,
							float rotation,
							const ColorRGB& color,
							CPFXAtlasGroup* group, TexAtlasEntry_t* entry)
{
	InternalInit(position, lifetime, group, entry);

	fStartSize = StartSize;

	m_vVelocity = velocity;

	rotate = rotation;

	nDraws = 0;

	m_vGravity = gravity;

	m_vLastColor = color * g_pGameWorld->m_info.ambientColor.xyz()*2.0f;

	m_group = group;
	m_entry = entry;
}

bool CFleckEffect::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if(m_fLifeTime <= 0)
		return false;

	m_vOrigin += m_vVelocity*dTime*1.25f;

	m_vVelocity += m_vGravity*dTime*1.25f;

	SetSortOrigin(m_vOrigin);

	float lifeTimePerc = GetLifetimePercent();

	PFXBillboard_t effect;

	effect.group = m_group;
	effect.tex = m_entry;

	effect.vOrigin = m_vOrigin;
	effect.vColor = Vector4D(m_vLastColor,lifeTimePerc*2);
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;

	rotate += dTime * 120.0f * lifeTimePerc;

	effect.fZAngle = rotate;

	//internaltrace_t tr;
	//physics->InternalTraceLine(m_vOrigin,m_vOrigin+normalize(m_vVelocity), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &tr);
	/*
	CollisionData_t coll;
	g_pPhysics->TestLine(m_vOrigin, m_vOrigin+normalize(m_vVelocity), coll);

	if(coll.fract < 1.0f)
	{
		m_vVelocity = reflect(m_vVelocity, coll.normal)*0.25f;
		m_fLifeTime -= dTime*8;

		effect.fZAngle = 0.0f;
	}*/

	effect.fWide = fStartSize;
	effect.fTall = fStartSize;

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);

	nDraws++;

	return true;
}

//-----------------------------------------------------------------------------------------------
// Smoke particle
//-----------------------------------------------------------------------------------------------
CSmokeEffect::CSmokeEffect(	const Vector3D &position, const Vector3D &velocity,
							float StartSize, float EndSize,
							float lifetime,
							CPFXAtlasGroup* group, TexAtlasEntry_t* entry,
							float rotation,
							const Vector3D &gravity,
							const Vector3D &color1, const Vector3D &color2, float alpha)
{
	InternalInit( position, lifetime, group, entry );

	fStartSize = StartSize;
	fEndSize = EndSize;

	m_vVelocity = velocity;

	rotate = rotation;

	m_vGravity = gravity;

	m_color1 = color1;
	m_color2 = color2;

	m_alpha = alpha;
}

bool CSmokeEffect::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if(m_fLifeTime <= 0)
		return false;

	m_vOrigin += m_vVelocity*dTime;

	ColorRGB lightColor(g_pGameWorld->m_info.ambientColor.xyz()+g_pGameWorld->m_info.sunColor.xyz());

	float lifeTimePerc = GetLifetimePercent();

	float fStartAlpha = clamp((1.0f-lifeTimePerc) * 50.0f, 0.0f, 1.0f);

	Vector3D col_lerp = lerp(m_color2, m_color1, lifeTimePerc);

	m_vVelocity += m_vGravity*dTime;

	SetSortOrigin(m_vOrigin);

	PFXBillboard_t effect;

	effect.vOrigin = m_vOrigin;
	effect.vColor = Vector4D(lightColor * col_lerp, lifeTimePerc*m_alpha*fStartAlpha);
	effect.group = m_atlGroup;
	effect.tex = m_atlEntry;
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = lifeTimePerc*rotate;

	effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
	effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

	// no frustum for now
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);

	return true;
}

//-----------------------------------------------------------------------------------------------
// Ripple particle
//-----------------------------------------------------------------------------------------------

CRippleEffect::CRippleEffect(	const Vector3D &position, float StartSize, float EndSize, float lifetime,
								CPFXAtlasGroup* group, TexAtlasEntry_t* entry,
								const Vector3D &color, const Vector3D& groundNormal, float rotation)
{
	InternalInit(position, lifetime, group, entry);

	normal = groundNormal;

	fStartSize = StartSize;
	fEndSize = EndSize;

	fRotation = rotation;

	m_color = color;
}

bool CRippleEffect::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if (m_fLifeTime <= 0)
		return false;

	ColorRGB lightColor(g_pGameWorld->m_info.ambientColor.xyz() + g_pGameWorld->m_info.sunColor.xyz());

	float lifeTimePerc = GetLifetimePercent();
	float fStartAlpha = clamp((1.0f - lifeTimePerc) * 50.0f, 0.0f, 1.0f);

	SetSortOrigin(m_vOrigin);

	PFXBillboard_t effect;

	effect.vOrigin = m_vOrigin;
	effect.vColor = Vector4D(lightColor * m_color, lifeTimePerc*fStartAlpha);
	effect.group = m_atlGroup;
	effect.tex = m_atlEntry;
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = fRotation;

	effect.fWide = lerp(fStartSize, fEndSize, 1.0f - lifeTimePerc);
	effect.fTall = lerp(fStartSize, fEndSize, 1.0f - lifeTimePerc);

	// no frustum for now
	Effects_DrawBillboard(&effect, Matrix3x3(-cross(vec3_right, normal), -cross(vec3_forward, normal), normal), nullptr);

	return true;
}

//-----------------------------------------------------------------------------------------------
// Spark particle
//-----------------------------------------------------------------------------------------------
CSparkLine::CSparkLine(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity,
						float length, float StartSize, float EndSize,
						float lifetime,
						CPFXAtlasGroup* group, TexAtlasEntry_t* entry)
{
	InternalInit(position, lifetime, group, entry);

	fCurSize = StartSize;
	fStartSize = StartSize;
	fEndSize = EndSize;

	m_vVelocity = velocity;

	fLength = length;

	vGravity = gravity;
}

bool CSparkLine::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if(m_fLifeTime <= 0)
		return false;

	PFXVertex_t* verts;
	if(m_atlGroup->AllocateGeom( 4, 4, &verts, NULL, true ) < 0)
		return false;

	m_vOrigin += m_vVelocity*dTime*2.0f;

	SetSortOrigin(m_vOrigin);

	float lifeTimePerc = GetLifetimePercent();

	Vector3D viewDir = fastNormalize(m_vOrigin - effectrenderer->GetViewSortPosition());
	Vector3D lineDir = m_vVelocity;

	Vector3D vEnd = m_vOrigin + fastNormalize(m_vVelocity)*(fLength*length(m_vVelocity)*0.001f);

	Vector3D ccross = fastNormalize(cross(lineDir, viewDir));

	float scale = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

	Vector3D temp;

	VectorMA( vEnd, scale, ccross, temp );

	Vector4D color(Vector3D(1)*lifeTimePerc, 1);

	verts[0].point = temp;
	verts[0].texcoord = m_atlEntry->rect.GetLeftTop();
	verts[0].color = color;

	VectorMA( vEnd, -scale, ccross, temp );

	verts[1].point = temp;
	verts[1].texcoord = m_atlEntry->rect.GetLeftBottom();
	verts[1].color = color;

	VectorMA( m_vOrigin, scale, ccross, temp );

	verts[2].point = temp;
	verts[2].texcoord = m_atlEntry->rect.GetRightTop();
	verts[2].color = color;

	VectorMA( m_vOrigin, -scale, ccross, temp );

	verts[3].point = temp;
	verts[3].texcoord = m_atlEntry->rect.GetRightBottom();
	verts[3].color = color;

	CollisionData_t coll;
	g_pPhysics->TestLine(m_vOrigin, vEnd, coll);

	m_vVelocity += vGravity*dTime*2.0f;

	if(coll.fract < 1.0f)
	{
		m_vVelocity = reflect(m_vVelocity, coll.normal)*0.8f;
		m_fLifeTime -= dTime*2;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------
// Particle particle
//-----------------------------------------------------------------------------------------------
CParticleLine::CParticleLine(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity, 
							float StartSize, float EndSize, 
							float lifetime, 
							float lengthScale,
							CPFXAtlasGroup* group, TexAtlasEntry_t* entry,
							const Vector3D &color, float alpha)
{
	InternalInit(position, lifetime, group, entry);

	fCurSize = StartSize;
	fStartSize = StartSize;
	fEndSize = EndSize;

	m_vVelocity = velocity;

	vGravity = gravity;

	m_color = color;
	m_alpha = alpha;

	fLengthScale = lengthScale;
}

bool CParticleLine::DrawEffect(float dTime)
{
	m_fLifeTime -= dTime;

	if(m_fLifeTime <= 0)
		return false;

	PFXVertex_t* verts;
	if(m_atlGroup->AllocateGeom( 4, 4, &verts, NULL, true ) < 0)
		return false;

	m_vOrigin += m_vVelocity*dTime*2.0f;

	SetSortOrigin(m_vOrigin);

	float lifeTimePerc = GetLifetimePercent();

	Vector3D viewDir = fastNormalize(m_vOrigin - effectrenderer->GetViewSortPosition());
	Vector3D lineDir = m_vVelocity;

	float partLength = fEndSize*length(lineDir)*fLengthScale;
	partLength = max(fEndSize, partLength);

	Vector3D vEnd = m_vOrigin + fastNormalize(lineDir)*partLength;

	Vector3D ccross = fastNormalize(cross(lineDir, viewDir));

	float scale = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

	Vector3D temp;
	VectorMA( vEnd, scale, ccross, temp );

	ColorRGB lightColor(g_pGameWorld->m_info.ambientColor.xyz()+g_pGameWorld->m_info.sunColor.xyz());

	ColorRGBA color(m_color*lightColor, pow(m_alpha*lifeTimePerc, 0.5f));

	verts[0].point = temp;
	verts[0].texcoord = m_atlEntry->rect.GetLeftTop();
	verts[0].color = color;

	VectorMA( vEnd, -scale, ccross, temp );

	verts[1].point = temp;
	verts[1].texcoord = m_atlEntry->rect.GetLeftBottom();
	verts[1].color = color;

	VectorMA( m_vOrigin, scale, ccross, temp );

	verts[2].point = temp;
	verts[2].texcoord = m_atlEntry->rect.GetRightTop();
	verts[2].color = color;

	VectorMA( m_vOrigin, -scale, ccross, temp );

	verts[3].point = temp;
	verts[3].texcoord = m_atlEntry->rect.GetRightBottom();
	verts[3].color = color;

	m_vVelocity += vGravity*dTime*2.0f;

	return true;
}


//--------------------------------------------------------------------------------------------

void MakeSparks(const Vector3D& origin, const Vector3D& velocity, const Vector3D& randomAngCone, float lifetime, int count)
{
	CPFXAtlasGroup* effgroup = g_additPartcles;
	TexAtlasEntry_t* entry = effgroup->FindEntry("tracer");

	float wlen = length(velocity);

	for(int i = 0; i < count; i++)
	{
		Vector3D rnd_ang = VectorAngles(normalize(velocity)) + Vector3D(RandomFloat(-randomAngCone.x,randomAngCone.x),RandomFloat(-randomAngCone.y,randomAngCone.y),RandomFloat(-randomAngCone.z,randomAngCone.z));
		Vector3D n;
		AngleVectors(rnd_ang, &n);

		float rwlen = wlen + RandomFloat(wlen*0.15f, wlen*0.8f);

		CSparkLine* pSpark = new CSparkLine(origin,
											n*rwlen*0.25f,	// velocity
											Vector3D(0.0f,RandomFloat(0.4f, -15.0f), 0.0f),		// gravity
											RandomFloat(50.8, 80.0), // len
											RandomFloat(0.005f, 0.01f), RandomFloat(0.01f, 0.02f), // sizes
											RandomFloat(lifetime*0.75f, lifetime*1.25f),// lifetime
											effgroup, entry);  // group - texture
		effectrenderer->RegisterEffectForRender(pSpark);
	}
}

void MakeWaterSplash(const Vector3D& origin, const Vector3D& velocity, const Vector3D& randomAngCone, float lifetime, int count, float particleScale)
{
	CPFXAtlasGroup* effgroup = g_translParticles;
	TexAtlasEntry_t* entry = g_worldGlobals.trans_raindrops;

	for(int i = 0; i < count; i++)
	{
		Vector3D rnd_ang(RandomFloat(-randomAngCone.x, randomAngCone.x),
						RandomFloat(-randomAngCone.y, randomAngCone.y),
						RandomFloat(-randomAngCone.z, randomAngCone.z));

		rnd_ang = VDEG2RAD(rnd_ang);

		Vector3D rndVelocity = (rotateVector(velocity, Quaternion(rnd_ang.x, rnd_ang.y, rnd_ang.z))) * RandomFloat(0.35f, 1.0f);

		Vector3D rndPos(RandomFloat(-0.5f, 0.5f), RandomFloat(-0.5f, 0.5f), RandomFloat(-0.5f, 0.5f));
		rndPos += origin;

		float startSize = RandomFloat(0.5f, 1.8f) * particleScale;
		float endSize = RandomFloat(2.5f, 3.0f) * particleScale;
		float lineEndSize = RandomFloat(1.0f, 1.5f) * particleScale;

		float lifeTime = RandomFloat(lifetime*0.5f, lifetime);
		float lineLifeTime = RandomFloat(lifetime*0.5f, lifetime) * 0.35f;
		
		CSmokeEffect* pSmoke = new CSmokeEffect(rndPos, rndVelocity,
			startSize, endSize,
			RandomFloat(lifetime*0.5f, lifetime),
			effgroup, entry,
			RandomFloat(5, 35), Vector3D(0,RandomFloat(-6.0, -10.0) , 0),
			ColorRGB(1), ColorRGB(1));

		effectrenderer->RegisterEffectForRender(pSmoke);
		
		CParticleLine* pSpark = new CParticleLine(rndPos,
											rndVelocity*0.8f,	// velocity
											Vector3D(0.0f,RandomFloat(-2.5f, -8.0f), 0.0f),		// gravity
											startSize, lineEndSize, // sizes
											lineLifeTime,// lifetime
											0.5f,
											effgroup, entry, ColorRGB(1.0f), 1.0f);  // group - texture
		effectrenderer->RegisterEffectForRender(pSpark);
	}
}

//--------------------------------------------------------------------------------------------------------------

//
// Lights
//
void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type)
{
	PFXBillboard_t effect;

	effect.vColor = color;

	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = g_pGameWorld->m_view.GetAngles().y;

	effect.group = g_additPartcles;

	effect.fWide = size;
	effect.fTall = size;

	effect.vOrigin = position;

	if(type == 0)
		effect.tex = g_additPartcles->FindEntry("light1");
	else if(type == 1)
		effect.tex = g_additPartcles->FindEntry("glow2");
	else if(type == 2)
		effect.tex = g_additPartcles->FindEntry("light1a");
	else if(type == 3)
		effect.tex = g_additPartcles->FindEntry("glow1");

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);
}

//
// Police siren flashing effects
//
void PoliceSirenEffect(float fCurTime, const ColorRGB& color, const Vector3D& pos, const Vector3D& dir_right, float rDist, float width)
{
	const float FLASH_MIN_SIZE = 0.025f;
	const float FLASH_MAX_SIZE = 0.55f;
	const float GLOW_MAX_SIZE = 1.05f;
	const float MID_MAX_SIZE = 0.20f;

	const float FLASHING_SPEED = 21.0f;

	PFXBillboard_t effect;
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = g_pGameWorld->m_view.GetAngles().y;
	effect.group = g_additPartcles;

	float leftFactor = sinf(fCurTime*FLASHING_SPEED);
	float rightFactor = sinf(fCurTime*FLASHING_SPEED + PI_F);
	float midFactor = sinf(fCurTime*FLASHING_SPEED + PI_F * 0.5f);

	float leftFlashSize = lerp(FLASH_MIN_SIZE, FLASH_MAX_SIZE, fabs(leftFactor));
	float leftGlowSize = lerp(FLASH_MIN_SIZE, GLOW_MAX_SIZE, fabs(leftFactor));

	float rightFlashSize = lerp(FLASH_MIN_SIZE, FLASH_MAX_SIZE, fabs(rightFactor));
	float rightGlowSize = lerp(FLASH_MIN_SIZE, GLOW_MAX_SIZE, fabs(rightFactor));

	float midGlowSize = lerp(FLASH_MIN_SIZE, MID_MAX_SIZE, fabs(midFactor));

	TexAtlasEntry_t* light1 = g_additPartcles->FindEntry("light1");
	TexAtlasEntry_t* light1a = g_additPartcles->FindEntry("light1a");
	TexAtlasEntry_t* glow1 = g_additPartcles->FindEntry("glow1");

	//---------------------------------
	// setup left
	effect.vColor = Vector4D(color * leftFactor, 1.0f);
	effect.vOrigin = pos + dir_right * rDist - dir_right * width;

	// draw left flash
	effect.fWide = effect.fTall = leftFlashSize;
	effect.tex = light1;
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);

	// draw left glow
	effect.fWide = effect.fTall = leftGlowSize;
	effect.tex = glow1;
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);

	//---------------------------------
	// setup right
	effect.vColor = Vector4D(color * rightFactor, 1.0f);
	effect.vOrigin = pos + dir_right * rDist + dir_right * width;

	// draw right flash
	effect.fWide = effect.fTall = rightFlashSize;
	effect.tex = light1a;
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);

	// draw right glow
	effect.fWide = effect.fTall = rightGlowSize;
	effect.tex = glow1;
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);

	//---------------------------------
	// setup mid
	effect.vColor = Vector4D(color * midFactor * 2.0f, 1.0f);
	effect.vOrigin = pos + dir_right * rDist;

	// draw mid glow
	effect.fWide = effect.fTall = midGlowSize;
	effect.tex = glow1;
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_view, NULL);
}

//-----------------------------------------------------------------------------