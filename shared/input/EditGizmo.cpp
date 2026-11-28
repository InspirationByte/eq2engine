//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Move / rotate / scale gizmo of the editors
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "math/Utility.h"

#include "EditGizmo.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "render/IDebugOverlay.h"

static void GizmoMakePolyCone(float baseSize, float height, int segments, const Vector3D& pos, const Quaternion& rot, int axis, CMeshBuilder& mb)
{
	Vector3D vertex = vec3_zero;
	vertex[axis] = height;

	mb.Position3fv(pos + rotateVector(vertex, rot));
	const int startIndex = mb.AdvanceVertex();

	const int u = (axis + 1) % 3;
	const int v = (axis + 2) % 3;
	vertex[axis] = 0.0f;

	for (int i = 0; i <= segments; ++i)
	{
		const float step = (float(i) / static_cast<float>(segments)) * M_PI_F * 2.0f;

		vertex[u] = sin(step) * baseSize;
		vertex[v] = cos(step) * baseSize;

		mb.Position3fv(pos + rotateVector(vertex, rot));
		mb.AdvanceVertex();
	}

	for (int i = 0; i < segments; ++i)
	{
		mb.AdvanceVertexIndex(startIndex);
		mb.AdvanceVertexIndex(startIndex + i + 1);
		mb.AdvanceVertexIndex(startIndex + i + 2);
	}
}

static void GizmoMakePolyCylinder(float baseSize, float height, int segments, const Vector3D& pos, const Quaternion& rot, int axis, CMeshBuilder& mb)
{
	Vector3D vertex = vec3_zero;

	const int u = (axis + 1) % 3;
	const int v = (axis + 2) % 3;

	int startIndex = -1;

	vertex[axis] = -height * 0.5f;
	for (int i = 0; i <= segments; ++i)
	{
		const float step = float(i) / static_cast<float>(segments) * M_PI_F * 2.0f;

		vertex[u] = sin(step) * baseSize;
		vertex[v] = cos(step) * baseSize;

		mb.Position3fv(pos + rotateVector(vertex, rot));
		const int idx = mb.AdvanceVertex();
		if (startIndex == -1)
			startIndex = idx;
	}

	vertex[axis] = height * 0.5f;
	for (int i = 0; i <= segments; ++i)
	{
		const float step = float(i) / static_cast<float>(segments) * M_PI_F * 2.0f;

		vertex[u] = sin(step) * baseSize;
		vertex[v] = cos(step) * baseSize;

		mb.Position3fv(pos + rotateVector(vertex, rot));
		mb.AdvanceVertex();
	}

	for (int i = 0; i <= segments; ++i)
	{
		mb.AdvanceVertexIndex(startIndex + i);
		mb.AdvanceVertexIndex(startIndex + i + 1);
		mb.AdvanceVertexIndex(startIndex + i + segments + 1);

		mb.AdvanceVertexIndex(startIndex + i + segments);
		mb.AdvanceVertexIndex(startIndex + i + segments + 1);
		mb.AdvanceVertexIndex(startIndex + i);
	}
}

void CEditGizmo::SetProps(const Matrix3x3& rotate, const Vector3D& pos)
{
	m_transform.t = pos;
	m_transform.r = rotate;
	m_transform.s = vec3_unit;
}

void CEditGizmo::SetProps(const Transform3D& transform)
{
	m_transform = transform;
}

void CEditGizmo::SetMode(EGizmoMode mode, int axes)
{
	m_mode = mode;
	m_axes = axes;
}

void CEditGizmo::SetDragHandlers(const DragBeginFunc& onBegin, const DragTranslateFunc& onTranslate, const DragRotateFunc& onRotate, const DragCompleteFunc& onComplete)
{
	m_onBegin = onBegin;
	m_onTranslate = onTranslate;
	m_onRotate = onRotate;
	m_onComplete = onComplete;
}

bool CEditGizmo::HandleMouse(EGizmoMouseEvent event, const Vector3D& rayStart, const Vector3D& rayDir, float camDist)
{
	if (IsDragging())
	{
		if (event == GIZMO_MOUSE_UP)
			EndDrag();
		else
			ProcessDrag(rayStart, rayDir);
		return true;
	}

	// the planes are drawn between two axes of the translation; a hit of the axes that are not
	// available is no hit, a plane of them falls back to the single axes
	const bool rotate = m_mode == EGizmoMode::ROTATE;
	const bool testPlanes = !rotate && numBitsSet(m_axes) > 1;
	int axes = TestRay(rayStart, rayDir, camDist, testPlanes);
	if ((axes & ~m_axes) && testPlanes)
		axes = TestRay(rayStart, rayDir, camDist, false);
	if (axes & ~m_axes)
		axes = 0;
	SetHighlightAxes(axes);

	if (event != GIZMO_MOUSE_DOWN || !axes || m_mode == EGizmoMode::SCALE)
		return false;

	if (m_onBegin && !m_onBegin(axes))
		return false;

	if (rotate)
	{
		m_dragRotStarted = false;
		BeginDragRotate(axes, [this](const Matrix3x3& rotation) {
			if (!m_dragRotStarted)
			{
				m_dragInitRot = rotation;
				m_dragRotStarted = true;
			}
			if (m_onRotate)
				m_onRotate(transpose(!m_dragInitRot * rotation));
		}, m_onComplete);
	}
	else
	{
		BeginDragTranslate(axes, [this](const Vector3D& movement) {
			if (m_onTranslate)
				m_onTranslate(movement);
		}, m_onComplete);
	}

	ProcessDrag(rayStart, rayDir);
	return true;
}

void CEditGizmo::Cancel()
{
	if (IsDragging())
		EndDrag(true);
}

bool CEditGizmo::IsDragging() const
{
	return m_draggedAxes != 0;
}

int CEditGizmo::GetDraggedAxes() const
{
	return m_draggedAxes;
}

void CEditGizmo::EndDrag(bool cancel)
{
	if (m_dragFuncComplete)
		m_dragFuncComplete(cancel);

	m_dragStart = vec3_zero;
	m_dragDirCur = vec3_zero;
	m_draggedAxes = 0;
	m_dragMode = false;

	m_dragFuncTranslate = nullptr;
	m_dragFuncRotate = nullptr;
	m_dragFuncComplete = nullptr;
}

void CEditGizmo::BeginDragTranslate(int initAxes, const DragTranslateFunc& dragFunc, const DragCompleteFunc& onComplete)
{
	if (initAxes == 0)
		return;
	m_draggedAxes = initAxes;
	m_dragFuncTranslate = dragFunc;
	m_dragFuncComplete = onComplete;
}

void CEditGizmo::BeginDragRotate(int initAxes, const DragRotateFunc& dragFunc, const DragCompleteFunc& onComplete)
{
	ASSERT_MSG(numBitsSet(initAxes) <= 1, "testPlanes must be off for TestRay");
	if (initAxes == 0)
		return;
	m_draggedAxes = initAxes;
	m_dragFuncRotate = dragFunc;
	m_dragFuncComplete = onComplete;
}

void CEditGizmo::ProcessDrag(const Vector3D& rayStart, const Vector3D& rayDir)
{
	if (!IsDragging())
		return;

	ASSERT(m_dragFuncTranslate || m_dragFuncRotate);
	if (m_dragFuncTranslate)
	{
		ASSERT(!m_dragFuncRotate);

		// calc movement plane
		const Vector3D planeNormal = m_transform.t - rayStart;
		const Plane pl(planeNormal, -dot(planeNormal, m_transform.t), true);

		Vector3D point;
		if (!pl.GetIntersectionWithRay(rayStart, rayDir, point))
			return;

		if(!m_dragMode)
			m_dragStart = m_transform.t - point;

		const Vector3D v_x = m_transform.right();
		const Vector3D v_y = m_transform.up();
		const Vector3D v_z = m_transform.forward();

		const Vector3D movement = (point - m_transform.t) + m_dragStart;
		const float movementXAmount = (m_draggedAxes & AXIS_X) ? dot(movement, v_x) : 0.0f;
		const float movementYAmount = (m_draggedAxes & AXIS_Y) ? dot(movement, v_y) : 0.0f;
		const float movementZAmount = (m_draggedAxes & AXIS_Z) ? dot(movement, v_z) : 0.0f;

		const Vector3D movementAmount = movementXAmount * v_x + movementYAmount * v_y + movementZAmount * v_z;
		m_dragFuncTranslate(movementAmount);
		m_dragMode = true;

		m_transform.t += movementAmount;
	}
	else if (m_dragFuncRotate)
	{
		ASSERT(!m_dragFuncTranslate);

		const Vector3D axsMod(	(m_draggedAxes & AXIS_X) ? 1.0f : 0.0f,
								(m_draggedAxes & AXIS_Y) ? 1.0f : 0.0f,
								(m_draggedAxes & AXIS_Z) ? 1.0f : 0.0f);

		const Vector3D edAxis = rotateVector(axsMod, m_transform.r);

		// movement plane
		const Plane pl(edAxis, -dot(edAxis, m_transform.t), true);
		Vector3D point;
		if (!pl.GetIntersectionWithRay(rayStart, rayDir, point))
			return;

		const Vector3D dirVec = normalize(point - m_transform.t);
		m_dragDirCur = dirVec;

		if (!m_dragMode)
			m_dragStart = dirVec;

		const Vector3D perpDirVec = normalize(cross(edAxis, dirVec));

		// process rotation
		int axisId = (m_draggedAxes == AXIS_X) ? 0 :
			(m_draggedAxes == AXIS_Y) ? -1 :
			(m_draggedAxes == AXIS_Z) ? -2 : -1;
		axisId += 3;

		const Matrix3x3 rotation(edAxis, dirVec, perpDirVec);

		// swap axes
		const Matrix3x3 swappedAxesRotation(rotation.rows[axisId % 3], rotation.rows[(axisId + 1) % 3], rotation.rows[(axisId + 2) % 3]);

		m_dragFuncRotate(swappedAxesRotation);
		m_dragMode = true;
	}
}

void CEditGizmo::Draw(float camDist, int axes, EGizmoMode mode, IGPURenderPassRecorder* rendPassRecorder) const
{
	const float fLength = camDist * EDAXIS_SCALE;
	const float fLengthHalf = fLength * 0.5f;
	const float fBoxSize = 0.1f * fLength;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultPass;
	RenderPassContext defaultPassContext(rendPassRecorder, &defaultPass);
	defaultPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultPass.depthTest = false;
	defaultPass.depthWrite = false;

	const Vector3D v_x = m_transform.right();
	const Vector3D v_y = m_transform.up();
	const Vector3D v_z = m_transform.forward();

	const Vector3D xPos = m_transform.t + v_x * fLength;
	const Vector3D yPos = m_transform.t + v_y * fLength;
	const Vector3D zPos = m_transform.t + v_z * fLength;

	const int highlightAxes = GetHighlightAxes();

	// draw lines
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);
	{
		if ((axes & AXIS_X) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_X))
		{
			meshBuilder.Color3fv((highlightAxes & AXIS_X) ? color_red : MColor(0.5f, 0.0f, 0.0f));
			meshBuilder.Line3fv(m_transform.t, xPos);
		}

		if ((axes & AXIS_Y) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Y))
		{
			meshBuilder.Color3fv((highlightAxes & AXIS_Y) ? color_green : MColor(0.0f, 0.5f, 0.0f));
			meshBuilder.Line3fv(m_transform.t, yPos);
		}

		if ((axes & AXIS_Z) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Z))
		{
			meshBuilder.Color3fv((highlightAxes & AXIS_Z) ? color_blue : MColor(0.0f, 0.0f, 0.5f));
			meshBuilder.Line3fv(m_transform.t, zPos);
		}
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);

	if (mode == EGizmoMode::ROTATE)
	{
		if (m_draggedAxes)
		{
			Quaternion circleAngle = (m_draggedAxes & AXIS_X) ? rotateZ(DEG2RAD(90)) :
				(m_draggedAxes & AXIS_Z) ? rotateX(DEG2RAD(90)) : qidentity;

			circleAngle = m_transform.r * circleAngle;

			// draw circle
			meshBuilder.Begin(PRIM_LINES);
			{
				const ColorRGBA axisColor = (m_draggedAxes & AXIS_X) ? ColorRGBA(1.0f, 0.25f, 0.25f, 1.0f) :
					(m_draggedAxes & AXIS_Z) ? ColorRGBA(0.0f, 0.25f, 1.0f, 1.0f) : ColorRGBA(0.25f, 1.0f, 0.25f, 1.0f);

				meshBuilder.Color4fv(axisColor);
				for (int i = 0; i < 32; i++)
				{
					float angle = 360.0f * (float)i / 32.0f;

					float si, co;
					SinCos(DEG2RAD(angle), &si, &co);
					const Vector3D circleAngleVec = rotateVector(Vector3D(si, 0, co), circleAngle) * fLength;

					meshBuilder.Position3fv(m_transform.t + circleAngleVec);
					meshBuilder.AdvanceVertex();
				}

				meshBuilder.Color4f(1.0f, 1.0f, 0.0f, 0.8f);
				meshBuilder.Line3fv(m_transform.t, m_transform.t + m_dragStart * fLength);
				meshBuilder.Line3fv(m_transform.t, m_transform.t + m_dragDirCur * fLength);
			}

			if (meshBuilder.End(drawCmd))
				g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
		}
	}
	else
	{
		const int drawAxes = m_draggedAxes == 0 ? axes : m_draggedAxes;

		meshBuilder.Begin(PRIM_TRIANGLES);
		{
			if ((drawAxes & (AXIS_X | AXIS_Z)) == (AXIS_X | AXIS_Z))
			{
				meshBuilder.Color4f(1, 0, 1, 0.5f);
				meshBuilder.Position3fv(m_transform.t); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_transform.t + v_z * fLengthHalf); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_transform.t + v_x * fLengthHalf); meshBuilder.AdvanceVertex();
			}
			if ((drawAxes & (AXIS_Y | AXIS_Z)) == (AXIS_Y | AXIS_Z))
			{
				meshBuilder.Color4f(0, 1, 1, 0.5f);
				meshBuilder.Position3fv(m_transform.t); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_transform.t + v_y * fLengthHalf); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_transform.t + v_z * fLengthHalf); meshBuilder.AdvanceVertex();
			}
			if ((drawAxes & (AXIS_X | AXIS_Y)) == (AXIS_X | AXIS_Y))
			{
				meshBuilder.Color4f(1, 0, 0, 0.5f);
				meshBuilder.Position3fv(m_transform.t); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_transform.t + v_x * fLengthHalf); meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv(m_transform.t + v_y * fLengthHalf); meshBuilder.AdvanceVertex();
			}
		}
		if (meshBuilder.End(drawCmd))
			g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
	}

	meshBuilder.Begin(PRIM_TRIANGLES);
	{
		float height = 0.5f;
		float size = 0.5f;
		int segments = 6;
		switch (mode)
		{
		case EGizmoMode::TRANSLATE:
			segments = 12;
			height = fBoxSize * 2.5f;
			size = fBoxSize * 0.75f;
			break;
		case EGizmoMode::ROTATE:
			segments = 12;
			height = fBoxSize * 0.5f;
			size = fBoxSize * 1.0f;
			break;
		case EGizmoMode::SCALE:
			segments = 4;
			height = fBoxSize * -1.0f;
			size = fBoxSize * 1.0f;
			break;
		}

		if ((axes & AXIS_X) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_X))
		{
			meshBuilder.Color4fv((highlightAxes& AXIS_X) ? color_yellow : MColor(0.75f, 0.0f, 0.0f, 0.5f));
			if (mode == EGizmoMode::ROTATE)
				GizmoMakePolyCylinder(size, height, segments, xPos, m_transform.r, 0, meshBuilder);
			else
				GizmoMakePolyCone(size, height, segments, xPos, m_transform.r, 0, meshBuilder);
		}

		if ((axes & AXIS_Y) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Y))
		{
			meshBuilder.Color4fv((highlightAxes& AXIS_Y) ? color_yellow : MColor(0.0f, 0.75f, 0.0f, 0.5f));
			if (mode == EGizmoMode::ROTATE)
				GizmoMakePolyCylinder(size, height, segments, yPos, m_transform.r, 1, meshBuilder);
			else
				GizmoMakePolyCone(size, height, segments, yPos, m_transform.r, 1, meshBuilder);
		}

		if ((axes & AXIS_Z) && m_draggedAxes == 0 || (m_draggedAxes & AXIS_Z))
		{
			meshBuilder.Color4fv((highlightAxes& AXIS_Z) ? color_yellow : MColor(0.0f, 0.0f, 0.75f, 0.5f));
			if (mode == EGizmoMode::ROTATE)
				GizmoMakePolyCylinder(size, height, segments, zPos, m_transform.r, 2, meshBuilder);
			else
				GizmoMakePolyCone(size, height, segments, zPos, m_transform.r, 2, meshBuilder);
		}
	}
	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
}

int	CEditGizmo::TestRay(const Vector3D& start, const Vector3D& dir, float camDist, bool testPlanes /*= true*/) const
{
	const float fLength = camDist * EDAXIS_SCALE;
	const float fLengthHalf = fLength*0.5f;
	const float fBoxSize = 0.1f*fLength;

	const Vector3D v_x = m_transform.right();
	const Vector3D v_y = m_transform.up();
	const Vector3D v_z = m_transform.forward();

	float frac = 1.0f;
	if(testPlanes)
	{
		// check the 2D coords
		if(IsRayIntersectsTriangle(m_transform.t, m_transform.t +v_z*fLengthHalf, m_transform.t +v_x*fLengthHalf, start, dir, frac, true))
			return (AXIS_X | AXIS_Z);

		if(IsRayIntersectsTriangle(m_transform.t, m_transform.t +v_y*fLengthHalf, m_transform.t +v_z*fLengthHalf, start, dir, frac, true))
			return (AXIS_Y | AXIS_Z);

		if(IsRayIntersectsTriangle(m_transform.t, m_transform.t +v_x*fLengthHalf, m_transform.t +v_y*fLengthHalf, start, dir, frac, true))
			return (AXIS_X | AXIS_Y);
	}

	// test box
	const BoundingBox boxX(m_transform.t+v_x*fLength - fBoxSize, m_transform.t+v_x*fLength + fBoxSize);
	const BoundingBox boxY(m_transform.t+v_y*fLength - fBoxSize, m_transform.t+v_y*fLength + fBoxSize);
	const BoundingBox boxZ(m_transform.t+v_z*fLength - fBoxSize, m_transform.t+v_z*fLength + fBoxSize);

	// test axis volumes
	float n,f;

	if(boxX.IntersectsRay(start, dir, n, f))
		return AXIS_X;
	else if(boxY.IntersectsRay(start, dir, n, f))
		return AXIS_Y;
	else if(boxZ.IntersectsRay(start, dir, n, f))
		return AXIS_Z;

	return 0;
}