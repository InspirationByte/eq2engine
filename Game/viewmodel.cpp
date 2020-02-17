//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Weapon view model
//////////////////////////////////////////////////////////////////////////////////

#include "viewmodel.h"
#include "Player.h"

#include "Math/math_util.h"

BEGIN_DATAMAP(CBaseViewmodel)
	DEFINE_FIELD(m_vecLastFacing, VTYPE_VECTOR3D),
END_DATAMAP()

ConVar r_drawViewModels("r_drawViewModels","1", "Draw View models (guns, etc)", CV_CHEAT);

CBaseViewmodel::CBaseViewmodel()
{
	m_vecLastFacing	= Vector3D(0,0,1);

	m_fLandingYaw = 0.0f;
	m_fSmoothLandingYaw = 0.0f;

	m_vecLastAngles = vec3_zero;
	m_vecAngVelocity = vec3_zero;
	m_vecAddAngle = vec3_zero;
}

void CBaseViewmodel::OnPreRender()
{
	if(!m_pModel)
		return;

	if(!r_drawViewModels.GetBool())
		return;

	UpdateBones();
	g_viewModels->AddRenderable( this );
}

ConVar	g_sway_returnspeed( "v_sway_returnspeed","10" );
ConVar	g_sway_max( "v_sway_max","2" );
ConVar  g_sway_scale( "v_sway_scale", "3.0" );
ConVar  g_landingmax( "v_landingmax", "7.0" );

void CBaseViewmodel::CalcViewModelLag( Vector3D& origin, Vector3D& angles, Vector3D& original_angles )
{
	// compute landing yaw
	m_fLandingYaw -= gpGlobals->frametime * 30;
	if(m_fLandingYaw < 0.0f)
	{
		m_fLandingYaw = 0.0f;
	}

	m_fSmoothLandingYaw = lerp(m_fSmoothLandingYaw,m_fLandingYaw,gpGlobals->frametime * 5);

	// Calculate our drift
	Vector3D	forward;
	AngleVectors( angles, &forward, NULL, NULL );

	Vector3D vDifference = forward - m_vecLastFacing;
	
	if ( fabs( vDifference.x ) > g_sway_max.GetFloat() ||
		 fabs( vDifference.y ) > g_sway_max.GetFloat() ||
		 fabs( vDifference.z ) > g_sway_max.GetFloat() )
		m_vecLastFacing = forward;

	VectorMA( m_vecLastFacing, g_sway_returnspeed.GetFloat() * gpGlobals->frametime, vDifference, m_vecLastFacing );

	// Make sure it doesn't grow out of control!!!
	m_vecLastFacing = normalize(m_vecLastFacing);
	VectorMA( origin, g_sway_scale.GetFloat(),vDifference * -1.0f, origin );

	Vector3D right, up;
	AngleVectors( original_angles, &forward, &right, &up );

	float pitch = original_angles.x;
	if ( pitch > 180.0f )
		pitch -= 360.0f;
	else if ( pitch < -180.0f )
		pitch += 360.0f;

	Vector3D angles_diff = AnglesDiff(m_vecLastAngles, angles);

	Vector3D oldAngles = angles;

	m_vecAngVelocity += angles_diff * 0.08f;

	SpringFunction(m_vecAddAngle, m_vecAngVelocity, 400.0f, 12.0f, gpGlobals->frametime);

	angles += m_vecAddAngle;

	//angles.x -= angles_diff.x * 0.15f;
	//angles.y -= angles_diff.y * 0.15f;
	
	VectorMA( m_vecLastAngles, g_sway_returnspeed.GetFloat() * gpGlobals->frametime, angles_diff, m_vecLastAngles );

	origin.y -= m_fSmoothLandingYaw;

	VectorMA( origin, -pitch * 0.007f,	forward,	origin );
	VectorMA( origin, -pitch * 0.01f,		right,	origin );
	VectorMA( origin, -pitch * 0.01f,		up,		origin);
}

void CBaseViewmodel::AddLandingYaw(float fVal)
{
	m_fLandingYaw += fVal;

	if(m_fLandingYaw > g_landingmax.GetFloat())
		m_fLandingYaw = g_landingmax.GetFloat();
}

// updates parent transformation
void CBaseViewmodel::UpdateTransform()
{
	m_matWorldTransform = ComputeWorldMatrix(GetAbsOrigin(), GetAbsAngles(), Vector3D(1), true);
}

// renders model
void CBaseViewmodel::Render(int nViewRenderFlags)
{
	g_pShaderAPI->SetDepthRange( 0.0f, 0.1f );
	BaseClass::Render( nViewRenderFlags | VR_FLAG_ALWAYSVISIBLE );
	g_pShaderAPI->SetDepthRange( 0.0f, 1.0f );
}

void CBaseViewmodel::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
{
	BaseClass::HandleAnimatingEvent(nEvent, options);

	CWhiteCagePlayer* pPlayer = dynamic_cast<CWhiteCagePlayer*>(m_pOwner);

	if(!pPlayer)
		return;

	// events to weapon
	pPlayer->GetActiveWeapon()->HandleEvent(nEvent, options);
}

DECLARE_ENTITYFACTORY(viewmodel, CBaseViewmodel)
