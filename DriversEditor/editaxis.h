//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Edit axis
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITAXIS_H
#define EDITAXIS_H

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MeshBuilder.h"
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

		float fBoxSize = 0.1f*fLength;

		Vector3D v_x, v_y, v_z;

		v_x = m_rotation*vec3_right;
		v_y = m_rotation*vec3_up;
		v_z = m_rotation*vec3_forward;

		materials->SetAmbientColor(1.0f);

		materials->SetDepthStates(false,false);
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA,BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
		materials->SetRasterizerStates(CULL_NONE, FILL_SOLID);

		g_pShaderAPI->SetTexture(NULL, NULL, 0);
		materials->BindMaterial(materials->GetDefaultMaterial());

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());

		Vector3D xPos = m_position + v_x * fLength;
		Vector3D yPos = m_position + v_y * fLength;
		Vector3D zPos = m_position + v_z * fLength;

		meshBuilder.Begin(PRIM_LINES);
			meshBuilder.Color3f(1,0,0);
			meshBuilder.Line3fv(m_position, xPos);

			meshBuilder.Color3f(0,1,0);
			meshBuilder.Line3fv(m_position, yPos);

			meshBuilder.Color3f(0,0,1);
			meshBuilder.Line3fv(m_position, zPos);
		meshBuilder.End();

		meshBuilder.Begin(PRIM_TRIANGLES);
			meshBuilder.Color4f(1,0,1,0.5f);
			meshBuilder.Position3fv(m_position); meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv(m_position+v_z*fLengthHalf); meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv(m_position+v_x*fLengthHalf); meshBuilder.AdvanceVertex();

			meshBuilder.Color4f(0,1,1,0.5f);
			meshBuilder.Position3fv(m_position); meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv(m_position+v_y*fLengthHalf); meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv(m_position+v_z*fLengthHalf); meshBuilder.AdvanceVertex();

			meshBuilder.Color4f(1,0,0,0.5f);
			meshBuilder.Position3fv(m_position); meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv(m_position+v_x*fLengthHalf); meshBuilder.AdvanceVertex();
			meshBuilder.Position3fv(m_position+v_y*fLengthHalf); meshBuilder.AdvanceVertex();
		meshBuilder.End();

		Matrix4x4 camView;
		materials->GetMatrix(MATRIXMODE_VIEW, camView);

#define EDAXISMAKESPRITE(point)\
		point + (camView.rows[1].xyz() * fBoxSize) + (camView.rows[0].xyz() * fBoxSize),\
		point + (camView.rows[1].xyz() * fBoxSize) - (camView.rows[0].xyz() * fBoxSize),\
		point - (camView.rows[1].xyz() * fBoxSize) + (camView.rows[0].xyz() * fBoxSize),\
		point - (camView.rows[1].xyz() * fBoxSize) - (camView.rows[0].xyz() * fBoxSize),

		Vector3D quadPointsX[4] = { EDAXISMAKESPRITE(xPos) };
		Vector3D quadPointsY[4] = { EDAXISMAKESPRITE(yPos) };
		Vector3D quadPointsZ[4] = { EDAXISMAKESPRITE(zPos) };

#undef EDAXISMAKESPRITE

		meshBuilder.Begin(PRIM_TRIANGLES);
			meshBuilder.Color4f(1, 0, 0, 0.5f);
			meshBuilder.Quad3(quadPointsX[0], quadPointsX[1], quadPointsX[2], quadPointsX[3]);

			meshBuilder.Color4f(0, 1, 0, 0.5f);
			meshBuilder.Quad3(quadPointsY[0], quadPointsY[1], quadPointsY[2], quadPointsY[3]);

			meshBuilder.Color4f(0, 0, 1, 0.5f);
			meshBuilder.Quad3(quadPointsZ[0], quadPointsZ[1], quadPointsZ[2], quadPointsZ[3]);
		meshBuilder.End();


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