//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera info entity
//////////////////////////////////////////////////////////////////////////////////

#include "InfoCamera.h"
#include "GameRenderer.h"

CInfoCamera::CInfoCamera()
{
	m_fFov = 90.0f;
	m_bSetView = false;
}

void CInfoCamera::Spawn()
{
	if(m_bSetView)
	{
		GR_SetViewEntity( this );
	}
}

void CInfoCamera::CalcView(Vector3D &origin, Vector3D &angles, float &fov)
{
	origin = m_vecAbsOrigin;
	angles = m_vecAbsAngles;
	fov = m_fFov;
}

// declare save data
BEGIN_DATAMAP(CInfoCamera)

	DEFINE_KEYFIELD( m_bSetView,	"enable", VTYPE_BOOLEAN ),
	DEFINE_KEYFIELD( m_fFov,		"fov", VTYPE_FLOAT ),

	//DEFINE_INPUTFUNC( "Toggle", Toggle )

END_DATAMAP()

DECLARE_ENTITYFACTORY(info_camera, CInfoCamera)