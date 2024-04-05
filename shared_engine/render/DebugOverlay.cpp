//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "math/Utility.h"
#include "math/Rectangle.h"
#include "DebugOverlay.h"

#include "font/IFontCache.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

static constexpr const int BOXES_DRAW_SUBDIV = 4096 / 16;
static constexpr const int LINES_DRAW_SUBDIV = 4096;
static constexpr const int POLYS_DRAW_SUBDIV = 256;
static constexpr const int GRAPH_MAX_VALUES = 400;

static CDebugOverlay g_DebugOverlays;
IDebugOverlay* debugoverlay = (IDebugOverlay*)&g_DebugOverlays;

#ifdef ENABLE_DEBUG_DRAWING
static Threading::CEqMutex	s_debugOverlayMutex;

DECLARE_CVAR(r_debugDrawFrameStats, "0", nullptr, CV_ARCHIVE);
DECLARE_CVAR(r_debugDrawGraphs, "0", nullptr, CV_ARCHIVE);
DECLARE_CVAR(r_debugDrawShapes, "0", nullptr, CV_ARCHIVE);
DECLARE_CVAR(r_debugDrawLines, "0", nullptr, CV_ARCHIVE);

void CDebugOverlay::OnShowTextureChanged(ConVar* pVar,char const* pszOldValue)
{
	if (!g_renderAPI)
		return;

	g_DebugOverlays.m_dbgTexture = g_renderAPI->FindTexture( pVar->GetString() );
}

DECLARE_CVAR_CHANGE(r_debugShowTexture, "", &CDebugOverlay::OnShowTextureChanged, "input texture name to show texture. To hide view input anything else.", CV_CHEAT);
DECLARE_CVAR(r_debugShowTextureScale, "1.0", nullptr, CV_ARCHIVE);

static void GUIDrawWindow(const AARectangle &rect, const MColor& color1, IGPURenderPassRecorder* rendPassRecorder)
{
	MColor color2(0.2f,0.2f,0.2f,0.8f);

	const Vector2D r0[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.leftTop.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r1[] = { MAKEQUAD(rect.rightBottom.x, rect.leftTop.y,rect.rightBottom.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r2[] = { MAKEQUAD(rect.leftTop.x, rect.rightBottom.y,rect.rightBottom.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r3[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.rightBottom.x, rect.leftTop.y, -0.5f) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = false;
	defaultRenderPass.depthWrite = false;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		// put main rectangle
		meshBuilder.Color4fv(color1);
		meshBuilder.Quad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop());

		// put borders
		meshBuilder.Color4fv(color2);
		meshBuilder.Quad2(r0[0], r0[1], r0[2], r0[3]);
		meshBuilder.Quad2(r1[0], r1[1], r1[2], r1[3]);
		meshBuilder.Quad2(r2[0], r2[1], r2[2], r2[3]);
		meshBuilder.Quad2(r3[0], r3[1], r3[2], r3[3]);
	if(meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}


// NOTE: unused, kept for reference and further use
static void DrawOrientedBoxFilled(const Vector3D& position, const Vector3D& mins, const Vector3D& maxs, const Quaternion& quat, const MColor& color, float fTime = 0.0f)
{
	Vector3D verts[18] = { 
		Vector3D(mins.x, maxs.y, maxs.z),
		Vector3D(maxs.x, maxs.y, maxs.z),
		Vector3D(mins.x, maxs.y, mins.z),
		Vector3D(maxs.x, maxs.y, mins.z),
		Vector3D(mins.x, mins.y, mins.z),
		Vector3D(maxs.x, mins.y, mins.z),
		Vector3D(mins.x, mins.y, maxs.z),
		Vector3D(maxs.x, mins.y, maxs.z),
		Vector3D(maxs.x, mins.y, maxs.z),
		Vector3D(maxs.x, mins.y, mins.z),
		Vector3D(maxs.x, mins.y, mins.z),
		Vector3D(maxs.x, maxs.y, mins.z),
		Vector3D(maxs.x, mins.y, maxs.z),
		Vector3D(maxs.x, maxs.y, maxs.z),
		Vector3D(mins.x, mins.y, maxs.z),
		Vector3D(mins.x, maxs.y, maxs.z),
		Vector3D(mins.x, mins.y, mins.z),
		Vector3D(mins.x, maxs.y, mins.z)
	};

	// transform them
	for (int i = 0; i < elementsOf(verts); i++)
		verts[i] = position + rotateVector(verts[i], quat);

	Vector3D r, u, f;
	r = rotateVector(vec3_right, quat);
	u = rotateVector(vec3_up, quat);
	f = rotateVector(vec3_forward, quat);

	debugoverlay->Line3D(position + r * mins.x, position + r * maxs.x, MColor(1, 0, 0, 1), MColor(1, 0, 0, 1));
	debugoverlay->Line3D(position + u * mins.y, position + u * maxs.y, MColor(0, 1, 0, 1), MColor(0, 1, 0, 1));
	debugoverlay->Line3D(position + f * mins.z, position + f * maxs.z, MColor(0, 0, 1, 1), MColor(0, 0, 1, 1));

	MColor polyColor(color);
	polyColor.a *= 0.65f;

	debugoverlay->Polygon3D(verts[0], verts[1], verts[2], polyColor);
	debugoverlay->Polygon3D(verts[2], verts[1], verts[3], polyColor);

	debugoverlay->Polygon3D(verts[2], verts[3], verts[4], polyColor);
	debugoverlay->Polygon3D(verts[4], verts[3], verts[5], polyColor);

	debugoverlay->Polygon3D(verts[4], verts[5], verts[6], polyColor);
	debugoverlay->Polygon3D(verts[6], verts[5], verts[7], polyColor);

	debugoverlay->Polygon3D(verts[10], verts[11], verts[12], polyColor);
	debugoverlay->Polygon3D(verts[12], verts[11], verts[13], polyColor);

	debugoverlay->Polygon3D(verts[12], verts[13], verts[14], polyColor);
	debugoverlay->Polygon3D(verts[14], verts[13], verts[15], polyColor);

	debugoverlay->Polygon3D(verts[14], verts[15], verts[16], polyColor);
	debugoverlay->Polygon3D(verts[16], verts[15], verts[17], polyColor);
}
#endif // ENABLE_DEBUG_DRAWING

void CDebugOverlay::Init(bool hidden)
{
#ifdef ENABLE_DEBUG_DRAWING
	if (!hidden)
	{
		r_debugDrawFrameStats.SetBool(true);
		r_debugDrawGraphs.SetBool(true);
		r_debugDrawShapes.SetBool(true);
		r_debugDrawLines.SetBool(true);
	}
#endif // ENABLE_DEBUG_DRAWING

	m_debugFont = g_fontCache->GetFont("debug", 0);
	m_debugFont2 = g_fontCache->GetFont("default", 0);
}

void CDebugOverlay::Shutdown()
{
#ifdef ENABLE_DEBUG_DRAWING
	m_dbgTexture = nullptr;
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Text(const MColor& color, char const *fmt,...)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(!r_debugDrawFrameStats.GetBool())
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugTextNode_t& textNode = m_TextArray.append();
	textNode.color = color.pack();

	va_list		argptr;
	va_start (argptr,fmt);
	textNode.pszText = EqString::FormatVa(fmt, argptr);
	va_end (argptr);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Text3D(const Vector3D &origin, float dist, const MColor& color, const char* text, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(hashId == 0 && !m_frustum.IsSphereInside(origin, 1.0f))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugText3DNode_t& textNode = m_Text3DArray.append();
	textNode.color = color.pack();
	textNode.origin = origin;
	textNode.dist = dist;
	textNode.lifetime = fTime;
	textNode.pszText = text;

	textNode.frameindex = m_frameId;
	textNode.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

#define MAX_MINICON_MESSAGES 32

void CDebugOverlay::TextFadeOut(int position, const MColor& color,float fFadeTime, char const *fmt, ...)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(position == 1)
	{
		if(!r_debugDrawFrameStats.GetBool())
			return;
	}

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugFadingTextNode_t textNode;
	textNode.color = color.pack();
	textNode.lifetime = fFadeTime;
	textNode.initialLifetime = fFadeTime;

	va_list		argptr;
	va_start (argptr,fmt);
	textNode.pszText = EqString::FormatVa(fmt, argptr);
	va_end (argptr);
	if(position == 0)
	{
		m_LeftTextFadeArray.append(std::move(textNode));

		if(m_LeftTextFadeArray.getCount() > MAX_MINICON_MESSAGES)
			m_LeftTextFadeArray.popFront();
	}
	else
		m_RightTextFadeArray.append(std::move(textNode));
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Box3D(const Vector3D &mins, const Vector3D &maxs, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(!r_debugDrawShapes.GetBool())
		return;

	if(hashId == 0 && !m_frustum.IsBoxInside(mins,maxs))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugBoxNode_t& box = m_BoxList.append();

	box.mins = mins;
	box.maxs = maxs;
	box.color = color.pack();
	box.lifetime = fTime;

	box.frameindex = m_frameId;
	box.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Cylinder3D(const Vector3D& position, float radius, float height, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if (!r_debugDrawShapes.GetBool())
		return;

	Vector3D boxSize(radius, height * 0.5f, radius);
	if(hashId == 0 && !m_frustum.IsSphereInside(position, max(radius, height)))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugCylinderNode_t& cyl = m_CylinderList.append();
	cyl.origin = position;
	cyl.radius = radius;
	cyl.height = height;
	cyl.color = color.pack();
	cyl.lifetime = fTime;

	cyl.frameindex = m_frameId;
	cyl.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Line3D(const Vector3D &start, const Vector3D &end, const MColor& color1, const MColor& color2, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(!r_debugDrawLines.GetBool())
		return;

	if(hashId == 0 && !m_frustum.IsBoxInside(start,end))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugLineNode_t& line = m_LineList.append();
	line.start = start;
	line.end = end;
	line.color1 = color1.pack();
	line.color2 = color2.pack();
	line.lifetime = fTime;

	line.frameindex = m_frameId;
	line.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::OrientedBox3D(const Vector3D& mins, const Vector3D& maxs, const Vector3D& position, const Quaternion& rotation, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(!r_debugDrawShapes.GetBool())
		return;

	if(hashId == 0 && !m_frustum.IsBoxInside(position+mins, position+maxs))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugOriBoxNode_t& box = m_OrientedBoxList.append();
	box.mins = mins;
	box.maxs = maxs;
	box.position = position;
	box.rotation = rotation;
	box.color = color.pack();
	box.lifetime = fTime;

	box.frameindex = m_frameId;
	box.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Sphere3D(const Vector3D& position, float radius, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(!r_debugDrawShapes.GetBool())
		return;

	if(hashId == 0 && !m_frustum.IsSphereInside(position, radius))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugSphereNode_t& sphere = m_SphereList.append();
	sphere.origin = position;
	sphere.radius = radius;
	sphere.color = color.pack();
	sphere.lifetime = fTime;

	sphere.frameindex = m_frameId;
	sphere.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(hashId == 0 && !m_frustum.IsTriangleInside(v0,v1,v2))
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugPolyNode_t& poly = m_polygons.append();
	poly.verts.append(v0);
	poly.verts.append(v1);
	poly.verts.append(v2);

	poly.color = color.pack();
	poly.lifetime = fTime;

	poly.frameindex = m_frameId;
	poly.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Polygon3D(ArrayCRef<Vector3D> verts, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	if (hashId == 0)
	{
		bool anyVisible = false;
		for (int i = 0; i < verts.numElem() - 2; ++i)
		{
			if (m_frustum.IsTriangleInside(verts[0], verts[i + 1], verts[i + 2]))
			{
				anyVisible = true;
				break;
			}
		}

		if (!anyVisible)
			return;
	}

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugPolyNode_t& poly = m_polygons.append();
	poly.verts.append(verts.ptr(), verts.numElem());

	poly.color = color.pack();
	poly.lifetime = fTime;

	poly.frameindex = m_frameId;
	poly.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Volume3D(ArrayCRef<Plane> planes, const MColor& color, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	// if (hashId == 0)
	// {
	// 	bool anyVisible = false;
	// 	for (int i = 0; i < verts.numElem() - 2; ++i)
	// 	{
	// 		if (m_frustum.IsTriangleInside(verts[0], verts[i + 1], verts[i + 2]))
	// 		{
	// 			anyVisible = true;
	// 			break;
	// 		}
	// 	}
	// 
	// 	if (!anyVisible)
	// 		return;
	// }

	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugVolumeNode_t& volume = m_volumes.append();
	volume.planes.append(planes.ptr(), planes.numElem());

	volume.color = color.pack();
	volume.lifetime = fTime;

	volume.frameindex = m_frameId;
	volume.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Draw2DFunc(const OnDebugDrawFn& func, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugDrawFunc_t& fn = m_draw2DFuncs.append();
	fn.func = func;
	fn.lifetime = fTime;

	fn.frameindex = m_frameId;
	fn.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

void CDebugOverlay::Draw3DFunc(const OnDebugDrawFn& func, float fTime, int hashId)
{
#ifdef ENABLE_DEBUG_DRAWING
	Threading::CScopedMutex m(s_debugOverlayMutex);

	DebugDrawFunc_t& fn = m_draw3DFuncs.append();
	fn.func = func;
	fn.lifetime = fTime;

	fn.frameindex = m_frameId;
	fn.nameHash = hashId;

	if (hashId != 0)
		m_newNames.insert(hashId, m_frameId);
#endif // ENABLE_DEBUG_DRAWING
}

#ifdef ENABLE_DEBUG_DRAWING
static void DrawLineArray(ArrayRef<DebugLineNode_t> lines, float frametime, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!lines.numElem())
		return;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;
	defaultRenderPass.depthWrite = false;

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

		for(int i = 0; i < lines.numElem(); i++)
		{
			DebugLineNode_t& line = lines[i];

			meshBuilder.Color4(line.color1);
			meshBuilder.Position3fv(line.start);

			meshBuilder.AdvanceVertex();

			meshBuilder.Color4(line.color2);
			meshBuilder.Position3fv(line.end);

			meshBuilder.AdvanceVertex();

			line.lifetime -= frametime;
		}

	if(meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

static void DrawOrientedBoxArray(ArrayRef<DebugOriBoxNode_t> boxes, float frametime, IGPURenderPassRecorder* rendPassRecorder)
{
	if (!boxes.numElem())
		return;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;
	defaultRenderPass.depthWrite = false;

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

	for (int i = 0; i < boxes.numElem(); i++)
	{
		if (i > 0 && (i % BOXES_DRAW_SUBDIV) == 0)
		{
			if (meshBuilder.End(drawCmd))
				g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));

			// start with new mesh
			meshBuilder.Init(g_matSystem->GetDynamicMesh());
			meshBuilder.Begin(PRIM_LINES);
		}

		DebugOriBoxNode_t& node = boxes[i];

		meshBuilder.Color4(node.color);

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.maxs.y, node.mins.z), node.rotation),
			node.position + rotateVector(Vector3D(node.mins.x, node.maxs.y, node.maxs.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.maxs.x, node.maxs.y, node.maxs.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.maxs.y, node.mins.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.maxs.x, node.mins.y, node.mins.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.mins.y, node.maxs.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.mins.y, node.maxs.z), node.rotation),
			node.position + rotateVector(Vector3D(node.mins.x, node.mins.y, node.mins.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.mins.y, node.maxs.z), node.rotation),
			node.position + rotateVector(Vector3D(node.mins.x, node.maxs.y, node.maxs.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.maxs.x, node.mins.y, node.maxs.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.maxs.y, node.maxs.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.mins.y, node.mins.z), node.rotation),
			node.position + rotateVector(Vector3D(node.mins.x, node.maxs.y, node.mins.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.maxs.x, node.mins.y, node.mins.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.maxs.y, node.mins.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.maxs.y, node.mins.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.maxs.y, node.mins.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.maxs.y, node.maxs.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.maxs.y, node.maxs.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.mins.y, node.mins.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.mins.y, node.mins.z), node.rotation));

		meshBuilder.Line3fv(node.position + rotateVector(Vector3D(node.mins.x, node.mins.y, node.maxs.z), node.rotation),
			node.position + rotateVector(Vector3D(node.maxs.x, node.mins.y, node.maxs.z), node.rotation));

		node.lifetime -= frametime;
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

static void DrawBoxArray(ArrayRef<DebugBoxNode_t> boxes, float frametime, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!boxes.numElem())
		return;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;
	defaultRenderPass.depthWrite = false;

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

		for(int i = 0; i < boxes.numElem(); i++)
		{
			if (i > 0 && (i % BOXES_DRAW_SUBDIV) == 0)
			{
				if (meshBuilder.End(drawCmd))
					g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));

				// start with new mesh
				meshBuilder.Init(g_matSystem->GetDynamicMesh());
				meshBuilder.Begin(PRIM_LINES);
			}

			DebugBoxNode_t& node = boxes[i];

			meshBuilder.Color4(node.color);

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.maxs.y, node.mins.z),
								Vector3D(node.mins.x, node.maxs.y, node.maxs.z));

			meshBuilder.Line3fv(Vector3D(node.maxs.x, node.maxs.y, node.maxs.z),
								Vector3D(node.maxs.x, node.maxs.y, node.mins.z));

			meshBuilder.Line3fv(Vector3D(node.maxs.x, node.mins.y, node.mins.z),
								Vector3D(node.maxs.x, node.mins.y, node.maxs.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.mins.y, node.maxs.z),
								Vector3D(node.mins.x, node.mins.y, node.mins.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.mins.y, node.maxs.z),
								Vector3D(node.mins.x, node.maxs.y, node.maxs.z));

			meshBuilder.Line3fv(Vector3D(node.maxs.x, node.mins.y, node.maxs.z),
								Vector3D(node.maxs.x, node.maxs.y, node.maxs.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.mins.y, node.mins.z),
								Vector3D(node.mins.x, node.maxs.y, node.mins.z));

			meshBuilder.Line3fv(Vector3D(node.maxs.x, node.mins.y, node.mins.z),
								Vector3D(node.maxs.x, node.maxs.y, node.mins.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.maxs.y, node.mins.z),
								Vector3D(node.maxs.x, node.maxs.y, node.mins.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.maxs.y, node.maxs.z),
								Vector3D(node.maxs.x, node.maxs.y, node.maxs.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.mins.y, node.mins.z),
								Vector3D(node.maxs.x, node.mins.y, node.mins.z));

			meshBuilder.Line3fv(Vector3D(node.mins.x, node.mins.y, node.maxs.z),
								Vector3D(node.maxs.x, node.mins.y, node.maxs.z));

			node.lifetime -= frametime;
		}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

static void DrawCylinder(CMeshBuilder& meshBuilder, DebugCylinderNode_t& cylinder, float frametime)
{
	static const int NUM_SEG = 8;
	static float dir[NUM_SEG * 2];
	static bool init = false;
	if (!init)
	{
		init = true;
		for (int i = 0; i < NUM_SEG; ++i)
		{
			const float a = (float)i / (float)NUM_SEG * M_PI_F * 2;
			dir[i * 2] = cosf(a);
			dir[i * 2 + 1] = sinf(a);
		}
	}

	Vector3D min, max;
	min = cylinder.origin + Vector3D(-cylinder.radius, -cylinder.height * 0.5f, -cylinder.radius);
	max = cylinder.origin + Vector3D(cylinder.radius, cylinder.height * 0.5f, cylinder.radius);

	const float cx = (max.x + min.x) / 2;
	const float cz = (max.z + min.z) / 2;
	const float rx = (max.x - min.x) / 2;
	const float rz = (max.z - min.z) / 2;

	meshBuilder.Color4(cylinder.color);

	for (int i = 0, j = NUM_SEG - 1; i < NUM_SEG; j = i++)
	{
		meshBuilder.Line3fv(
			Vector3D(cx + dir[j * 2 + 0] * rx, min.y, cz + dir[j * 2 + 1] * rz), 
			Vector3D(cx + dir[i * 2 + 0] * rx, min.y, cz + dir[i * 2 + 1] * rz));

		meshBuilder.Line3fv(
			Vector3D(cx + dir[j * 2 + 0] * rx, max.y, cz + dir[j * 2 + 1] * rz),
			Vector3D(cx + dir[i * 2 + 0] * rx, max.y, cz + dir[i * 2 + 1] * rz));
	}

	for (int i = 0; i < NUM_SEG; i += NUM_SEG / 4)
	{
		meshBuilder.Line3fv(
			Vector3D(cx + dir[i * 2 + 0] * rx, min.y, cz + dir[i * 2 + 1] * rz),
			Vector3D(cx + dir[i * 2 + 0] * rx, max.y, cz + dir[i * 2 + 1] * rz));
	}

	cylinder.lifetime -= frametime;
}

static void DrawCylinderArray(ArrayRef<DebugCylinderNode_t> cylArray, float frametime, IGPURenderPassRecorder* rendPassRecorder)
{
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;
	defaultRenderPass.depthWrite = false;

	meshBuilder.Begin(PRIM_LINES);

	for (int i = 0; i < cylArray.numElem(); ++i)
	{
		if (i > 0 && (i % BOXES_DRAW_SUBDIV) == 0)
		{
			if (meshBuilder.End(drawCmd))
				g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));

			// start with new mesh
			meshBuilder.Init(g_matSystem->GetDynamicMesh());
			meshBuilder.Begin(PRIM_LINES);
		}

		DrawCylinder(meshBuilder, cylArray[i], frametime);
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

static void DrawGraph(DbgGraphBucket* graph, int position, IEqFont* pFont, float frame_time, IGPURenderPassRecorder* rendPassRecorder)
{
	const float GRAPH_HEIGHT = 100;
	const float GRAPH_Y_OFFSET = 50;

	float x_pos = 15;
	float y_pos = GRAPH_Y_OFFSET + GRAPH_HEIGHT + position*110;

	Vector2D last_point(-1);

	Vertex2D lines[] =
	{
		Vertex2D(Vector2D(x_pos, y_pos), vec2_zero),
		Vertex2D(Vector2D(x_pos, y_pos - GRAPH_HEIGHT), vec2_zero),

		Vertex2D(Vector2D(x_pos, y_pos), vec2_zero),
		Vertex2D(Vector2D(x_pos+400, y_pos), vec2_zero),

		Vertex2D(Vector2D(x_pos, y_pos - GRAPH_HEIGHT *0.75f), vec2_zero),
		Vertex2D(Vector2D(x_pos+32, y_pos - GRAPH_HEIGHT *0.75f), vec2_zero),

		Vertex2D(Vector2D(x_pos, y_pos - GRAPH_HEIGHT *0.50f), vec2_zero),
		Vertex2D(Vector2D(x_pos+32, y_pos - GRAPH_HEIGHT *0.50f), vec2_zero),

		Vertex2D(Vector2D(x_pos, y_pos - GRAPH_HEIGHT *0.25f), vec2_zero),
		Vertex2D(Vector2D(x_pos+32, y_pos - GRAPH_HEIGHT *0.25f), vec2_zero),
	};

	eqFontStyleParam_t textStl;
	textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

	pFont->SetupRenderText(graph->name, Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT - 16), textStl, rendPassRecorder);
	pFont->SetupRenderText("0", Vector2D(x_pos + 5, y_pos), textStl, rendPassRecorder);
	pFont->SetupRenderText(EqString::Format("%.2f", graph->maxValue).ToCString(), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT), textStl, rendPassRecorder);

	MatSysDefaultRenderPass defaultRender;
	defaultRender.blendMode = SHADER_BLEND_TRANSLUCENT;
	RenderPassContext defaultPassContext(rendPassRecorder, &defaultRender);

	g_matSystem->SetupDrawDefaultUP(PRIM_LINES, ArrayCRef(lines), defaultPassContext);

	pFont->SetupRenderText(EqString::Format("%.2f", (graph->maxValue*0.75f)).ToCString(), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT *0.75f), textStl, rendPassRecorder);
	pFont->SetupRenderText(EqString::Format("%.2f", (graph->maxValue*0.50f)).ToCString(), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT *0.50f), textStl, rendPassRecorder);
	pFont->SetupRenderText(EqString::Format("%.2f", (graph->maxValue*0.25f)).ToCString(), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT *0.25f), textStl, rendPassRecorder);

	int value_id = 0;

	Vertex2D graph_line_verts[GRAPH_MAX_VALUES*2];
	int num_line_verts = 0;

	float graph_max_value = 1.0f;

	int length = graph->values.numElem();
	while(length-- > 0)
	{
		//if(graph->points.getCurrent() > graph->fMaxValue)
		//	graph->fMaxValue = graph->points.getCurrent();

		const int graphIdx = (graph->cursor + length) % graph->values.numElem();
		DbgGraphBucket::DbgGraphValue& graphVal = graph->values[graphIdx];

		// get a value of it.
		float value = clamp(graphVal.value, 0.0f, graph->maxValue);

		if(graphVal.value > graph_max_value )
			graph_max_value = graphVal.value;

		value /= graph->maxValue;

		value *= GRAPH_HEIGHT;

		Vector2D point(x_pos + GRAPH_MAX_VALUES - value_id, y_pos - value);

		if(value_id > 0 && num_line_verts < GRAPH_MAX_VALUES*2)
		{
			graph_line_verts[num_line_verts].position = last_point;
			graph_line_verts[num_line_verts].color = graphVal.color;

			num_line_verts++;

			graph_line_verts[num_line_verts].position = point;
			graph_line_verts[num_line_verts].color = graphVal.color;

			num_line_verts++;
		}

		value_id++;

		last_point = point;
	}

	if(graph->dynamic)
		graph->maxValue = graph_max_value;

	defaultRender.blendMode = SHADER_BLEND_NONE;
	g_matSystem->SetupDrawDefaultUP(PRIM_LINES, ArrayCRef(graph_line_verts, num_line_verts), defaultPassContext);

	graph->remainingTime -= frame_time;

	if(graph->remainingTime <= 0)
		graph->remainingTime = 0.0f;

}

static void DrawPolygons(ArrayRef<DebugPolyNode_t> polygons, float frameTime, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!polygons.numElem())
		return;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.cullMode = CULL_BACK;
	defaultRenderPass.depthTest = true;
	defaultRenderPass.depthWrite = false;

	RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLES);

		for(int i = 0; i < polygons.numElem(); i++)
		{
			if (i > 0 && (i % POLYS_DRAW_SUBDIV) == 0)
			{
				if (meshBuilder.End(drawCmd))
					g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);

				// start with new mesh
				meshBuilder.Init(g_matSystem->GetDynamicMesh());
				meshBuilder.Begin(PRIM_TRIANGLES);
			}

			meshBuilder.Color4(polygons[i].color);

			for(int j = 0; j < polygons[i].verts.numElem()-2; ++j)
			{
				meshBuilder.Triangle3(polygons[i].verts[0], polygons[i].verts[j+1], polygons[i].verts[j+2]);
			}

			polygons[i].lifetime -= frameTime;
		}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);

	meshBuilder.Begin(PRIM_LINES);
		for(int i = 0; i < polygons.numElem(); i++)
		{
			if (i > 0 && (i % LINES_DRAW_SUBDIV) == 0)
			{
				if (meshBuilder.End(drawCmd))
					g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);

				// start with new mesh
				meshBuilder.Init(g_matSystem->GetDynamicMesh());
				meshBuilder.Begin(PRIM_LINES);
			}

			meshBuilder.Color4(polygons[i].color);

			for (int j = 0; j < polygons[i].verts.numElem(); ++j)
			{
				meshBuilder.Line3fv(polygons[i].verts[j], polygons[i].verts[(j+1) % polygons[i].verts.numElem()]);
			}
		}
	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
}

static bool SortWindingIndices(ArrayCRef<Vector3D> vertices, ArrayRef<int> indices, const Plane& plane)
{
	if (indices.numElem() < 3)
		return false;

	// get the center of that poly
	const float countFac = 1.0f / (float)indices.numElem();

	Vector3D center(0);
	for (int i = 0; i < indices.numElem(); i++)
		center += vertices[indices[i]] * countFac;

	// sort them now
	for (int i = 0; i < indices.numElem() - 2; i++)
	{
		const Vector3D iVert = vertices[indices[i]];

		float smallestAngle = -1;
		int smallestIdx = -1;

		Vector3D a = normalize(iVert - center);
		Plane p(iVert, center, center + plane.normal, true);

		for (int j = i + 1; j < indices.numElem(); j++)
		{
			const Vector3D jVert = vertices[indices[j]];

			// the vertex is in front or lies on the plane
			if (p.ClassifyPoint(jVert) != CP_BACK)
			{
				const Vector3D b = normalize(jVert - center);
				const float cosAngle = dot(b, a);
				if (cosAngle > smallestAngle)
				{
					smallestAngle = cosAngle;
					smallestIdx = j;
				}
			}
		}

		if (smallestIdx == -1)
			return false;

		// swap the indices
		QuickSwap(indices[smallestIdx], indices[i + 1]);
	}

	return true;
}

const void DebugDrawVolume(const ArrayCRef<Plane>& volume, const MColor& color)
{
	Array<Vector3D> tempVerts(PP_SL);
	Map<int, Set<int>> intersectedWindings(PP_SL);

	const int numPlanes = volume.numElem();

	for (int i = 0; i < numPlanes; ++i)
	{
		const Plane& iPlane = volume[i];
		for (int j = i + 1; j < numPlanes; ++j)
		{
			const Plane& jPlane = volume[j];
			for (int k = j + 1; k < numPlanes; ++k)
			{
				const Plane& kPlane = volume[k];

				Vector3D point;
				if (!iPlane.GetIntersectionWithPlanes(jPlane, kPlane, point))
					continue;

				const int vertexId = tempVerts.addUnique(point, [](const Vector3D& a, const Vector3D& b) {
					constexpr float VERTEX_PRECISION = 0.001f;
					return vecSimilar(a, b, VERTEX_PRECISION);
				});

				Set<int>& windingIDs = intersectedWindings[vertexId];
				windingIDs.insert(i);
				windingIDs.insert(j);
				windingIDs.insert(k);
			}
		}
	}

	struct Winding
	{
		Array<int> indices{ PP_SL };
	};

	Array<Vector3D> windingVerts(PP_SL);
	Array<Winding> windings(PP_SL);
	windings.setNum(numPlanes);

	for (int i = 0; i < tempVerts.numElem(); ++i)
	{
		const Vector3D point = tempVerts[i];
		if (!Volume::IsSphereInside(volume, point, 0.01f))
			continue;

		const int vertexId = windingVerts.numElem();
		windingVerts.append(point);

		const Set<int>& windingIDs = intersectedWindings[i];
		for (auto it = windingIDs.begin(); !it.atEnd(); ++it)
		{
			Winding& wnd = windings[it.key()];
			wnd.indices.append(vertexId);
		}
	}

	int planeIdx = 0;
	for (Winding& wnd : windings)
	{
		SortWindingIndices(windingVerts, wnd.indices, volume[planeIdx++]);
		if (wnd.indices.numElem() < 3)
			continue;

		DbgPoly builder;
		builder.Color(color);

		for (int i = 0; i < wnd.indices.numElem(); ++i)
			builder.Point(windingVerts[wnd.indices[i]]);
	}
}

static void DrawVolumes(ArrayRef<DebugVolumeNode_t> volumes, float frameTime, IGPURenderPassRecorder* rendPassRecorder)
{
	for (int i = 0; i < volumes.numElem(); i++)
	{
		DebugDrawVolume(volumes[i].planes, volumes[i].color);
		volumes[i].lifetime -= frameTime;
	}
}

static Vector3D v3sphere(float theta, float phi)
{
	return Vector3D(
		cos(theta) * cos(phi),
		sin(theta) * cos(phi),
		sin(phi));
}

// use PRIM_LINES
static void DrawSphereWireframe(CMeshBuilder& meshBuilder, DebugSphereNode_t& sphere, int sides)
{
	if (sphere.radius <= 0)
		return;

	meshBuilder.Color4(sphere.color);

	sides = sides + (sides % 2);

	const double oneBySides = 1.0 / sides;

	{
		for (int i = 0; i < sides; i++)
		{
			const double ds = sin((i * 2 * M_PI_D) * oneBySides);
			const double dc = cos((i * 2 * M_PI_D) * oneBySides);

			meshBuilder.Position3f(
						static_cast<float>(sphere.origin.x + sphere.radius * dc),
						static_cast<float>(sphere.origin.y + sphere.radius * ds),
						sphere.origin.z
						);

			meshBuilder.AdvanceVertex();
		}
	}

	{
		for (int i = 0; i < sides; i++)
		{
			const double ds = sin((i * 2 * M_PI_D) * oneBySides);
			const double dc = cos((i * 2 * M_PI_D) * oneBySides);

			meshBuilder.Position3f(
						static_cast<float>(sphere.origin.x + sphere.radius * dc),
						sphere.origin.y,
						static_cast<float>(sphere.origin.z + sphere.radius * ds)
						);

			meshBuilder.AdvanceVertex();
		}
	}

	{
		for (int i = 0; i < sides; i++)
		{
			const double ds = sin((i * 2 * M_PI_D) * oneBySides);
			const double dc = cos((i * 2 * M_PI_D) * oneBySides);

			meshBuilder.Position3f(
						sphere.origin.x,
						static_cast<float>(sphere.origin.y + sphere.radius * dc),
						static_cast<float>(sphere.origin.z + sphere.radius * ds)
						);

			meshBuilder.AdvanceVertex();
		}
	}
}

// use PRIM_TRIANGLES
static void DrawSphereFilled(CMeshBuilder& meshBuilder, DebugSphereNode_t& sphere, int sides, IGPURenderPassRecorder* rendPassRecorder)
{
	if (sphere.radius <= 0)
		return;

	const float dt = M_PI_D *2.0f / float(sides);
	const float dp = M_PI_D / float(sides);

	meshBuilder.Color4(sphere.color);

	for (int i = 0; i <= sides - 1; i++)
	{
		for (int j = 0; j <= sides - 2; j++)
		{
			const double t = i * dt;
			const double p = (j * dp) - (M_PI_D * 0.5f);

			{
				Vector3D v(sphere.origin + (v3sphere(t, p) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t, p + dp) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t + dt, p + dp) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t, p) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t + dt, p + dp) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t + dt, p) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}
		}
	}

	{
		const double p = (sides - 1) * dp - (M_PI_D * 0.5f);
		for (int i = 0; i <= sides - 1; i++)
		{
			const double t = i * dt;

			{
				Vector3D v(sphere.origin + (v3sphere(t, p) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t + dt, p + dp) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}

			{
				Vector3D v(sphere.origin + (v3sphere(t + dt, p) * sphere.radius));
				meshBuilder.Position3fv(v);
				meshBuilder.AdvanceVertex();
			}
		}
	}
}

static void DrawSphereArray(ArrayRef<DebugSphereNode_t> spheres, float frameTime, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!spheres.numElem())
		return;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;
	defaultRenderPass.depthWrite = false;

	RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

	for (int i = 0; i < spheres.numElem(); i++)
	{
		DrawSphereWireframe(meshBuilder, spheres[i], 20);
		spheres[i].lifetime -= frameTime;
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
}

#endif // ENABLE_DEBUG_DRAWING

void CDebugOverlay::SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view )
{
	m_projMat = proj;
	m_viewMat = view;

	Matrix4x4 viewProj = m_projMat*m_viewMat;
	m_frustum.LoadAsFrustum(viewProj);
}

void CDebugOverlay::Draw(int winWide, int winTall, float timescale)
{
	m_frameTime = m_timer.GetTime(true) * timescale;

#ifdef ENABLE_DEBUG_DRAWING
	IGPURenderPassRecorderPtr rendPassRecorder = g_renderAPI->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(g_matSystem->GetCurrentBackbuffer())
		.DepthStencilTarget(g_matSystem->GetDefaultDepthBuffer())
		.End()
	);

	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, m_projMat);
	g_matSystem->SetMatrix(MATRIXMODE_VIEW, m_viewMat);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	CleanOverlays();

	// draw custom stuff
	{
		Threading::CScopedMutex m(s_debugOverlayMutex);

		for (int i = 0; i < m_draw3DFuncs.numElem(); i++)
		{
			if (!m_draw3DFuncs[i].func(rendPassRecorder))
			{
				m_draw3DFuncs.fastRemoveIndex(i);
				--i;
				continue;
			}
			m_draw3DFuncs[i].lifetime -= m_frameTime;
		}
	}

	// may need a reset again
	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, m_projMat);
	g_matSystem->SetMatrix(MATRIXMODE_VIEW, m_viewMat);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	// draw all of 3d stuff
	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawBoxArray(m_BoxList, m_frameTime, rendPassRecorder);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawCylinderArray(m_CylinderList, m_frameTime, rendPassRecorder);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawOrientedBoxArray(m_OrientedBoxList, m_frameTime, rendPassRecorder);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawVolumes(m_volumes, m_frameTime, rendPassRecorder);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawSphereArray(m_SphereList, m_frameTime, rendPassRecorder);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawLineArray(m_LineList, m_frameTime, rendPassRecorder);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);
		DrawPolygons(m_polygons, m_frameTime, rendPassRecorder);
	}

	// now rendering 2D stuff
	g_matSystem->Setup2D(winWide, winTall);

	const Vector2D drawFadedTextBoxPosition = Vector2D(15,45);
	const Vector2D drawTextBoxPosition = Vector2D(15,45);

	eqFontStyleParam_t textStl;
	textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);

		using TextIterator = List< DebugFadingTextNode_t>::Iterator;
		Array<TextIterator> deletedNodes(PP_SL);

		int n = 0;
		for(auto it = m_LeftTextFadeArray.begin(); !it.atEnd(); ++it)
		{
			DebugFadingTextNode_t& current = *it;
			if (current.lifetime < 0.0f)
			{
				deletedNodes.append(it);
				continue;
			}
			current.lifetime -= m_frameTime;

			MColor curColor(current.color);
			if (current.initialLifetime > 0.05f)
				curColor.a = clamp(current.lifetime, 0.0f, 1.0f);
			else
				curColor.a = 1.0f;

			textStl.textColor = curColor;

			const Vector2D textPos = drawFadedTextBoxPosition + Vector2D(0, (n * m_debugFont->GetLineHeight(textStl)));
			m_debugFont->SetupRenderText(current.pszText.GetData(), textPos, textStl, rendPassRecorder);

			++n;
		}

		for (TextIterator it : deletedNodes)
			m_LeftTextFadeArray.remove(it);
	}

	{
		Threading::CScopedMutex m(s_debugOverlayMutex);

		for (DebugText3DNode_t& current : m_Text3DArray)
		{
			current.lifetime -= m_frameTime;

			Vector3D screen(0);
			const bool behind = PointToScreen(current.origin, screen, m_projMat * m_viewMat, Vector2D(winWide, winTall));
			if (behind)
				continue;

			if (screen.z > current.dist)
				continue;

			textStl.textColor = current.color;
			m_debugFont2->SetupRenderText(current.pszText.GetData(), screen.xy(), textStl, rendPassRecorder);
		}
	}

	if(r_debugDrawFrameStats.GetBool())
	{
		{
			Threading::CScopedMutex m(s_debugOverlayMutex);
			if (m_TextArray.numElem())
			{
				GUIDrawWindow(AARectangle(drawTextBoxPosition.x, drawTextBoxPosition.y, drawTextBoxPosition.x + 380, drawTextBoxPosition.y + (m_TextArray.numElem() * m_debugFont->GetLineHeight(textStl))), MColor(0.5f, 0.5f, 0.5f, 0.5f), rendPassRecorder);

				int n = 0;
				for (DebugTextNode_t& current : m_TextArray)
				{
					const Vector2D textPos(drawTextBoxPosition.x, drawTextBoxPosition.y + (n * m_debugFont->GetLineHeight(textStl)));

					textStl.textColor = current.color;
					m_debugFont->SetupRenderText(current.pszText.GetData(), textPos, textStl, rendPassRecorder);
					++n;
				}
			}
			m_TextArray.clear();
		}

		eqFontStyleParam_t rTextFadeStyle = textStl;
		rTextFadeStyle.align = TEXT_ALIGN_RIGHT;

		{
			Threading::CScopedMutex m(s_debugOverlayMutex);

			int n = 0;
			for (DebugFadingTextNode_t& current : m_RightTextFadeArray)
			{
				current.lifetime -= m_frameTime;

				MColor curColor(current.color);
				if (current.initialLifetime > 0.05f)
					curColor.a = clamp(current.lifetime, 0.0f, 1.0f);
				else
					curColor.a = 1.0f;

				rTextFadeStyle.textColor = curColor;

				const float textLen = m_debugFont->GetStringWidth(current.pszText.ToCString(), textStl);
				const Vector2D textPos(winWide - (textLen * m_debugFont->GetLineHeight(textStl)), 45 + (n * m_debugFont->GetLineHeight(textStl)));

				m_debugFont->SetupRenderText(current.pszText.GetData(), textPos, rTextFadeStyle, rendPassRecorder);
				++n;
			}
		}
	}

	if(r_debugDrawGraphs.GetBool())
	{
		Threading::CScopedMutex m(s_debugOverlayMutex);

		for(int i = 0; i < m_graphbuckets.numElem(); i++)
			DrawGraph( m_graphbuckets[i], i, m_debugFont, m_frameTime, rendPassRecorder);

		m_graphbuckets.clear(false);
	}

	// draw custom stuff
	{
		Threading::CScopedMutex m(s_debugOverlayMutex);

		for (int i = 0; i < m_draw2DFuncs.numElem(); i++)
		{
			DebugDrawFunc_t& drawFunc = m_draw2DFuncs[i];
			if (!drawFunc.func(rendPassRecorder))
			{
				m_draw2DFuncs.fastRemoveIndex(i);
				--i;
				continue;
			}
			drawFunc.lifetime -= m_frameTime;
		}
	}

	if(m_dbgTexture)
	{
		// destroy texture if debug overlay is the only one owning it
		if (m_dbgTexture->Ref_Count() == 1)
		{
			m_dbgTexture = nullptr;
			return;
		}

		g_matSystem->Setup2D( winWide, winTall );

		float w = (float)m_dbgTexture->GetWidth() * r_debugShowTextureScale.GetFloat();
		float h = (float)m_dbgTexture->GetHeight() * r_debugShowTextureScale.GetFloat();

		if(h > winTall)
		{
			float fac = (float)winTall / h;
			w *= fac;
			h *= fac;
		}

		Vertex2D rect[] = { MAKETEXQUAD(0, 0, w, h, 0) };

		const int flags = m_dbgTexture->GetFlags();
		const bool isCubemap = (flags & TEXFLAG_CUBEMAP);

		const int cubeOrArrayIndex = (m_frameId / 30) % (isCubemap ? 6 : m_dbgTexture->GetArraySize());
		const int viewIndex = isCubemap ? ITexture::ViewArraySlice(cubeOrArrayIndex) : ITexture::DEFAULT_VIEW;

		MatSysDefaultRenderPass defaultRender;
		defaultRender.texture = TextureView(m_dbgTexture, viewIndex);
		g_matSystem->SetupDrawDefaultUP(PRIM_TRIANGLE_STRIP, ArrayCRef(rect), RenderPassContext(rendPassRecorder, &defaultRender));

		eqFontStyleParam_t textStl;
		textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

		EqString str = EqString::Format("%dx%d (frame %d)\n%s\nrefcnt %d", m_dbgTexture->GetWidth(), m_dbgTexture->GetHeight(), m_dbgTexture->GetAnimationFrame(), m_dbgTexture->GetName(), m_dbgTexture->Ref_Count());
		m_debugFont2->SetupRenderText(str, Vector2D(10, 10), textStl, rendPassRecorder);
	}

	++m_frameId;

	g_matSystem->QueueCommandBuffer(rendPassRecorder->End());
#endif
}

bool CDebugOverlay::CheckNodeLifetime(DebugNodeBase& node)
{
	// don't touch newly added nodes
	if (node.frameindex == m_frameId)
		return true;

	// expired.
	if (node.lifetime < 0.0f)
		return false;

	if (node.nameHash == 0)
		return true;

	// check if it's been replaced
	auto found = m_newNames.find(node.nameHash);
	if (found.atEnd())
		return true;

	// check node time stamp, we allow to put different objects under similar name in one frame
	return found.value() == node.frameindex;
}

void CDebugOverlay::CleanOverlays()
{
#ifdef ENABLE_DEBUG_DRAWING
	Threading::CScopedMutex m(s_debugOverlayMutex);

	for (int i = 0; i < m_draw2DFuncs.numElem(); i++)
	{
		if (!CheckNodeLifetime(m_draw2DFuncs[i]))
		{
			m_draw2DFuncs.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_draw3DFuncs.numElem(); i++)
	{
		if (!CheckNodeLifetime(m_draw3DFuncs[i]))
		{
			m_draw3DFuncs.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_RightTextFadeArray.numElem(); i++)
	{
		if (m_RightTextFadeArray[i].lifetime <= 0)
		{
			m_RightTextFadeArray.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_Text3DArray.numElem(); i++)
	{
		if(!CheckNodeLifetime(m_Text3DArray[i]))
		{
			m_Text3DArray.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_LineList.numElem(); i++)
	{
		if(!CheckNodeLifetime(m_LineList[i]))
		{
			m_LineList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_BoxList.numElem(); i++)
	{
		if(!CheckNodeLifetime(m_BoxList[i]))
		{
			m_BoxList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_CylinderList.numElem(); i++)
	{
		if (!CheckNodeLifetime(m_CylinderList[i]))
		{
			m_CylinderList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_OrientedBoxList.numElem(); i++)
	{
		if (!CheckNodeLifetime(m_OrientedBoxList[i]))
		{
			m_OrientedBoxList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_SphereList.numElem(); i++)
	{
		if(!CheckNodeLifetime(m_SphereList[i]))
		{
			m_SphereList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_polygons.numElem(); i++)
	{
		if(!CheckNodeLifetime(m_polygons[i]))
		{
			m_polygons.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0; i < m_volumes.numElem(); i++)
	{
		if (!CheckNodeLifetime(m_volumes[i]))
		{
			m_volumes.fastRemoveIndex(i);
			i--;
		}
	}

#endif
}

void CDebugOverlay::Graph_DrawBucket(DbgGraphBucket* pBucket)
{
#ifdef ENABLE_DEBUG_DRAWING
	if (!r_debugDrawGraphs.GetBool())
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	m_graphbuckets.addUnique(pBucket);
#endif
}

void CDebugOverlay::Graph_AddValue(DbgGraphBucket* bucket, float value)
{
#ifdef ENABLE_DEBUG_DRAWING
	if(!r_debugDrawGraphs.GetBool())
		return;

	Threading::CScopedMutex m(s_debugOverlayMutex);

	if(bucket->remainingTime <= 0)
	{
		int index;
		if (bucket->values.numElem() < GRAPH_MAX_VALUES)
		{
			index = bucket->values.append(DbgGraphBucket::DbgGraphValue{ value, MColor(bucket->color).pack() });
		}
		else
		{
			index = bucket->cursor;
			bucket->values[index] = DbgGraphBucket::DbgGraphValue{ value, MColor(bucket->color).pack() };
		}
		index++;
		bucket->cursor = index % GRAPH_MAX_VALUES;

		bucket->remainingTime = bucket->updateTime;
	}
#endif
}

IEqFont* CDebugOverlay::GetFont()
{
	return m_debugFont;
}

