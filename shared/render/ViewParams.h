//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera settings
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr const float CAMERA_DEFAULT_ZNEAR = 1.0f;
static constexpr const float CAMERA_DEFAULT_FOV = 72.0f;

class CViewParams
{
public:
	CViewParams();

	//Initializes the CViewParams. Angles and FOV in degrees
	CViewParams(const Vector3D &origin,const Vector3D &angles,float fFov);

	const Vector3D&	GetOrigin() const;
	const Vector3D&	GetAngles() const;
	float			GetFOV() const;
	float			GetZNear() const;

	void			SetOrigin( const Vector3D &origin );
	void			SetAngles( const Vector3D &angles );

	void			SetFOV( float fFov );
	void			SetZNear( float fNear );

	float			GetLodScale() const;
	float			GetLODScaledDistFrom(const Vector3D& position ) const;

	void			GetMatrices(Matrix4x4& proj, Matrix4x4& view, float width, float height, float zNear, float zFar, bool orthographic = false) const;
	void			GetMatricesOrtho(Matrix4x4& proj, Matrix4x4& view, AARectangle rect, float zNear, float zFar) const;

	static void		Interpolate(const CViewParams& from, const CViewParams& to, float factor, CViewParams& out);
protected:

	Vector3D		m_vecOrigin{ 0 };
	Vector3D		m_vecAngles{ 0 };
	float			m_fFOV{ CAMERA_DEFAULT_FOV };
	float			m_fZNear{ CAMERA_DEFAULT_ZNEAR };
};