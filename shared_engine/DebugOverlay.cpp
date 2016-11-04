//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Debug text drawer system
//////////////////////////////////////////////////////////////////////////////////

#include "DebugOverlay.h"
#include "FontCache.h"

#include "DebugInterface.h"
#include "math/math_util.h"
#include "ConVar.h"
#include "utils/strtools.h"

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MeshBuilder.h"


#ifndef _WIN32
#include <stdarg.h>
#endif

#if !defined(EDITOR) &&  !defined(NO_GAME)
#define DEBUG_DEFAULT_VALUE "0"
#else
#define DEBUG_DEFAULT_VALUE "1"
#endif // !EDITOR && !NO_GAME

static ConVar r_drawFrameStats("r_frameStats", DEBUG_DEFAULT_VALUE,NULL, CV_ARCHIVE);
static ConVar r_debugdrawgraphs("r_debugDrawGraphs", DEBUG_DEFAULT_VALUE,NULL, CV_ARCHIVE);
static ConVar r_debugdrawboxes("r_debugDrawBoxes", DEBUG_DEFAULT_VALUE,NULL, CV_ARCHIVE);
static ConVar r_debugdrawlines("r_debugDrawLines", DEBUG_DEFAULT_VALUE,NULL, CV_ARCHIVE);

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

void GUIDrawWindow(const Rectangle_t &rect, const ColorRGBA &color)
{
	ColorRGBA color2(0.2,0.2,0.2,0.8);

	Vertex2D_t tmprect[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, 0) };

	// Cancel textures
	g_pShaderAPI->Reset(STATE_RESET_TEX);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), NULL, color, &blending);

	// Set color

	// Draw 4 solid rectangles
	Vertex2D_t r0[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -0.5f) };
	Vertex2D_t r1[] = { MAKETEXQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -0.5f) };
	Vertex2D_t r2[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -0.5f) };
	Vertex2D_t r3[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -0.5f) };

	// Set alpha,rasterizer and depth parameters
	//g_pShaderAPI->SetBlendingStateFromParams(NULL);
	//g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT,FILL_SOLID);

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r0,elementsOf(r0), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r1,elementsOf(r1), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r2,elementsOf(r2), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r3,elementsOf(r3), NULL, color2, &blending);
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
	m_polygons(128)
{
	m_pDebugFont = NULL;

	m_oldtime = 0;
}

void CDebugOverlay::Init()
{
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

	if(!r_debugdrawboxes.GetBool())
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

	if(!r_debugdrawlines.GetBool())
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

void CDebugOverlay::Polygon3D(const Vector3D &v0, const Vector3D &v1,const Vector3D &v2, const Vector4D &color, float fTime)
{
	Threading::CScopedMutex m(m_mutex);

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

/*
void CDebugOverlay::Line2D(Vector2D &start, Vector2D &end, ColorRGBA &color1, ColorRGBA &color2, float fTime = 0.0f)
{

}

void CDebugOverlay::Box2D(Vector2D &mins, Vector2D &maxs, ColorRGBA &color1, float fTime = 0.0f)
{

}
*/

void DrawLineArray(DkList<DebugLineNode_t>* pNodes, float frametime)
{
	if(r_debugdrawlines.GetBool())
	{
		g_pShaderAPI->SetTexture(NULL,NULL, 0);

		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
		materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
		materials->SetDepthStates(true, false);

		materials->Apply();

		// bind the default material as we're emulating an FFP
		materials->BindMaterial(materials->GetDefaultMaterial());

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());
		meshBuilder.Begin(PRIM_LINES);

			DebugLineNode_t* nodes = pNodes->ptr();
			for(int i = 0; i < pNodes->numElem(); i++)
			{
				meshBuilder.Color4fv(nodes->color1);
				meshBuilder.Position3fv(nodes->start);

				meshBuilder.AdvanceVertex();

				meshBuilder.Color4fv(nodes->color2);
				meshBuilder.Position3fv(nodes->end);

				meshBuilder.AdvanceVertex();

				nodes->lifetime -= frametime;
				nodes++;
			}

		meshBuilder.End();
	}
}

void DrawBoxArray(DkList<DebugBoxNode_t>* pNodes, float frametime)
{
	if(r_debugdrawboxes.GetBool())
	{
		g_pShaderAPI->SetTexture(NULL,NULL, 0);

		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
		materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
		materials->SetDepthStates(true,false);

		materials->Apply();

		// bind the default material as we're emulating an FFP
		materials->BindMaterial(materials->GetDefaultMaterial());

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());
		meshBuilder.Begin(PRIM_LINES);

			DebugBoxNode_t* nodes = pNodes->ptr();
			for(int i = 0; i < pNodes->numElem(); i++)
			{
				meshBuilder.Color4fv(nodes->color);

				meshBuilder.Position3f(nodes->mins.x, nodes->maxs.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->mins.x, nodes->maxs.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->maxs.x, nodes->maxs.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->maxs.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->maxs.x, nodes->mins.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->mins.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->mins.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->mins.x, nodes->mins.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->mins.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->mins.x, nodes->maxs.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->maxs.x, nodes->mins.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->maxs.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->mins.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->mins.x, nodes->maxs.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->maxs.x, nodes->mins.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->maxs.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->maxs.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->maxs.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->maxs.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->maxs.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->mins.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->mins.y, nodes->mins.z);
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f(nodes->mins.x, nodes->mins.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3f(nodes->maxs.x, nodes->mins.y, nodes->maxs.z);
				meshBuilder.AdvanceVertex();

				nodes->lifetime -= frametime;
				nodes++;
			}

		meshBuilder.End();

	}
}

void DrawGraph(debugGraphBucket_t* graph, int position, IEqFont* pFont, float frame_time)
{
	float x_pos = 15;
	float y_pos = 90 + 100 + position*110;

	Vector2D last_point(-1);

	Vertex2D_t lines[] =
	{
		Vertex2D_t(Vector2D(x_pos, y_pos), vec2_zero),
		Vertex2D_t(Vector2D(x_pos, y_pos - 100), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+400, y_pos), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos - 90*0.75f), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+32, y_pos - 90*0.75f), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos - 90*0.50f), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+32, y_pos - 90*0.50f), vec2_zero),

		Vertex2D_t(Vector2D(x_pos, y_pos - 90*0.25f), vec2_zero),
		Vertex2D_t(Vector2D(x_pos+32, y_pos - 90*0.25f), vec2_zero),
	};

	/*
	g_pShaderAPI->DrawLine2D(Vector2D(x_pos, y_pos), Vector2D(x_pos, y_pos - 100));
	g_pShaderAPI->DrawLine2D(Vector2D(x_pos, y_pos), Vector2D(x_pos+400, y_pos));
	*/

	eqFontStyleParam_t textStl;
	textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

	pFont->RenderText(graph->pszName, Vector2D(x_pos + 5, y_pos - 100), textStl);

	pFont->RenderText("0", Vector2D(x_pos + 5, y_pos), textStl);
	pFont->RenderText(varargs("%.2f", graph->fMaxValue), Vector2D(x_pos + 5, y_pos - 90), textStl);

	//g_pShaderAPI->DrawLine2D(Vector2D(x_pos, y_pos - 90*0.75f), Vector2D(x_pos+4, y_pos - 90*0.75f));
	//g_pShaderAPI->DrawLine2D(Vector2D(x_pos, y_pos - 90*0.50f), Vector2D(x_pos+4, y_pos - 90*0.50f));
	//g_pShaderAPI->DrawLine2D(Vector2D(x_pos, y_pos - 90*0.25f), Vector2D(x_pos+4, y_pos - 90*0.25f));

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_LINES,lines,elementsOf(lines), NULL, color4_white, &blending);

	pFont->RenderText(varargs("%.2f", (graph->fMaxValue*0.75f)), Vector2D(x_pos + 5, y_pos - 90*0.75f), textStl);
	pFont->RenderText(varargs("%.2f", (graph->fMaxValue*0.50f)), Vector2D(x_pos + 5, y_pos - 90*0.5f), textStl);
	pFont->RenderText(varargs("%.2f", (graph->fMaxValue*0.25f)), Vector2D(x_pos + 5, y_pos - 90*0.25f), textStl);

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

			// get a value of it.
			float value = clamp(graph->points.getCurrent(), 0.0f, graph->fMaxValue);

			if( graph->points.getCurrent() > graph_max_value )
				graph_max_value = graph->points.getCurrent();

			value /= graph->fMaxValue;

			value *= 100;

			Vector2D point(x_pos + 400 - (4*value_id), y_pos - value);

			if(value_id > 0)
			{
				graph_line_verts[num_line_verts].m_vPosition = last_point;
				graph_line_verts[num_line_verts].m_vColor = graph->color;

				num_line_verts++;

				graph_line_verts[num_line_verts].m_vPosition = point;
				graph_line_verts[num_line_verts].m_vColor = graph->color;

				num_line_verts++;
				//g_pShaderAPI->DrawSetColor(graph->color);
				//g_pShaderAPI->DrawLine2D(point, last_point);
			}

			value_id++;

			last_point = point;

		}while(graph->points.goToNext());
	}

	if(graph->isDynamic)
		graph->fMaxValue = graph_max_value;

	materials->DrawPrimitives2DFFP(PRIM_LINES,graph_line_verts,num_line_verts, NULL, color4_white);

	graph->remaining_update_time -= frame_time;

	if(graph->remaining_update_time <= 0)
		graph->remaining_update_time = 0.0f;

}

void DrawPolygons(DebugPolyNode_t* polygons, int numPolys, float frameTime)
{
	g_pShaderAPI->SetTexture(NULL,NULL, 0);

	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_BACK,FILL_SOLID);
	materials->SetDepthStates(true, true);

	// bind the default material as we're emulating an FFP
	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLES);

		for(int i = 0; i < numPolys; i++)
		{
			meshBuilder.Color4fv(polygons[i].color);

			meshBuilder.Position3fv(polygons[i].v0);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v1);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(polygons[i].v2);
			meshBuilder.AdvanceVertex();

			polygons[i].lifetime -= frameTime;
		}

	meshBuilder.End();

	meshBuilder.Begin(PRIM_LINES);
		for(int i = 0; i < numPolys; i++)
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
		}
	meshBuilder.End();
}

void CDebugOverlay::SetMatrices( const Matrix4x4 &proj, const Matrix4x4 &view )
{
	m_projMat = proj;
	m_viewMat = view;
}

void CDebugOverlay::Draw(int winWide, int winTall)
{
	float fCurTime = Platform_GetCurrentTime();

	m_frametime = fCurTime - m_oldtime;

	m_oldtime = fCurTime;

	materials->SetMatrix(MATRIXMODE_PROJECTION, m_projMat);
	materials->SetMatrix(MATRIXMODE_VIEW, m_viewMat);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

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

		DrawBoxArray(&m_BoxList, m_frametime);
		DrawBoxArray(&m_FastBoxList, m_frametime);

		DrawLineArray(&m_LineList, m_frametime);
		DrawLineArray(&m_FastLineList, m_frametime);

		DrawPolygons(m_polygons.ptr(), m_polygons.numElem(), m_frametime);
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

				if(!m_LeftTextFadeArray.goToPrev()) // get back
					break;

				continue;
			}

			if(current.initialLifetime > 0.05f)
				current.color.w = clamp(current.lifetime, 0.0f, 1.0f);
			else
				current.color.w = 1.0f;

			eqFontStyleParam_t textStl;
			textStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
			textStl.textColor = current.color;

			Vector2D textPos = drawFadedTextBoxPosition + Vector2D( 0, (idx*m_pDebugFont->GetLineHeight()) );

			m_pDebugFont->RenderText( current.pszText.GetData(), textPos, textStl);

			idx++;

			current.lifetime -= m_frametime;

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

			current.lifetime -= m_frametime;

			if(!beh && visible)
			{
				textStl.textColor = current.color;
				m_pDebugFont->RenderText(current.pszText.GetData(), screen.xy(), textStl);
			}
		}

		if(m_TextArray.numElem())
		{
			GUIDrawWindow(Rectangle_t(drawTextBoxPosition.x,drawTextBoxPosition.y,drawTextBoxPosition.x+380,drawTextBoxPosition.y+(m_TextArray.numElem()*m_pDebugFont->GetLineHeight())),ColorRGBA(0.5f,0.5f,0.5f,0.5f));

			for (int i = 0;i < m_TextArray.numElem();i++)
			{
				DebugTextNode_t& current = m_TextArray[i];

				textStl.textColor = current.color;

				Vector2D textPos(drawTextBoxPosition.x,drawTextBoxPosition.y+(i*m_pDebugFont->GetLineHeight()));

				m_pDebugFont->RenderText(current.pszText.GetData(), textPos, textStl);
			}
		}

		eqFontStyleParam_t rTextFadeStyle;
		rTextFadeStyle.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
		rTextFadeStyle.align = TEXT_ALIGN_RIGHT;

		for (int i = 0;i < m_RightTextFadeArray.numElem();i++)
		{
			DebugFadingTextNode_t& current = m_RightTextFadeArray[i];

			float textLen = m_pDebugFont->GetStringWidth( current.pszText.c_str(), textStl.styleFlag );

			if(current.initialLifetime > 0.05f)
				current.color.w = clamp(current.lifetime, 0.0f, 1.0f);
			else
				current.color.w = 1.0f;

			rTextFadeStyle.textColor = current.color;
			Vector2D textPos(winWide - (textLen*m_pDebugFont->GetLineHeight()), 45+(i*m_pDebugFont->GetLineHeight()));

			m_pDebugFont->RenderText( current.pszText.GetData(), textPos, rTextFadeStyle);

			current.lifetime -= m_frametime;
		}
	}

	if(r_debugdrawgraphs.GetBool())
	{
		Threading::CScopedMutex m(m_mutex);

		for(int i = 0; i < m_graphbuckets.numElem(); i++)
		{
			DrawGraph( m_graphbuckets[i], i, m_pDebugFont, m_frametime);
		}
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

	for (int i = 0;i < m_polygons.numElem();i++)
	{
		if(m_polygons[i].lifetime <= 0)
		{
			m_polygons.fastRemoveIndex(i);
			i--;
		}
	}

	m_FastBoxList.clear();
	m_FastLineList.clear();
}

debugGraphBucket_t*	CDebugOverlay::Graph_AddBucket( const char* pszName, const ColorRGBA &color, float fMaxValue, float fUpdateTime)
{
	debugGraphBucket_t *graph = new debugGraphBucket_t;
	graph->color = color;
	strcpy(graph->pszName, pszName);
	graph->fMaxValue = fMaxValue;
	graph->update_time = fUpdateTime;
	graph->remaining_update_time = 0.0f;
	graph->isDynamic = false;

	int id = m_graphbuckets.append(graph);

	return m_graphbuckets[id];
}

void CDebugOverlay::Graph_AddValue(debugGraphBucket_t* bucket, float value)
{
	if(!r_debugdrawgraphs.GetBool())
		return;

	if(bucket->remaining_update_time <= 0)
	{
		bucket->remaining_update_time = bucket->update_time;

		bucket->points.addFirst(value);

		if(bucket->points.getCount() > 100)
		{
			if(bucket->points.goToLast())
				bucket->points.removeCurrent();
		}
	}
}

void CDebugOverlay::RemoveAllGraphs()
{
	for(int i = 0; i < m_graphbuckets.numElem(); i++)
		delete m_graphbuckets[i];

	m_graphbuckets.clear();
}

void CDebugOverlay::Graph_RemoveBucket(debugGraphBucket_t* pBucket)
{
	if(m_graphbuckets.remove(pBucket))
		delete pBucket;
}

debugGraphBucket_t*	CDebugOverlay::Graph_FindBucket(const char* pszName)
{
	for(int i = 0; i < m_graphbuckets.numElem(); i++)
	{
		if(!stricmp(m_graphbuckets[i]->pszName, pszName))
			return m_graphbuckets[i];
	}

	return NULL;
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
	FMatrix4x4 mat = matrix;

	mat = transpose(mat);
	debugoverlay->Line3D(pos + mat.rows[0].xyz()*mins.x, pos + mat.rows[0].xyz()*maxs.x, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
	debugoverlay->Line3D(pos + mat.rows[1].xyz()*mins.y, pos + mat.rows[1].xyz()*maxs.y, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1));
	debugoverlay->Line3D(pos + mat.rows[2].xyz()*mins.z, pos + mat.rows[2].xyz()*maxs.z, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));

	Vector3D verts[18] =
	{
		BBOX_STRIP_VERTS(mins, maxs)
	};

	mat = transpose(mat);

	// transform them
	for(int i = 0; i < 18; i++)
	{
		verts[i] = Vector3D(pos + (mat*FVector4D(verts[i], 1.0f)).xyz());
	}

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
