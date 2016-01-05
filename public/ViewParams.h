//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: View parameters.
// Used by GameDLL and Engine to determine renderable camera position, rotation and FOV//////////////////////////////////////////////////////////////////////////////////

#ifndef VIEWPARAMS_H
#define VIEWPARAMS_H

#include "math/DkMath.h"

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

	float			GetLODScaledDistFrom( const Vector3D& position );

	/*
	void			BuildViewMatrices(float fNear, float fFar, int width, int height, bool orthographic = false, bool cubemap = false);

	void			SetupMatrices();

	Matrix4x4		m_viewprojection;
	Matrix4x4		m_matrices[4];	*/

protected:

	Vector3D		m_vecOrigin;
	Vector3D		m_vecAngles;
	float			m_fFOV;

	//float			m_orthoScale;
	//int				m_cubemapFace;
};

#endif //VIEWPARAMS_H
