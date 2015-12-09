//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable entity parameters interface class
//////////////////////////////////////////////////////////////////////////////////

#include "EditableEntityParameters.h"

#include "EntityProperties.h"
#include "EditorCamera.h"
#include "SelectionEditor.h"
#include "GameSoundEmitterSystem.h"

typedef IEntityVariable* (*IENTREGVARIABLE_FACTORY)(CEditableEntity* pEnt, char* pszValue);

struct paramfactory_t
{
	char* pszType;
	IENTREGVARIABLE_FACTORY factory;
};

//----------------------------------
// Variable modifiers and renderers
//----------------------------------

extern void DkDrawSphere(const Vector3D& origin, float radius, int sides);
extern void DkDrawFilledSphere(const Vector3D &origin, float radius, int sides);

class CRadiusVar : public IEntityVariable
{
public:
	CRadiusVar(CEditableEntity* pEnt, char* pszValue) : IEntityVariable(pEnt, pszValue)
	{
		// you need to set parameter type here also
		m_variable.varType = PARAM_TYPE_RADIUS;
		//SetValue(pszValue);
		m_bCanDrag = false;
	}

	void Render(int nViewRenderFlags)
	{
		if(!(m_pEntity->GetFlags() & EDFL_SELECTED))
			return;

		materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 0.25f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

		//DkDrawSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
		DkDrawFilledSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
	}

	void RenderGhost(int nViewRenderFlags)
	{
		if(!(m_pEntity->GetFlags() & EDFL_SELECTED))
			return;

		materials->GetConfiguration().wireframeMode = false;

		materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 1.0f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

		DkDrawSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
		//DkDrawFilledSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
	}

	bool UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
	{
		Matrix4x4 view, proj;
		pViewRender->GetViewProjection(view,proj);

		Matrix4x4 vp = proj*view;

		Vector2D screenDims = pViewRender->Get2DDimensions();

		// get mouse cursor position
		Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

		Vector3D ray_start, ray_dir;
		// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
		ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, ray_start, ray_dir, vp, true);

		Vector3D marker_3d = view.rows[0].xyz() * m_variable.GetFloat();
		Vector2D markerPos;

		PointToScreen(m_pEntity->GetPosition() + marker_3d, markerPos, proj*view, screenDims);

		if(m_bCanDrag && mouseEvents.Dragging() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			float fMarkerSizeAdd = dot(delta3D, view.rows[0].xyz());

			m_variable.flVal += fMarkerSizeAdd;

			return true;
		}

		if(!mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_bCanDrag)
		{
			m_pEntity->SetKey(m_pszKey, varargs("%g", fabs(m_variable.GetFloat())));
			g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();
			m_bCanDrag = false;
		}

		if(length(mouse_position-markerPos) < 4)
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.ShiftDown())
				m_bCanDrag = true;

			return true;
		}

		return false;
	}

	void Render2D(CEditorViewRender* pViewRender)
	{
		//Msg("2D!");

		Matrix4x4 view, proj;
		pViewRender->GetViewProjection(view, proj);

		Matrix4x4 vp = proj*view;

		Vector2D markerPos;

		Vector3D marker_3d = Vector3D(m_variable.GetFloat())*view.rows[0].xyz();

		PointToScreen(m_pEntity->GetPosition() + marker_3d, markerPos, vp, pViewRender->Get2DDimensions());

		Vertex2D_t handle_verts[] = {MAKETEXQUAD(markerPos.x-3,markerPos.y-3,markerPos.x+3,markerPos.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, handle_verts, elementsOf(handle_verts), NULL, color4_white);

		// draw only single point
		Vertex2D_t pointR[] = {MAKETEXRECT(markerPos.x-3,markerPos.y-3,markerPos.x+3,markerPos.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
	}

protected:
	bool			m_bCanDrag;	// is dragging?
};

class CTargetVar : public IEntityVariable
{
public:
	CTargetVar(CEditableEntity* pEnt, char* pszValue) : IEntityVariable(pEnt, pszValue)
	{
		// you need to set parameter type here also
		m_variable.varType = PARAM_TYPE_ENTITY;
		m_pTargetEntity = NULL;
		m_nObjectID = -1;
		//SetValue(pszValue);
	}

	void Update()
	{
		for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
		{
			if(g_pLevel->GetEditable(i)->GetType() == EDITABLE_ENTITY)
			{
				if(!stricmp(g_pLevel->GetEditable(i)->GetName(), m_variable.GetString()))
				{
					m_pTargetEntity = (CEditableEntity*)g_pLevel->GetEditable(i);
					m_nObjectID = m_pTargetEntity->GetObjectID();
					return;
				}
			}
		}

		MsgError("Target '%s' not found!\n", m_variable.GetString());
		m_nObjectID = -1;
	}

	void Render(int nViewRenderFlags)
	{
		// always check this shit
		if(m_nObjectID == -1 || (g_pLevel->GetEditableByUniqueID(m_nObjectID) != m_pTargetEntity))
		{
			m_nObjectID = -1;
			m_pTargetEntity = NULL;

			Update();
		}

		if(m_pTargetEntity)
		{
			materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 1.0f));
			materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);
		
			IMeshBuilder* meshbuild = g_pShaderAPI->CreateMeshBuilder();

			meshbuild->Begin(PRIM_LINE_STRIP);
				meshbuild->Position3fv(m_pTargetEntity->GetPosition());
				meshbuild->AdvanceVertex();
				meshbuild->Position3fv(m_pEntity->GetPosition());
				meshbuild->AdvanceVertex();
			meshbuild->End();

			g_pShaderAPI->DestroyMeshBuilder(meshbuild);
		}
	}

	void RenderGhost(int nViewRenderFlags)
	{
		Render(0);
	}

protected:

	CEditableEntity*	m_pTargetEntity;
	int					m_nObjectID;
};


class CSoundVar : public IEntityVariable
{
public:
	CSoundVar(CEditableEntity* pEnt, char* pszValue) : IEntityVariable(pEnt, pszValue)
	{
		// you need to set parameter type here also
		m_variable.varType = PARAM_TYPE_SOUND;
		
		//SetValue(pszValue);
	}

	void OnRemove()
	{
		
	}

	void Update()
	{

		//ses->StopAllSounds();
	}

	void Render(int nViewRenderFlags)
	{

	}

	void RenderGhost(int nViewRenderFlags)
	{

	}

protected:

};

class CTargetPoint : public IEntityVariable
{
public:
	CTargetPoint(CEditableEntity* pEnt, char* pszValue) : IEntityVariable(pEnt, pszValue)
	{
		// you need to set parameter type here also
		m_variable.varType = PARAM_TYPE_TARGETPOINT;
		//SetValue(pszValue);
		m_bCanDrag = false;
	}

	void Render(int nViewRenderFlags)
	{
		if(!(m_pEntity->GetFlags() & EDFL_SELECTED))
			return;

		g_pShaderAPI->Reset(STATE_RESET_VBO);

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 0.25f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

		//DkDrawSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
		DkDrawFilledSphere(m_pEntity->GetPosition() + m_variable.GetVector3D(), 8, 8);
	}

	void RenderGhost(int nViewRenderFlags)
	{
		if(!(m_pEntity->GetFlags() & EDFL_SELECTED))
			return;

		g_pShaderAPI->Reset(STATE_RESET_VBO);

		materials->GetConfiguration().wireframeMode = false;

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 1.0f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

		DkDrawFilledSphere(m_pEntity->GetPosition() + m_variable.GetVector3D(), 8, 8);
	}

	bool UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
	{
		Matrix4x4 view, proj;
		pViewRender->GetViewProjection(view,proj);

		Matrix4x4 vp = proj*view;

		Vector2D screenDims = pViewRender->Get2DDimensions();

		// get mouse cursor position
		Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

		Vector3D ray_start, ray_dir;
		// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
		ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, ray_start, ray_dir, vp, true);

		Vector3D marker_3d = m_pEntity->GetPosition() + m_variable.GetVector3D();
		Vector2D markerPos;

		PointToScreen(marker_3d, markerPos, proj*view, screenDims);

		if(m_bCanDrag && mouseEvents.Dragging() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			// do local
			Vector3D origin = (marker_3d + delta3D) - m_pEntity->GetPosition();

			m_variable.vecVal3[0] = origin.x;
			m_variable.vecVal3[1] = origin.y;
			m_variable.vecVal3[2] = origin.z;

			return true;
		}

		if(!mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_bCanDrag)
		{
			m_pEntity->SetKey(m_pszKey, varargs("%g %g %g", m_variable.vecVal3[0], m_variable.vecVal3[1], m_variable.vecVal3[2]));

			g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();
			m_bCanDrag = false;
		}

		if(length(mouse_position-markerPos) < 24)
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.ShiftDown())
				m_bCanDrag = true;

			return true;
		}

		return false;
	}

	void Render2D(CEditorViewRender* pViewRender)
	{

	}

protected:
	bool			m_bCanDrag;	// is dragging?
};

/*
class CTargetVar : public IEntityVariable
{
public:
	CTargetVar(CEditableEntity* pEnt, char* pszValue) : IEntityVariable(pEnt, pszValue)
	{
		// you need to set parameter type here also
		m_variable.varType = PARAM_TYPE_ENTITY;
		m_pTargetEntity = NULL;
		m_nObjectID = -1;
		//SetValue(pszValue);
	}

	void Update()
	{
		for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
		{
			if(g_pLevel->GetEditable(i)->GetType() == EDITABLE_ENTITY)
			{
				if(!stricmp(g_pLevel->GetEditable(i)->GetName(), m_variable.GetString()))
				{
					m_pTargetEntity = (CEditableEntity*)g_pLevel->GetEditable(i);
					m_nObjectID = m_pTargetEntity->GetObjectID();
					return;
				}
			}
		}

		MsgError("Target '%s' not found!\n", m_variable.GetString());
		m_nObjectID = -1;
	}

	void Render(int nViewRenderFlags)
	{
		// always check this shit
		if(m_nObjectID == -1 || (g_pLevel->GetEditableByUniqueID(m_nObjectID) != m_pTargetEntity))
		{
			m_nObjectID = -1;
			m_pTargetEntity = NULL;

			Update();
		}

		if(m_pTargetEntity)
		{
			materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 1.0f));
			materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);
		
			IMeshBuilder* meshbuild = g_pShaderAPI->CreateMeshBuilder();

			meshbuild->Begin(PRIM_LINE_STRIP);
				meshbuild->Position3fv(m_pTargetEntity->GetPosition());
				meshbuild->AdvanceVertex();
				meshbuild->Position3fv(m_pEntity->GetPosition());
				meshbuild->AdvanceVertex();
			meshbuild->End();

			g_pShaderAPI->DestroyMeshBuilder(meshbuild);
		}
	}

	void RenderGhost(int nViewRenderFlags)
	{
		Render(0);
	}

protected:

	CEditableEntity*	m_pTargetEntity;
	int					m_nObjectID;
};
*/


extern void DkDrawCone(const Vector3D& origin, const Vector3D& angles, float distance, float fFov, int sides);

class CFOVRadius : public IEntityVariable
{
public:
	CFOVRadius(CEditableEntity* pEnt, char* pszValue) : IEntityVariable(pEnt, pszValue)
	{
		// you need to set parameter type here also
		m_variable.varType = PARAM_TYPE_RADIUS;
		m_fov.varType = PARAM_TYPE_FLOAT;

		char* var = pEnt->GetKeyValue("fov");

		if(var)
			m_fov.SetValue(pEnt->GetKeyValue("fov"));
		else
			m_fov.flVal = 90;

		//SetValue(pszValue);
		m_bCanDrag = false;
	}

	void Render(int nViewRenderFlags)
	{
		if(!(m_pEntity->GetFlags() & EDFL_SELECTED))
			return;

		materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 0.25f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

		//DkDrawSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
		//DkDrawFilledSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
		DkDrawCone(m_pEntity->GetPosition(), m_pEntity->GetAngles(), m_variable.GetFloat(), m_fov.GetFloat(), 16);
	}

	void RenderGhost(int nViewRenderFlags)
	{
		if(!(m_pEntity->GetFlags() & EDFL_SELECTED))
			return;

		materials->GetConfiguration().wireframeMode = false;

		materials->SetAmbientColor(ColorRGBA(m_pEntity->GetGroupColor(), 1.0f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), true);

		//DkDrawSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
		DkDrawCone(m_pEntity->GetPosition(), m_pEntity->GetAngles(), m_variable.GetFloat(), m_fov.GetFloat(), 16);
		//DkDrawFilledSphere(m_pEntity->GetPosition(), m_variable.GetFloat(), 32);
	}

	void Update()
	{
		char* var = m_pEntity->GetKeyValue("fov");

		if(var)
			m_fov.SetValue(m_pEntity->GetKeyValue("fov"));
		else
			m_fov.flVal = 90;
	}

	bool UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
	{
		Matrix4x4 view, proj;
		pViewRender->GetViewProjection(view,proj);

		Matrix4x4 vp = proj*view;

		Vector2D screenDims = pViewRender->Get2DDimensions();

		// get mouse cursor position
		Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

		Vector3D ray_start, ray_dir;
		// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
		ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, ray_start, ray_dir, vp, true);

		Vector3D marker_3d = view.rows[0].xyz() * m_variable.GetFloat();
		Vector2D markerPos;

		PointToScreen(m_pEntity->GetPosition() + marker_3d, markerPos, proj*view, screenDims);

		if(m_bCanDrag && mouseEvents.Dragging() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			float fMarkerSizeAdd = dot(delta3D, view.rows[0].xyz());

			m_variable.flVal += fMarkerSizeAdd;

			return true;
		}

		if(!mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_bCanDrag)
		{
			m_pEntity->SetKey(m_pszKey, varargs("%g", fabs(m_variable.GetFloat())));
			g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();
			m_bCanDrag = false;
		}

		if(length(mouse_position-markerPos) < 4)
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.ShiftDown())
				m_bCanDrag = true;

			return true;
		}

		return false;
	}

	void Render2D(CEditorViewRender* pViewRender)
	{
		//Msg("2D!");

		Matrix4x4 view, proj;
		pViewRender->GetViewProjection(view, proj);

		Matrix4x4 vp = proj*view;

		Vector2D markerPos;

		Vector3D marker_3d = Vector3D(m_variable.GetFloat())*view.rows[0].xyz();

		PointToScreen(m_pEntity->GetPosition() + marker_3d, markerPos, vp, pViewRender->Get2DDimensions());

		Vertex2D_t handle_verts[] = {MAKETEXQUAD(markerPos.x-3,markerPos.y-3,markerPos.x+3,markerPos.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, handle_verts, elementsOf(handle_verts), NULL, color4_white);

		// draw only single point
		Vertex2D_t pointR[] = {MAKETEXRECT(markerPos.x-3,markerPos.y-3,markerPos.x+3,markerPos.y+3, 0)};
		materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
	}

protected:
	bool			m_bCanDrag;	// is dragging?
	evariable_t		m_fov;
};

// FACTORY

IEntityVariable* CreateRadiusVar(CEditableEntity* pEnt, char* pszValue) {return new CRadiusVar(pEnt, pszValue);}
IEntityVariable* CreateFOVRadiusVar(CEditableEntity* pEnt, char* pszValue) {return new CFOVRadius(pEnt, pszValue);}
IEntityVariable* CreateTargetVar(CEditableEntity* pEnt, char* pszValue) {return new CTargetVar(pEnt, pszValue);}
IEntityVariable* CreateSoundVar(CEditableEntity* pEnt, char* pszValue) {return new CSoundVar(pEnt, pszValue);}
IEntityVariable* CreateTargetPoint(CEditableEntity* pEnt, char* pszValue) {return new CTargetPoint(pEnt, pszValue);}

// Parameter factory.
// always add here your type factory

paramfactory_t g_entParamFactory[] =
{
	{"radius", CreateRadiusVar},
	{"spotradius", CreateFOVRadiusVar},
	{"entity", CreateTargetVar},
	{"sound", CreateSoundVar},
	{"targetpoint", CreateTargetPoint},
	{NULL, NULL}, // the last is always null
};

IEntityVariable*	CreateRegisteredVariable(char* pszType, CEditableEntity* pEnt, char* pszValue)
{
	for(int i = 0; (g_entParamFactory[i].pszType != NULL); i++)
	{
		if(!stricmp(g_entParamFactory[i].pszType, pszType))
			return g_entParamFactory[i].factory(pEnt, pszValue);
	}

	// nothing created, deny
	return NULL;
}