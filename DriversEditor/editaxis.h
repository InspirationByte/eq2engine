//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

	AXIS_ALL = AXIS_X | AXIS_Y | AXIS_Z
};

inline int _wrapIndex(int i, int l)
{
	const int m = i % l;
	return m < 0 ? m + l : m;
}

class CEditGizmo
{
public:
	CEditGizmo() : m_draggedAxes(0) {}

	CEditGizmo(const Matrix3x3& rotate, const Vector3D& pos)
	{
		SetProps(rotate, pos);
		m_draggedAxes = 0;
	}

	void SetProps(const Matrix3x3& rotate, const Vector3D& pos)
	{
		m_position = pos;
		m_rotation = rotate;
	}

	void EndDrag()
	{
		m_draggedAxes = 0;
		m_dragMode = 0;
	}

	Vector3D PerformTranslate(const Vector3D& rayStart, const Vector3D& rayDir, const Vector3D& planeNormal, int initAxes)
	{
		// movement plane
		Plane pl(planeNormal, -dot(planeNormal, m_position), true);

		Vector3D point;
		if (pl.GetIntersectionWithRay(rayStart, rayDir, point))
		{
			if (!m_draggedAxes)
			{
				m_dragStart = m_position - point;
				m_draggedAxes = initAxes;
				m_dragMode = 0;
			}

			Vector3D movement = (point - m_position) + m_dragStart;

			Vector3D axsMod((m_draggedAxes & AXIS_X) ? 1.0f : 0.0f,
							(m_draggedAxes & AXIS_Y) ? 1.0f : 0.0f,
							(m_draggedAxes & AXIS_Z) ? 1.0f : 0.0f);

			Vector3D edAxis = Vector3D(	dot(m_rotation.rows[0], axsMod),
										dot(m_rotation.rows[1], axsMod),
										dot(m_rotation.rows[2], axsMod));

			// process movement
			movement *= edAxis * sign(edAxis);

			return movement;
		}

		return vec3_zero;
	}

	Matrix3x3 PerformRotation(const Vector3D& rayStart, const Vector3D& rayDir, int initAxes)
	{
		if (!m_draggedAxes)
		{
			//m_dragStart = m_position - point;
			m_draggedAxes = initAxes;
			m_dragMode = 1;
		}

		Vector3D axsMod((m_draggedAxes & AXIS_X) ? 1.0f : 0.0f,
						(m_draggedAxes & AXIS_Y) ? 1.0f : 0.0f,
						(m_draggedAxes & AXIS_Z) ? 1.0f : 0.0f);

		Vector3D edAxis = Vector3D(	dot(m_rotation.rows[0], axsMod),
									dot(m_rotation.rows[1], axsMod),
									dot(m_rotation.rows[2], axsMod));

		Vector3D planeEdAxis = edAxis * sign(edAxis);

		// movement plane
		Plane pl(planeEdAxis, -dot(planeEdAxis, m_position), true);

		Vector3D point;
		if (pl.GetIntersectionWithRay(rayStart, rayDir, point))
		{
			// process rotation
			int axisId = (m_draggedAxes & AXIS_X) ? 0 :
						(m_draggedAxes & AXIS_Y) ? 1 :
						(m_draggedAxes & AXIS_Z) ? 2 : -1;

			Vector3D dirVec = normalize(point-m_position);
			Vector3D rDirVec = cross(planeEdAxis, dirVec);

			Matrix3x3 rotation(rDirVec, planeEdAxis, dirVec);

			// swap axes
			Matrix3x3 swappedAxesRotation(rotation.rows[_wrapIndex(axisId, 3)], rotation.rows[_wrapIndex(axisId + 1, 3)], rotation.rows[_wrapIndex(axisId + 2, 3)]);

			//debugoverlay->Line3D(m_position, m_position + swappedAxesRotation.rows[0] * 5.0f, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1), 0.1f);
			//debugoverlay->Line3D(m_position, m_position + swappedAxesRotation.rows[1] * 5.0f, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1), 0.1f);
			//debugoverlay->Line3D(m_position, m_position + swappedAxesRotation.rows[2] * 5.0f, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1), 0.1f);

			return swappedAxesRotation;
		}

		return identity3();
	}

	void Draw(float camDist, int axes = AXIS_ALL)
	{
		float fLength = camDist * EDAXIS_SCALE;
		float fLengthHalf = fLength * 0.5f;

		float fBoxSize = 0.1f*fLength;

		Vector3D v_x, v_y, v_z;

		v_x = m_rotation * vec3_right;
		v_y = m_rotation * vec3_up;
		v_z = m_rotation * vec3_forward;

		materials->SetAmbientColor(1.0f);

		materials->SetDepthStates(false, false);
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
		materials->SetRasterizerStates(CULL_NONE, FILL_SOLID);

		g_pShaderAPI->SetTexture(NULL, NULL, 0);
		materials->BindMaterial(materials->GetDefaultMaterial());

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());

		Vector3D xPos = m_position + v_x * fLength;
		Vector3D yPos = m_position + v_y * fLength;
		Vector3D zPos = m_position + v_z * fLength;

		// draw lines
		meshBuilder.Begin(PRIM_LINES);

		if ((axes & AXIS_X) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_X))
		{
			meshBuilder.Color3f(1, 0, 0);
			meshBuilder.Line3fv(m_position, xPos);
		}

		if ((axes & AXIS_Y) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Y))
		{
			meshBuilder.Color3f(0, 1, 0);
			meshBuilder.Line3fv(m_position, yPos);
		}

		if ((axes & AXIS_Z) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Z))
		{
			meshBuilder.Color3f(0, 0, 1);
			meshBuilder.Line3fv(m_position, zPos);
		}

		meshBuilder.End();

		if (m_dragMode == 1 && m_draggedAxes)
		{
			Matrix3x3 circleAngle = (m_draggedAxes & AXIS_X) ? rotateZ3(DEG2RAD(90)) : 
									(m_draggedAxes & AXIS_Z) ? rotateX3(DEG2RAD(90)) : identity3();

			circleAngle = m_rotation * circleAngle;

			// draw circle
			meshBuilder.Begin(PRIM_LINE_STRIP);

				meshBuilder.Color4f(1.0f, 1.0f, 0.0f, 0.8f);
				for (int i = 0; i < 33; i++)
				{
					float angle = 360.0f*(float)i / 32.0f;

					float si, co;
					SinCos(DEG2RAD(angle), &si, &co);

					Vector3D circleAngleVec = circleAngle*Vector3D(si, 0, co)*fLength;

					meshBuilder.Position3fv(m_position + circleAngleVec);
					meshBuilder.AdvanceVertex();
				}

			meshBuilder.End();
		}

		meshBuilder.Begin(PRIM_TRIANGLES);
			if ((axes & AXIS_X) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_X) && (m_draggedAxes & AXIS_Z))
			{
				meshBuilder.Color4f(1, 0, 1, 0.5f);
				meshBuilder.Position3fv(m_position); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_position + v_z * fLengthHalf); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_position + v_x * fLengthHalf); meshBuilder.AdvanceVertex();
			}
			if ((axes & AXIS_Y) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Y) && (m_draggedAxes & AXIS_Z))
			{
				meshBuilder.Color4f(0, 1, 1, 0.5f);
				meshBuilder.Position3fv(m_position); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_position + v_y * fLengthHalf); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_position + v_z * fLengthHalf); meshBuilder.AdvanceVertex();
			}
			if ((axes & AXIS_Z) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_X) && (m_draggedAxes & AXIS_Y))
			{
				meshBuilder.Color4f(1, 0, 0, 0.5f);
				meshBuilder.Position3fv(m_position); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_position + v_x * fLengthHalf); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_position + v_y * fLengthHalf); meshBuilder.AdvanceVertex();
			}
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
			if ((axes & AXIS_X) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_X))
			{
				meshBuilder.Color4f(1, 0, 0, 0.5f);
				meshBuilder.Quad3(quadPointsX[0], quadPointsX[1], quadPointsX[2], quadPointsX[3]);
			}

			if ((axes & AXIS_Y) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Y))
			{
				meshBuilder.Color4f(0, 1, 0, 0.5f);
				meshBuilder.Quad3(quadPointsY[0], quadPointsY[1], quadPointsY[2], quadPointsY[3]);
			}

			if ((axes & AXIS_Z) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Z))
			{
				meshBuilder.Color4f(0, 0, 1, 0.5f);
				meshBuilder.Quad3(quadPointsZ[0], quadPointsZ[1], quadPointsZ[2], quadPointsZ[3]);
			}
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

protected:
	Vector3D	m_dragStart;
	int			m_draggedAxes;
	int			m_dragMode;
};



#endif // EDITAXIS_H