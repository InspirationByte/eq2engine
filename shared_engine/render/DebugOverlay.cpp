//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//////////////////////////////////////////////////////////////////////////////////


#include "DebugOverlay.h"

#include "core/DebugInterface.h"
#include "core/ConVar.h"

#include "utils/strtools.h"

#include "math/Utility.h"
#include "font/IFontCache.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#ifndef _WIN32
#include <stdarg.h>
#endif

#define BOXES_DRAW_SUBDIV (64)
#define LINES_DRAW_SUBDIV (128)
#define POLYS_DRAW_SUBDIV (64)

static CDebugOverlay g_DebugOverlays;
IDebugOverlay* debugoverlay = (IDebugOverlay*)&g_DebugOverlays;

static ConVar r_drawFrameStats("r_frameStats", "0", NULL, CV_ARCHIVE);
static ConVar r_debugdrawGraphs("r_debugDrawGraphs", "0", NULL, CV_ARCHIVE);
static ConVar r_debugdrawShapes("r_debugDrawShapes", "0", NULL, CV_ARCHIVE);
static ConVar r_debugdrawLines("r_debugDrawLines", "0", NULL, CV_ARCHIVE);

ITexture* g_pDebugTexture = NULL;

void OnShowTextureChanged(ConVar* pVar,char const* pszOldValue)
{
	if(g_pDebugTexture)
		g_pShaderAPI->FreeTexture(g_pDebugTexture);

	g_pDebugTexture = g_pShaderAPI->FindTexture( pVar->GetString() );

	if(g_pDebugTexture)
		g_pDebugTexture->Ref_Grab();
}

ConVar r_showTexture("r_debug_showTexture", "", OnShowTextureChanged, "input texture name to show texture. To hide view input anything else.", CV_CHEAT);
ConVar r_showTextureScale("r_debug_textureScale", "1.0", NULL, CV_ARCHIVE);

#include "math/Rectangle.h"

void GUIDrawWindow(const Rectangle_t &rect, const ColorRGBA &color1)
{
	ColorRGBA color2(0.2,0.2,0.2,0.8);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(NULL,0,0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false,false);

	materials->BindMaterial(materials->GetDefaultMaterial());

	Vector2D r0[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -0.5f) };
	Vector2D r1[] = { MAKEQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -0.5f) };
	Vector2D r2[] = { MAKEQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -0.5f) };
	Vector2D r3[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -0.5f) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
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
	meshBuilder.End();
}


CDebugOverlay::CDebugOverlay() :
	m_TextArray(64),
	m_Text3DArray(32),
	m_RightTextFadeArray(32),
	m_BoxList(32),
	m_LineList(32),
	m_FastBoxList(128),
	m_FastLineList(128),
	m_graphbuckets(4),
	m_polygons(128),
	m_frameTime(0.0f)
{
	m_pDebugFont = NULL;
}

void CDebugOverlay::Init(bool hidden)
{
	if (!hidden)
	{
		r_drawFrameStats.SetBool(true);
		r_debugdrawGraphs.SetBool(true);
		r_debugdrawShapes.SetBool(true);
		r_debugdrawLines.SetBool(true);
	}

	m_pDebugFont = g_fontCache->GetFont("debug", 0);
}

void CDebugOverlay::Text(const ColorRGBA &color, char const *fmt,...)
{
	if(!r_drawFrameStats.GetBool())
		return;

	DebugTextNode_t textNode;
	textNode.color = color;

	va_list		argptr;
	static char	string[1024];

	va_start (argptr,fmt);
	vsnprintf(string,sizeof(string), fmt,argptr);
	va_end (argptr);

	textNode.pszText = string;

	m_TextArray.append(textNode);
}

void CDebugOverlay::Text3D(const Vector3D &origin, float dist, const ColorRGBA &color, float fTime, char const *fmt,...)
{
	if(!r_drawFrameStats.GetBool())
		return;

	if(!m_frustum.IsSphereInside(origin, 1.0f))
		return;

	va_list		argptr;
	static char	string[1024];

	va_start (argptr,fmt);
	vsnprintf(string,sizeof(string), fmt,argptr);
	va_end (argptr);

	DebugText3DNode_t textNode;
	textNode.color = color;
	textNode.origin = origin;
	textNode.dist = dist;
	textNode.lifetime = fTime;

	textNode.pszText = string;

	m_Text3DArray.append(textNode);
}

#define MAX_MINICON_MESSAGES 32

void CDebugOverlay::TextFadeOut(int position, const ColorRGBA &color,float fFadeTime, char const *fmt, ...)
{
	if(position == 1)
	{
		if(!r_drawFrameStats.GetBool())
			return;
	}

	DebugFadingTextNode_t textNode;
	textNode.color = color;
	textNode.lifetime = fFadeTime;
	textNode.initialLifetime = fFadeTime;

	va_list		argptr;
	static char	string[1024];

	va_start (argptr,fmt);
	vsnprintf(string,sizeof(string), fmt,argptr);
	va_end (argptr);

	textNode.pszText = string;

	if(position == 0)
	{
		m_LeftTextFadeArray.addLast(textNode);

		if(m_LeftTextFadeArray.getCount() > MAX_MINICON_MESSAGES && m_LeftTextFadeArray.goToFirst())
			m_LeftTextFadeArray.removeCurrent();
	}
	else
		m_RightTextFadeArray.append(textNode);
}

void CDebugOverlay::Box3D(const Vector3D &mins, const Vector3D &maxs, const ColorRGBA &color, float fTime)
{
	Threading::CScopedMutex m(m_mutex);

	if(!r_debugdrawShapes.GetBool())
		return;

	if(!m_frustum.IsBoxInside(mins,maxs))
		return;

	DebugBoxNode_t box;
	box.mins = mins;
	box.maxs = maxs;
	box.color = color;
	box.lifetime = fTime;

	if(fTime == 0.0f)
		m_FastBoxList.append(box);
	else
		m_BoxList.append(box);
}

void CDebugOverlay::Line3D(const Vector3D &start, const Vector3D &end, const ColorRGBA &color1, const ColorRGBA &color2, float fTime)
{
	Threading::CScopedMutex m(m_mutex);

	if(!r_debugdrawLines.GetBool())
		return;

	if(!m_frustum.IsBoxInside(start,end))
		return;

	DebugLineNode_t line;
	line.start = start;
	line.end = end;
	line.color1 = color1;
	line.color2 = color2;
	line.lifetime = fTime;

	if(fTime == 0.0f)
		m_FastLineList.append(line);
	else
		m_LineList.append(line);
}
/*
void CDebugOverlay::OrientedBox3D(const Vector3D &mins, const Vector3D &maxs, Matrix4x4& transform, const ColorRGBA &color, float fTime)
{
	Threading::CScopedMutex m(m_mutex);

	if(!r_debugdrawShapes.GetBool())
		return;

	//if(!m_frustum.IsBoxInside(mins,maxs))
	//	return;

	DebugOriBoxNode_t box;
	box.mins = mins;
	box.maxs = maxs;
	box.transform = transform;
	box.color = color;
	box.lifetime = fTime;

	if(fTime == 0.0f)
		m_FastOrientedBoxList.append(box);
	else
		m_OrientedBoxList.append(box);
}*/

void CDebugOverlay::Sphere3D(const Vector3D& position, float radius, const ColorRGBA &color, float fTime)
{
	Threading::CScopedMutex m(m_mutex);

	if(!r_debugdrawShapes.GetBool())
		return;

	if(!m_frustum.IsSphereInside(position, radius))
		return;

	DebugSphereNode_t sphere;
	sphere.origin = position;
	sphere.radius = radius;
	sphere.color = color;
	sphere.lifetime = fTime;

	if(fTime <= 0.0f)
		m_FastSphereList.append(sphere);
	else
		m_SphereList.append(sphere);
}

void CDebugOverlay::Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime)
{
	Threading::CScopedMutex m(m_mutex);

	if(!m_frustum.IsTriangleInside(v0,v1,v2))
		return;

	DebugPolyNode_t poly;
	poly.v0 = v0;
	poly.v1 = v1;
	poly.v2 = v2;

	poly.color = color;
	poly.lifetime = fTime;

	m_polygons.append(poly);
}

void CDebugOverlay::Draw3DFunc( OnDebugDrawFn func, void* args )
{
	Threading::CScopedMutex m(m_mutex);

	DebugDrawFunc_t fn = {func, args};
	m_drawFuncs.append(fn);
}

void DrawLineArray(DkList<DebugLineNode_t>& lines, float frametime)
{
	if(!lines.numElem())
		return;

	g_pShaderAPI->SetTexture(NULL,NULL, 0);

	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
	materials->SetDepthStates(true, false);

	materials->Apply();

	// bind the default material as we're emulating an FFP
	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

		for(int i = 0; i < lines.numElem(); i++)
		{
			DebugLineNode_t& line = lines[i];

			meshBuilder.Color4fv(line.color1);
			meshBuilder.Position3fv(line.start);

			meshBuilder.AdvanceVertex();

			meshBuilder.Color4fv(line.color2);
			meshBuilder.Position3fv(line.end);

			meshBuilder.AdvanceVertex();

			line.lifetime -= frametime;
		}

	meshBuilder.End();
}

void DrawBoxArray(DkList<DebugBoxNode_t>& boxes, float frametime)
{
	if(!boxes.numElem())
		return;

	g_pShaderAPI->SetTexture(NULL,NULL, 0);

	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
	materials->SetDepthStates(true,false);

	materials->Apply();

	// bind the default material as we're emulating an FFP
	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

		for(int i = 0; i < boxes.numElem(); i++)
		{
			DebugBoxNode_t& node = boxes[i];

			meshBuilder.Color4fv(node.color);

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

			if((i % BOXES_DRAW_SUBDIV) == 0)
			{
				meshBuilder.End();
				meshBuilder.Begin(PRIM_LINES);
			}
		}

	meshBuilder.End();
}

void DrawGraph(debugGraphBucket_t* graph, int position, IEqFont* pFont, float frame_time)
{
	const float GRAPH_HEIGHT = 100;
	const float GRAPH_Y_OFFSET = 50;

	float x_pos = 15;
	float y_pos = GRAPH_Y_OFFSET + GRAPH_HEIGHT + position*110;

	Vector2D last_point(-1);

	Vertex2D_t lines[] =
	{
		Vertex2D_t(Vector2D(x_pos, y_pos), vec2_zero),
		Vertex2D_t(Vector2D(x_pos, y_pos - GRAPH_HEIGHT), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+400, y_pos), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos - GRAPH_HEIGHT *0.75f), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+32, y_pos - GRAPH_HEIGHT *0.75f), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos - GRAPH_HEIGHT *0.50f), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+32, y_pos - GRAPH_HEIGHT *0.50f), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos - GRAPH_HEIGHT *0.25f), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+32, y_pos - GRAPH_HEIGHT *0.25f), vec2_zero),
	};

	eqFontStyleParam_t textStl;
	textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

	pFont->RenderText(graph->pszName, Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT - 16), textStl);

	pFont->RenderText("0", Vector2D(x_pos + 5, y_pos), textStl);
	pFont->RenderText(varargs("%.2f", graph->maxValue), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT), textStl);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_LINES,lines,elementsOf(lines), NULL, color4_white, &blending);

	pFont->RenderText(varargs("%.2f", (graph->maxValue*0.75f)), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT *0.75f), textStl);
	pFont->RenderText(varargs("%.2f", (graph->maxValue*0.50f)), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT *0.50f), textStl);
	pFont->RenderText(varargs("%.2f", (graph->maxValue*0.25f)), Vector2D(x_pos + 5, y_pos - GRAPH_HEIGHT *0.25f), textStl);

	int value_id = 0;

	Vertex2D_t graph_line_verts[200];
	int num_line_verts = 0;

	float graph_max_value = 1.0f;

	if(graph->points.goToFirst())
	{
		do
		{
			//if(graph->points.getCurrent() > graph->fMaxValue)
			//	graph->fMaxValue = graph->points.getCurrent();

			graphPoint_t& graphVal = graph->points.getCurrent();

			// get a value of it.
			float value = clamp(graphVal.value, 0.0f, graph->maxValue);

			if(graphVal.value > graph_max_value )
				graph_max_value = graphVal.value;

			value /= graph->maxValue;

			value *= GRAPH_HEIGHT;

			Vector2D point(x_pos + 400 - (4*value_id), y_pos - value);

			if(value_id > 0)
			{
				graph_line_verts[num_line_verts].position = last_point;
				graph_line_verts[num_line_verts].color = ColorRGBA(graphVal.color, 1);

				num_line_verts++;

				graph_line_verts[num_line_verts].position = point;
				graph_line_verts[num_line_verts].color = ColorRGBA(graphVal.color, 1);

				num_line_verts++;
				//g_pShaderAPI->DrawSetColor(graph->color);
				//g_pShaderAPI->DrawLine2D(point, last_point);
			}

			value_id++;

			last_point = point;

		}while(graph->points.goToNext());
	}

	if(graph->dynamic)
		graph->maxValue = graph_max_value;

	materials->DrawPrimitives2DFFP(PRIM_LINES,graph_line_verts,num_line_verts, NULL, color4_white);

	graph->remainingTime -= frame_time;

	if(graph->remainingTime <= 0)
		graph->remainingTime = 0.0f;

}

void DrawPolygons(DkList<DebugPolyNode_t>& polygons, float frameTime)
{
	if(!polygons.numElem())
		return;

	g_pShaderAPI->SetTexture(NULL,NULL, 0);

	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_BACK,FILL_SOLID);
	materials->SetDepthStates(true, true);

	// bind the default material as we're emulating an FFP
	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLES);

		for(int i = 0; i < polygons.numElem(); i++)
		{
			meshBuilder.Color4fv(polygons[i].color);

			meshBuilder.Position3fv(polygons[i].v0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v2);
			meshBuilder.AdvanceVertex();

			polygons[i].lifetime -= frameTime;

			if((i % POLYS_DRAW_SUBDIV) == 0)
			{
				meshBuilder.End();
				meshBuilder.Begin(PRIM_TRIANGLES);
			}
		}

	meshBuilder.End();

	meshBuilder.Begin(PRIM_LINES);
		for(int i = 0; i < polygons.numElem(); i++)
		{
			meshBuilder.Color4fv(polygons[i].color);

			meshBuilder.Position3fv(polygons[i].v0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v2);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v2);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v0);
			meshBuilder.AdvanceVertex();

			if((i % LINES_DRAW_SUBDIV) == 0)
			{
				meshBuilder.End();
				meshBuilder.Begin(PRIM_LINES);
			}
		}
	meshBuilder.End();
}

Vector3D v3sphere(float theta, float phi)
{
	return Vector3D(
		cos(theta) * cos(phi),
		sin(theta) * cos(phi),
		sin(phi));
}

// use PRIM_LINES
void DrawSphereWireframe(CMeshBuilder& meshBuilder, DebugSphereNode_t& sphere, int sides)
{
	if (sphere.radius <= 0)
		return;

	meshBuilder.Color4fv(sphere.color);

	sides = sides + (sides % 2);

	{
		for (int i = 0; i < sides; i++)
		{
			double ds = sin((i * 2 * PI) / sides);
			double dc = cos((i * 2 * PI) / sides);

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
			double ds = sin((i * 2 * PI) / sides);
			double dc = cos((i * 2 * PI) / sides);

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
			double ds = sin((i * 2 * PI) / sides);
			double dc = cos((i * 2 * PI) / sides);

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
void DrawSphereFilled(CMeshBuilder& meshBuilder, DebugSphereNode_t& sphere, int sides)
{
	if (sphere.radius <= 0)
		return;

	float dt = PI*2.0f / float(sides);
	float dp = PI / float(sides);

	meshBuilder.Color4fv(sphere.color);

	for (int i = 0; i <= sides - 1; i++)
	{
		for (int j = 0; j <= sides - 2; j++)
		{
			const double t = i * dt;
			const double p = (j * dp) - (PI * 0.5f);

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
		const double p = (sides - 1) * dp - (PI * 0.5f);
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

void DrawSphereArray(DkList<DebugSphereNode_t>& spheres, float frameTime)
{
	if(!spheres.numElem())
		return;

	g_pShaderAPI->SetTexture(NULL,NULL, 0);

	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_BACK,FILL_SOLID);
	materials->SetDepthStates(true, true);

	// bind the default material as we're emulating an FFP
	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	//meshBuilder.Begin(PRIM_TRIANGLES);

	meshBuilder.Begin(PRIM_LINES);

	for (int i = 0; i < spheres.numElem(); i++)
	{
		DrawSphereWireframe(meshBuilder, spheres[i], 20);
		spheres[i].lifetime -= frameTime;
	}

	meshBuilder.End();
}

void CDebugOverlay::SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view )
{
	m_projMat = proj;
	m_viewMat = view;

	Matrix4x4 viewProj = m_projMat*m_viewMat;
	m_frustum.LoadAsFrustum(viewProj);
}

void CDebugOverlay::Draw(int winWide, int winTall)
{
	m_frameTime = m_timer.GetTime(true);

	materials->SetMatrix(MATRIXMODE_PROJECTION, m_projMat);
	materials->SetMatrix(MATRIXMODE_VIEW, m_viewMat);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->SetAmbientColor(1.0f);

	// draw custom stuff
	{
		Threading::CScopedMutex m(m_mutex);

		for(int i = 0; i < m_drawFuncs.numElem(); i++)
		{
			m_drawFuncs[i].func(m_drawFuncs[i].arg);
		}

		m_drawFuncs.clear();
	}

	// draw all of 3d stuff
	{
		Threading::CScopedMutex m(m_mutex);

		DrawBoxArray(m_BoxList, m_frameTime);
		DrawBoxArray(m_FastBoxList, m_frameTime);

		DrawSphereArray(m_SphereList, m_frameTime);
		DrawSphereArray(m_FastSphereList, m_frameTime);

		DrawLineArray(m_LineList, m_frameTime);
		DrawLineArray(m_FastLineList, m_frameTime);

		DrawPolygons(m_polygons, m_frameTime);
	}

	// now rendering 2D stuff
	materials->Setup2D(winWide, winTall);

#ifdef EDITOR
	const Vector2D drawFadedTextBoxPosition = Vector2D(5,5);
	const Vector2D drawTextBoxPosition = Vector2D(5,5);
#else
	const Vector2D drawFadedTextBoxPosition = Vector2D(15,45);
	const Vector2D drawTextBoxPosition = Vector2D(15,45);
#endif // EDITOR

	int idx = 0;

	if(m_LeftTextFadeArray.goToFirst())
	{
		Threading::CScopedMutex m(m_mutex);

		do
		{
			DebugFadingTextNode_t& current = m_LeftTextFadeArray.getCurrentNode()->object;

			if(current.lifetime < 0.0f)
			{
				m_LeftTextFadeArray.removeCurrent();
				continue;
			}

			if(current.initialLifetime > 0.05f)
				current.color.w = clamp(current.lifetime, 0.0f, 1.0f);
			else
				current.color.w = 1.0f;

			eqFontStyleParam_t textStl;
			textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
			textStl.textColor = current.color;

			Vector2D textPos = drawFadedTextBoxPosition + Vector2D( 0, (idx*m_pDebugFont->GetLineHeight(textStl)) );

			m_pDebugFont->RenderText( current.pszText.GetData(), textPos, textStl);

			idx++;

			current.lifetime -= m_frameTime;

		}while(m_LeftTextFadeArray.goToNext());
	}

	if(r_drawFrameStats.GetBool())
	{
		Threading::CScopedMutex m(m_mutex);

		eqFontStyleParam_t textStl;
		textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

		for (int i = 0;i < m_Text3DArray.numElem();i++)
		{
			DebugText3DNode_t& current = m_Text3DArray[i];

			Vector3D screen(0);

			bool beh = PointToScreen_Z(current.origin, screen, m_projMat * m_viewMat, Vector2D(winWide, winTall));

			bool visible = true;

			if(current.dist > 0)
				visible = (screen.z < current.dist);

			current.lifetime -= m_frameTime;

			if(!beh && visible)
			{
				textStl.textColor = current.color;
				m_pDebugFont->RenderText(current.pszText.GetData(), screen.xy(), textStl);
			}
		}

		if(m_TextArray.numElem())
		{
			GUIDrawWindow(Rectangle_t(drawTextBoxPosition.x,drawTextBoxPosition.y,drawTextBoxPosition.x+380,drawTextBoxPosition.y+(m_TextArray.numElem()*m_pDebugFont->GetLineHeight(textStl))),ColorRGBA(0.5f,0.5f,0.5f,0.5f));

			for (int i = 0;i < m_TextArray.numElem();i++)
			{
				DebugTextNode_t& current = m_TextArray[i];

				textStl.textColor = current.color;

				Vector2D textPos(drawTextBoxPosition.x,drawTextBoxPosition.y+(i*m_pDebugFont->GetLineHeight(textStl)));

				m_pDebugFont->RenderText(current.pszText.GetData(), textPos, textStl);
			}
		}

		eqFontStyleParam_t rTextFadeStyle;
		rTextFadeStyle.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
		rTextFadeStyle.align = TEXT_ALIGN_RIGHT;

		for (int i = 0;i < m_RightTextFadeArray.numElem();i++)
		{
			DebugFadingTextNode_t& current = m_RightTextFadeArray[i];

			float textLen = m_pDebugFont->GetStringWidth( current.pszText.c_str(), textStl );

			if(current.initialLifetime > 0.05f)
				current.color.w = clamp(current.lifetime, 0.0f, 1.0f);
			else
				current.color.w = 1.0f;

			rTextFadeStyle.textColor = current.color;
			Vector2D textPos(winWide - (textLen*m_pDebugFont->GetLineHeight(textStl)), 45+(i*m_pDebugFont->GetLineHeight(textStl)));

			m_pDebugFont->RenderText( current.pszText.GetData(), textPos, rTextFadeStyle);

			current.lifetime -= m_frameTime;
		}
	}

	if(r_debugdrawGraphs.GetBool())
	{
		Threading::CScopedMutex m(m_mutex);

		for(int i = 0; i < m_graphbuckets.numElem(); i++)
			DrawGraph( m_graphbuckets[i], i, m_pDebugFont, m_frameTime);

		m_graphbuckets.clear();
	}

	CleanOverlays();

	// more universal thing
	if( g_pDebugTexture )
	{
		materials->Setup2D( winWide, winTall );

		float w, h;
		w = (float)g_pDebugTexture->GetWidth()*r_showTextureScale.GetFloat();
		h = (float)g_pDebugTexture->GetHeight()*r_showTextureScale.GetFloat();

		if(h > winTall)
		{
			float fac = (float)winTall/h;

			w *= fac;
			h *= fac;
		}

		Vertex2D_t light_depth[] = { MAKETEXQUAD(0, 0, w, h, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, light_depth, elementsOf(light_depth),g_pDebugTexture);
	}
}

void CDebugOverlay::CleanOverlays()
{
	m_TextArray.clear();

	for (int i = 0;i < m_Text3DArray.numElem();i++)
	{
		if(m_Text3DArray[i].lifetime <= 0)
		{
			m_Text3DArray.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0;i < m_RightTextFadeArray.numElem();i++)
	{
		if(m_RightTextFadeArray[i].lifetime <= 0)
		{
			m_RightTextFadeArray.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0;i < m_LineList.numElem();i++)
	{
		if(m_LineList[i].lifetime <= 0)
		{
			m_LineList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0;i < m_BoxList.numElem();i++)
	{
		if(m_BoxList[i].lifetime <= 0)
		{
			m_BoxList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0;i < m_SphereList.numElem();i++)
	{
		if(m_SphereList[i].lifetime <= 0)
		{
			m_SphereList.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = 0;i < m_polygons.numElem();i++)
	{
		if(m_polygons[i].lifetime <= 0)
		{
			m_polygons.fastRemoveIndex(i);
			i--;
		}
	}

	m_FastSphereList.clear();
	m_FastBoxList.clear();
	m_FastLineList.clear();
}

void CDebugOverlay::Graph_DrawBucket(debugGraphBucket_t* pBucket)
{
	if (!r_debugdrawGraphs.GetBool())
		return;

	m_graphbuckets.addUnique(pBucket);
}

void CDebugOverlay::Graph_AddValue(debugGraphBucket_t* bucket, float value)
{
	if(!r_debugdrawGraphs.GetBool())
		return;

	if(bucket->remainingTime <= 0)
	{
		if (bucket->points.getCount() >= 100)
		{
			if (bucket->points.goToLast())
				bucket->points.removeCurrent();
		}

		bucket->remainingTime = bucket->updateTime;
		bucket->points.addFirst(graphPoint_t{value, bucket->color});
	}
}

IEqFont* CDebugOverlay::GetFont()
{
	return m_pDebugFont;
}

#define BBOX_STRIP_VERTS(min, max) \
	Vector3D(min.x, max.y, max.z),\
	Vector3D(max.x, max.y, max.z),\
	Vector3D(min.x, max.y, min.z),\
	Vector3D(max.x, max.y, min.z),\
	Vector3D(min.x, min.y, min.z),\
	Vector3D(max.x, min.y, min.z),\
	Vector3D(min.x, min.y, max.z),\
	Vector3D(max.x, min.y, max.z),\
	Vector3D(max.x, min.y, max.z),\
	Vector3D(max.x, min.y, min.z),\
	Vector3D(max.x, min.y, min.z),\
	Vector3D(max.x, max.y, min.z),\
	Vector3D(max.x, min.y, max.z),\
	Vector3D(max.x, max.y, max.z),\
	Vector3D(min.x, min.y, max.z),\
	Vector3D(min.x, max.y, max.z),\
	Vector3D(min.x, min.y, min.z),\
	Vector3D(min.x, max.y, min.z)

void UTIL_DebugDrawOBB(const FVector3D& pos, const Vector3D& mins, const Vector3D& maxs, const Matrix4x4& matrix, const ColorRGBA& color)
{
	Matrix4x4 tmatrix = transpose(matrix);

	Vector3D verts[18] = { BBOX_STRIP_VERTS(mins, maxs) };

	// transform them
	for (int i = 0; i < 18; i++)
		verts[i] = Vector3D(pos + (matrix*Vector4D(verts[i], 1.0f)).xyz());

	debugoverlay->Line3D(pos + tmatrix.rows[0].xyz()*mins.x, pos + tmatrix.rows[0].xyz()*maxs.x, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
	debugoverlay->Line3D(pos + tmatrix.rows[1].xyz()*mins.y, pos + tmatrix.rows[1].xyz()*maxs.y, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
	debugoverlay->Line3D(pos + tmatrix.rows[2].xyz()*mins.z, pos + tmatrix.rows[2].xyz()*maxs.z, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));

	debugoverlay->Polygon3D(verts[0], verts[1], verts[2], color);
	debugoverlay->Polygon3D(verts[2], verts[1], verts[3], color);

	debugoverlay->Polygon3D(verts[2], verts[3], verts[4], color);
	debugoverlay->Polygon3D(verts[4], verts[3], verts[5], color);

	debugoverlay->Polygon3D(verts[4], verts[5], verts[6], color);
	debugoverlay->Polygon3D(verts[6], verts[5], verts[7], color);

	debugoverlay->Polygon3D(verts[10], verts[11], verts[12], color);
	debugoverlay->Polygon3D(verts[12], verts[11], verts[13], color);

	debugoverlay->Polygon3D(verts[12], verts[13], verts[14], color);
	debugoverlay->Polygon3D(verts[14], verts[13], verts[15], color);

	debugoverlay->Polygon3D(verts[14], verts[15], verts[16], color);
	debugoverlay->Polygon3D(verts[16], verts[15], verts[17], color);
}
