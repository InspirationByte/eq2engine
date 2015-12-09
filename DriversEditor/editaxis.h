//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Edit axis
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITAXIS_H
#define EDITAXIS_H

#include "materialsystem/IMaterialSystem.h"
#include "IDebugOverlay.h"

#define EDAXIS_SCALE 0.15f

enum EAxisSelectionFlags
{
	AXIS_NONE = 0,

	AXIS_X = (1 << 0),
	AXIS_Y = (1 << 1),
	AXIS_Z = (1 << 2),
};

class CEditAxisXYZ
{
public:


	CEditAxisXYZ() {}

	CEditAxisXYZ(const Matrix3x3& rotate, const Vector3D& pos)
	{
		SetProps(rotate, pos);
	}

	void SetProps(const Matrix3x3& rotate, const Vector3D& pos)
	{
		m_position = pos;
		m_rotation = rotate;
	}

	void Draw(float camDist)
	{

		float fLength = camDist*EDAXIS_SCALE;
		float fLengthHalf = fLength*0.5f;

		float fBoxSize = 0.08f*fLength;

		Vector3D v_x, v_y, v_z;

		v_x = m_rotation*vec3_right;
		v_y = m_rotation*vec3_up;
		v_z = m_rotation*vec3_forward;

		Vertex3D_t axis_verts[] = 
		{
			Vertex3D_t(m_position, vec2_zero, ColorRGBA(1,0,0,1)), Vertex3D_t(m_position+v_x*fLength, vec2_zero, ColorRGBA(1,0,0,1)),
			Vertex3D_t(m_position, vec2_zero, ColorRGBA(0,1,0,1)), Vertex3D_t(m_position+v_y*fLength, vec2_zero, ColorRGBA(0,1,0,1)),
			Vertex3D_t(m_position, vec2_zero, ColorRGBA(0,0,1,1)), Vertex3D_t(m_position+v_z*fLength, vec2_zero, ColorRGBA(0,0,1,1)),
		};

		DepthStencilStateParams_t depthParams;
		depthParams.depthWrite = false;
		depthParams.depthTest = false;

		BlendStateParam_t blendParams;
		blendParams.blendEnable = true;
		blendParams.srcFactor = BLENDFACTOR_SRC_ALPHA;
		blendParams.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

		g_pShaderAPI->Reset(STATE_RESET_VBO);

		materials->DrawPrimitivesFFP(PRIM_LINES, axis_verts, 6, NULL, ColorRGBA(1), NULL, &depthParams);

		Vertex3D_t poly_verts[] = 
		{
			Vertex3D_t(m_position, vec2_zero, ColorRGBA(1,0,1,1)), 
			Vertex3D_t(m_position+v_z*fLengthHalf, vec2_zero, ColorRGBA(1,0,1,1)),
			Vertex3D_t(m_position+v_x*fLengthHalf, vec2_zero, ColorRGBA(1,0,1,1)),

			Vertex3D_t(m_position, vec2_zero, ColorRGBA(0,1,1,1)), 
			Vertex3D_t(m_position+v_y*fLengthHalf, vec2_zero, ColorRGBA(0,1,1,1)),
			Vertex3D_t(m_position+v_z*fLengthHalf, vec2_zero, ColorRGBA(0,1,1,1)),

			Vertex3D_t(m_position, vec2_zero, ColorRGBA(1,0,0,1)), 
			Vertex3D_t(m_position+v_x*fLengthHalf, vec2_zero, ColorRGBA(1,0,0,1)),
			Vertex3D_t(m_position+v_y*fLengthHalf, vec2_zero, ColorRGBA(1,0,0,1)),
		};

		materials->DrawPrimitivesFFP(PRIM_TRIANGLES, poly_verts, 9, NULL, ColorRGBA(1, 1, 1, 0.25f), NULL, &depthParams);

		/*
		debugoverlay->Box3D(m_position+v_x*fLength - fBoxSize, m_position+v_x*fLength + fBoxSize, ColorRGBA(1,0,0,1), 0);
		debugoverlay->Box3D(m_position+v_y*fLength - fBoxSize, m_position+v_y*fLength + fBoxSize, ColorRGBA(0,1,0,1), 0);
		debugoverlay->Box3D(m_position+v_z*fLength - fBoxSize, m_position+v_z*fLength + fBoxSize, ColorRGBA(0,0,1,1), 0);
		*/
	}

	int	TestRay(const Vector3D& start, const Vector3D& dir, float camDist, bool testPlanes = true)
	{
		float fLength = camDist*EDAXIS_SCALE;
		float fLengthHalf = fLength*0.5f;

		float fBoxSize = 0.1f*fLength;

		Vector3D v_x, v_y, v_z;

		v_x = m_rotation*vec3_right;
		v_y = m_rotation*vec3_up;
		v_z = m_rotation*vec3_forward;

		float frac = 1.0f;

		if(testPlanes)
		{
			// check the 2D coords
			if(IsRayIntersectsTriangle(m_position, m_position+v_z*fLengthHalf, m_position+v_x*fLengthHalf, start, dir, frac, true))
				return (AXIS_X | AXIS_Z);

			if(IsRayIntersectsTriangle(m_position, m_position+v_y*fLengthHalf, m_position+v_z*fLengthHalf, start, dir, frac, true))
				return (AXIS_Y | AXIS_Z);

			if(IsRayIntersectsTriangle(m_position, m_position+v_x*fLengthHalf, m_position+v_y*fLengthHalf, start, dir, frac, true))
				return (AXIS_X | AXIS_Y);
		}

		// test box

		BoundingBox box_x(m_position+v_x*fLength - fBoxSize, m_position+v_x*fLength + fBoxSize);
		BoundingBox box_y(m_position+v_y*fLength - fBoxSize, m_position+v_y*fLength + fBoxSize);
		BoundingBox box_z(m_position+v_z*fLength - fBoxSize, m_position+v_z*fLength + fBoxSize);
		/*
		debugoverlay->Box3D(box_x.minPoint, box_x.maxPoint, ColorRGBA(1,0,0,1), 5.0f);
		debugoverlay->Box3D(box_y.minPoint, box_y.maxPoint, ColorRGBA(0,1,0,1), 5.0f);
		debugoverlay->Box3D(box_z.minPoint, box_z.maxPoint, ColorRGBA(0,0,1,1), 5.0f);

		debugoverlay->Line3D(start, start+dir*10000.0f, ColorRGBA(1,1,1,1),ColorRGBA(1,1,1,1), 5.0f);
		*/
		// test axis volumes
		float n,f;

		if(box_x.IntersectsRay(start, dir, n, f))
			return AXIS_X;
		else if(box_y.IntersectsRay(start, dir, n, f))
			return AXIS_Y;
		else if(box_z.IntersectsRay(start, dir, n, f))
			return AXIS_Z;

		return 0;
	}

	Vector3D	m_position;
	Matrix3x3	m_rotation;
};



#endif // EDITAXIS_H