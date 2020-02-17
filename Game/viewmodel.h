//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Weapon view model
//////////////////////////////////////////////////////////////////////////////////

#ifndef VIEWMODEL_H
#define VIEWMODEL_H

#include "BaseWeapon.h"
#include "BaseEngineHeader.h"
#include "BaseAnimating.h"

class CBaseViewmodel : public BaseAnimating
{
	DEFINE_CLASS_BASE(BaseAnimating);
public:
	CBaseViewmodel();

	void			OnPreRender();

	// updates parent transformation
	void			UpdateTransform();

	void			CalcViewModelLag( Vector3D& origin, Vector3D& angles, Vector3D& original_angles );

	void			AddLandingYaw(float fVal);

	void			Render(int nViewRenderFlags);

	void			HandleAnimatingEvent(AnimationEvent nEvent, char* options);

protected:
	DECLARE_DATAMAP();
	Vector3D		m_vecLastFacing;
	Vector3D		m_vecLastAngles;

	Vector3D		m_vecAngVelocity;
	Vector3D		m_vecAddAngle;

	float			m_fLandingYaw;
	float			m_fSmoothLandingYaw;
};

#endif // VIEWMODEL_H