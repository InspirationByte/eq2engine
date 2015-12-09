//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera info entity
//////////////////////////////////////////////////////////////////////////////////

#ifndef INFOCAMERA_H
#define INFOCAMERA_H

#include "BaseEngineHeader.h"

class CInfoCamera : public BaseEntity
{
	// always do this
	typedef BaseEntity BaseClass;

public:

	CInfoCamera();

	void		Spawn();
	void		CalcView(Vector3D &origin, Vector3D &angles, float &fov);

protected:
	float		m_fFov;
	bool		m_bSetView;

	DECLARE_DATAMAP()
};

#endif // INFOCAMERA_H