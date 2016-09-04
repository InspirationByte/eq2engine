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

	effect.fZAngle = lifeTimePerc*rotate;

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

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);

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

	fCurSize = StartSize;
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

	m_vCurrColor = (g_pGameWorld->m_info.ambientColor.xyz()+g_pGameWorld->m_info.sunColor.xyz());

	float lifeTimePerc = GetLifetimePercent();

	float fStartAlpha = clamp((1.0f-lifeTimePerc) * 50.0f, 0.0f, 1.0f);

	Vector3D col_lerp = lerp(m_color2, m_color1, lifeTimePerc);

	m_vVelocity += m_vGravity*dTime;

	SetSortOrigin(m_vOrigin);

	PFXBillboard_t effect;

	effect.vOrigin = m_vOrigin;
	effect.vColor = Vector4D(m_vCurrColor * col_lerp, lifeTimePerc*m_alpha*fStartAlpha);
	effect.group = m_atlGroup;
	effect.tex = m_atlEntry;
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = lifeTimePerc*rotate;

	effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
	effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

	// no frustum for now
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);

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

//--------------------------------------------------------------------------------------------------------------

//
// Lights
//
void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type)
{
	PFXBillboard_t effect;

	effect.vColor = color;

	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = g_pGameWorld->m_CameraParams.GetAngles().y;

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

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
}

//
// Police siren flashing effects
//
void PoliceSirenEffect(float fCurTime, const ColorRGB& color, const Vector3D& pos, const Vector3D& dir_right, float rDist, float width)
{
	PFXBillboard_t effect;

	float fSinFactor = sinf(fCurTime*16.0f);

	effect.vColor = Vector4D(color * fSinFactor, 1.0f);
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = g_pGameWorld->m_CameraParams.GetAngles().y;

	effect.group = g_additPartcles;

	float min_size = 0.025f;

	float max_size = 0.55f;
	float max_glow_size = 1.05f;

	// TODO: trace particle visibility

	effect.fWide = lerp(min_size, max_size, fabs(fSinFactor));
	effect.fTall = lerp(min_size, max_size, fabs(fSinFactor));

	effect.vOrigin = pos + dir_right*rDist - dir_right*width;
	effect.tex = g_additPartcles->FindEntry("light1");

	// no frustum for now
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
	effect.tex = g_additPartcles->FindEntry("glow1");
	effect.fWide = lerp(min_size, max_glow_size, fabs(fSinFactor));
	effect.fTall = lerp(min_size, max_glow_size, fabs(fSinFactor));
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);

	float fCosFactor = sinf((fCurTime*16.0f)+PI_F);

	effect.vColor = Vector4D(color * fCosFactor, 1.0f);
	effect.fWide = lerp(min_size, max_size, fabs(fCosFactor));
	effect.fTall = lerp(min_size, max_size, fabs(fCosFactor));

	effect.vOrigin = pos + dir_right*rDist + dir_right*width;
	effect.tex = g_additPartcles->FindEntry("light1a");

	// no frustum for now
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
	effect.tex = g_additPartcles->FindEntry("glow1");
	effect.fWide = lerp(min_size, max_glow_size, fabs(fCosFactor));
	effect.fTall = lerp(min_size, max_glow_size, fabs(fCosFactor));

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
}

//-----------------------------------------------------------------------------
