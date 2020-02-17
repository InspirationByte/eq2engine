//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: View parameters.
// Used by GameDLL and Engine to determine renderable camera position, rotation and FOV//////////////////////////////////////////////////////////////////////////////////

#ifndef VIEWPARAMS_H
#define VIEWPARAMS_H

#include "math/DkMath.h"
#include "math/Rectangle.h"

class CViewParams
{
public:
					CViewParams();

					//Initializes the CViewParams. Angles and FOV in degrees
					CViewParams(const Vector3D &origin,const Vector3D &angles,float fFov);

public:
	const Vector3D&	GetOrigin() const;
	const Vector3D&	GetAngles() const;
	float			GetFOV() const;

	void			SetOrigin( const Vector3D &origin );
	void			SetAngles( const Vector3D &angles );

	void			SetFOV( float fov );

	float			GetLODScaledDistFrom(const Vector3D& position ) const;

	void			GetMatrices(Matrix4x4& proj, Matrix4x4& view, float width, float height, float zNear, float zFar, bool orthographic = false) const;
	void			GetMatricesOrtho(Matrix4x4& proj, Matrix4x4& view, Rectangle_t rect, float zNear, float zFar) const;

protected:

	Vector3D		m_vecOrigin;
	Vector3D		m_vecAngles;
	float			m_fFOV;
};

#endif //VIEWPARAMS_H
