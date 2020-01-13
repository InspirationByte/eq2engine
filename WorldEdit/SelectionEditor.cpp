//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Selection model
//////////////////////////////////////////////////////////////////////////////////

#include "EditorHeader.h"
#include "EntityProperties.h"

#include "SelectionEditor.h"
#include "math/math_util.h"
#include "DebugInterface.h"
#include "EditorMainFrame.h"
#include "SurfaceDialog.h"
#include "IDebugOverlay.h"

#include "EditableSurface.h"
#include "EqBrush.h"
#include "EditableEntity.h"
#include "EditableEntityParameters.h"
#include "GroupEditable.h"

#include "ClipperTool.h"
#include "BlockTool.h"
#include "VertexTool.h"
#include "TerrainPatchTool.h"
#include "EntityTool.h"
#include "DecalTool.h"

#include "grid.h"

#include "materialsystem/MeshBuilder.h"

bool PointToScreenOrtho(const Vector3D& point, Vector2D& screen, Matrix4x4 &mvp)
{
	Matrix4x4 worldToScreen = mvp;

	bool behind = false;

	Vector4D outMat;
	Vector4D transpos(point,1.0f);

	outMat = worldToScreen*transpos;

	if(outMat.w < 0)
		behind = true;

	screen.x = outMat.x;
	screen.y = outMat.y;

	return behind;
}

// converts 3d bbox to rectangle
void SelectionRectangleFromBBox(CEditorViewRender* pViewRender,const Vector3D &mins,const Vector3D &maxs, Rectangle_t &rect)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	Matrix4x4 vp = proj*view;

	Vector2D min, max;

	PointToScreenOrtho(mins, min, vp);
	PointToScreenOrtho(maxs, max, vp);

	min *= 0.5f;
	max *= 0.5f;

	rect.vleftTop = min*pViewRender->Get2DDimensions()+pViewRender->Get2DDimensions()*0.5f;
	rect.vrightBottom = max*pViewRender->Get2DDimensions()+pViewRender->Get2DDimensions()*0.5f;

	rect.vleftTop.y = pViewRender->Get2DDimensions().y - rect.vleftTop.y;
	rect.vrightBottom.y = pViewRender->Get2DDimensions().y - rect.vrightBottom.y;
}

// converts rectangle to 3d bbox
void SelectionBBoxFromRectangle(CEditorViewRender* pViewRender, const Rectangle_t &rect, Vector3D &mins, Vector3D &maxs)
{

}

// 2D handle position
void SelectionGetHandlePositions(Rectangle_t &rect, Vector2D *handles, Rectangle_t &window_rect, float offset)
{
	Vector2D temp_handles[SH_COUNT];

	Rectangle_t offset_rect = rect;

	rect.Fix();
	window_rect.Fix();

	// fix rectangle
	offset_rect.Fix();

	offset_rect.vleftTop -= Vector2D(offset);
	offset_rect.vrightBottom += Vector2D(offset);

	handles[SH_CENTER] = offset_rect.GetCenter();

	handles[SH_TOPLEFT] = offset_rect.GetLeftTop();
	handles[SH_BOTTOMLEFT] = offset_rect.GetLeftBottom();

	handles[SH_TOPRIGHT] = offset_rect.GetRightTop();
	handles[SH_BOTTOMRIGHT] = offset_rect.GetRightBottom();


	if(/*!window_rect.IsFullyInside(rect) && */window_rect.IsIntersectsRectangle(rect))
	{
		// clip rectangle to window rectangle.
		if(offset_rect.vleftTop.x < window_rect.vleftTop.x)
			offset_rect.vleftTop.x = window_rect.vleftTop.x;

		if(offset_rect.vleftTop.y < window_rect.vleftTop.y)
			offset_rect.vleftTop.y = window_rect.vleftTop.y;

		if(offset_rect.vrightBottom.x > window_rect.vrightBottom.x)
			offset_rect.vrightBottom.x = window_rect.vrightBottom.x;

		if(offset_rect.vrightBottom.y > window_rect.vrightBottom.y)
			offset_rect.vrightBottom.y = window_rect.vrightBottom.y;
	}

	handles[SH_LEFT] = (offset_rect.GetLeftTop() + offset_rect.GetLeftBottom())*0.5f;
	handles[SH_RIGHT] = (offset_rect.GetRightTop() + offset_rect.GetRightBottom())*0.5f;

	handles[SH_TOP] = (offset_rect.GetLeftTop() + offset_rect.GetRightTop())*0.5f;
	handles[SH_BOTTOM] = (offset_rect.GetLeftBottom() + offset_rect.GetRightBottom())*0.5f;
}

Vector3D SelectionGetHandleDirection(CEditorViewRender* pViewRender, SelectionHandle_e type)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	Vector3D to_transform;

	Vector3D right = view.rows[0].xyz();
	Vector3D up = view.rows[1].xyz();

	switch(type)
	{
		case SH_LEFT:
			to_transform = right;//Vector3D(1,0,0);
			break;
		case SH_RIGHT:
			to_transform = -right;//Vector3D(-1,0,0);
			break;
		case SH_TOP:
			to_transform = -up;//Vector3D(0,-1,0);
			break;
		case SH_BOTTOM:
			to_transform = up;//Vector3D(0,1,0);
			break;
		case SH_TOPLEFT:
			to_transform = -Vector3D(1,1,1);
			break;
		case SH_TOPRIGHT:
			to_transform = Vector3D(1,-1,1);
			break;
		case SH_BOTTOMLEFT:
			to_transform = Vector3D(-1,1,1);
			break;
		case SH_BOTTOMRIGHT:
			to_transform = Vector3D(1,1,1);
			break;
		case SH_CENTER:
			to_transform = vec3_zero;
			break;
	}

	//Vector3D move_dir = (view * Vector4D(to_transform,1)).xyz();

	return to_transform;
}

void SelectionGetVolumePlanes(CEditorViewRender* pViewRender, SelectionHandle_e type, Volume &vol, int *plane_ids)
{
	SelectionHandle_e types[2];

	TranslateHandleType(type, types);

	plane_ids[0] = -1;
	plane_ids[1] = -1;

	if(types[0] != SH_NONE)
	{
		Vector3D dir = SelectionGetHandleDirection(pViewRender, types[0]);
		for(int i = 0; i < 6; i++)
		{
			Plane plane = vol.GetPlane(i);
			if(dot(dir, plane.normal) >= 0.9f)
			{
				plane_ids[0] = i;
				break;
			}
		}
	}

	if(types[1] != SH_NONE)
	{
		Vector3D dir = SelectionGetHandleDirection(pViewRender, types[1]);
		for(int i = 0; i < 6; i++)
		{
			Plane plane = vol.GetPlane(i);
			if(dot(dir, plane.normal) >= 0.9f)
			{
				plane_ids[1] = i;
				break;
			}
		}
	}
}

void UpdateAllWindows()
{
	g_editormainframe->UpdateAllWindows();
}

void Update3DView()
{
	g_editormainframe->Update3DView();
}

void SnapVector3D(Vector3D &vec)
{
	g_editormainframe->SnapVector3D(vec);
}

void SnapFloatValue(float &val)
{
	g_editormainframe->SnapFloatValue(val);
}

void OnDeactivateCurrentTool()
{
	if(g_pSelectionTools[g_editormainframe->GetToolType()])
	{
		g_pSelectionTools[g_editormainframe->GetToolType()]->OnDeactivate();

		if(g_pSelectionTools[g_editormainframe->GetToolType()]->GetToolPanel())
			g_pSelectionTools[g_editormainframe->GetToolType()]->GetToolPanel()->Hide();
	}
}

void OnActivateCurrentTool()
{
	if(g_pSelectionTools[g_editormainframe->GetToolType()])
	{
		if(g_pSelectionTools[g_editormainframe->GetToolType()]->GetToolPanel())
			g_pSelectionTools[g_editormainframe->GetToolType()]->GetToolPanel()->Show();
	}
}

//------------------------------
// Selection tool panel
//------------------------------

class CSelectionToolPanel : public wxPanel
{
public:
	CSelectionToolPanel(wxWindow* pMultiToolPanel) : wxPanel(pMultiToolPanel,-1, wxPoint(0,0),wxSize(200,400))
	{
		m_pRotate_scale_over_center = new wxCheckBox(this, -1, DKLOC("TOKEN_ROTATESCALEOVERSCENTER", "Rotate/Scale over selection center"), wxPoint(5,5), wxSize(190, 25));
		m_pIgnoreGroups = new wxCheckBox(this, -1, DKLOC("TOKEN_IGNOREGROUPS", "Ignore groups when selecting"), wxPoint(5,30), wxSize(190, 25));
		m_pSelectBrushes = new wxCheckBox(this, -1, DKLOC("TOKEN_SELECTBRUSHES", "Select brushes"), wxPoint(5,55), wxSize(190, 25));
		m_pSelectSurfaces = new wxCheckBox(this, -1, DKLOC("TOKEN_SELECTSURFACES", "Select surfaces"), wxPoint(5,80), wxSize(190, 25));
		m_pSelectEntities = new wxCheckBox(this, -1, DKLOC("TOKEN_SELECTENTITIES", "Select entities"), wxPoint(5,105), wxSize(190, 25));

		m_pSelectBrushes->SetValue(true);
		m_pSelectSurfaces->SetValue(true);
		m_pSelectEntities->SetValue(true);
	}

	bool IsManipulationOverSelectionBox()
	{
		return m_pRotate_scale_over_center->GetValue();
	}

	bool IsIgnoreGroupsEnabled()
	{
		return m_pIgnoreGroups->GetValue();
	}

	int GetSelectionFlags()
	{
		int nFlags = 0;

		if(m_pSelectBrushes->GetValue())
			nFlags |= NR_FLAG_BRUSHES;

		if(m_pSelectSurfaces->GetValue())
			nFlags |= NR_FLAG_SURFACEMODS;

		if(m_pSelectEntities->GetValue())
			nFlags |= NR_FLAG_ENTITIES;

		return nFlags;
	}

protected:

	wxCheckBox* m_pRotate_scale_over_center;
	wxCheckBox* m_pIgnoreGroups;

	wxCheckBox* m_pSelectSurfaces;
	wxCheckBox* m_pSelectBrushes;
	wxCheckBox* m_pSelectEntities;
};

//------------------------------
// Selection box implementation
//------------------------------
CSelectionBaseTool::CSelectionBaseTool()
{
	// initially is none
	m_state = SELECTION_NONE;

	m_bIsDragging = false;		// dragging state
	m_bIsPainting = false;

	m_plane_ids[0] = -1;
	m_plane_ids[1] = -1;

	m_selectedhandle = SH_NONE;
	m_mouseover_handle = SH_NONE;

	m_translation = vec3_zero;
	m_rotation = vec3_zero;
	m_scaling = vec3_zero;
}

// tool settings panel, can be null
wxPanel* CSelectionBaseTool::GetToolPanel()
{
	return m_selection_panel;
}

void CSelectionBaseTool::InitializeToolPanel(wxWindow* pMultiToolPanel)
{
	m_selection_panel = new CSelectionToolPanel(pMultiToolPanel);
}

// updates manipulation using mouse events.
void CSelectionBaseTool::UpdateManipulation(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
{
	// manipulate in two modes
	if(pViewRender->GetCameraMode() == CPM_PERSPECTIVE)
	{
		UpdateManipulation3D(pViewRender, mouseEvents, delta3D, delta2D);
	}
	else
	{
		UpdateManipulation2D(pViewRender, mouseEvents, delta3D, delta2D);
	}
}

void CSelectionBaseTool::BackupSelectionForUndo()
{
	if(m_selectedobjects.numElem() == 0)
		return;

	CObjectAction* pNewAction = new CObjectAction;

	pNewAction->Begin();

	for(int i = 0; i < m_selectedobjects.numElem(); i++)
		pNewAction->AddObject(m_selectedobjects[i]);

	pNewAction->End();
	g_pLevel->AddAction(pNewAction);
}

Vector3D GetKeyDirection(CEditorViewRender* pViewRender, int keyCode)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	Vector3D to_transform(0);

	Vector3D right = view.rows[0].xyz();
	Vector3D up = view.rows[1].xyz();
	//Vector3D forward = view.rows[2].xyz();

	switch(keyCode)
	{
		case WXK_LEFT:
			to_transform = right;//Vector3D(1,0,0);
			break;
		case WXK_RIGHT:
			to_transform = -right;//Vector3D(-1,0,0);
			break;
		case WXK_UP:
			to_transform = -up;//Vector3D(0,-1,0);
			break;
		case WXK_DOWN:
			to_transform = up;//Vector3D(0,1,0);
			break;
			/*
		case WXK_PAGEDOWN:
			to_transform = -Vector3D(1,1,1);
			break;
		case WXK_PAGEUP:
			to_transform = Vector3D(1,-1,1);
			break;
*/
	}

	//Vector3D move_dir = (view * Vector4D(to_transform,1)).xyz();

	return to_transform;
}

// down/up key events
void CSelectionBaseTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
{
	if(!bDown)
	{
		if(event.GetKeyCode() == WXK_ESCAPE)
			CancelSelection(true);
		else if(event.m_keyCode == WXK_RETURN)
			SelectObjects();
		else if(event.m_keyCode == WXK_DELETE)
		{
			// Create undo
			BackupSelectionForUndo();

			DkList<CBaseEditableObject*> *selection = GetSelectedObjects();
		
			for(int i = 0; i < selection->numElem(); i++)
			{
				g_pLevel->RemoveEditable(selection->ptr()[i]);
				i--;
			}

			CancelSelection();
			g_editormainframe->UpdateAllWindows();
		}

		/*
		Vector3D dir = GetKeyDirection(pViewRender, event.GetKeyCode());
		if(length(dir) > 0)
		{
			TransformByPreview();
			m_bIsDragging = false;
		}
		*/
	}
	else
	{
		/*
		if(event.GetKeyCode() >= WXK_LEFT && event.GetKeyCode() <= WXK_DOWN)
		{
			Msg("MOVE\n");
			Vector3D dir = GetKeyDirection(pViewRender, event.GetKeyCode()) * g_gridsize;

			SnapVector3D(dir);

			m_translation += dir;
			m_bIsDragging = true;
		}
		*/
	}
}

// retunrs selection state
SelectionState_e CSelectionBaseTool::GetSelectionState()
{
	return m_state;
}

// retunrs selection state
void CSelectionBaseTool::SetSelectionState(SelectionState_e state)
{
	m_state = state;
}

// retunrs handle
SelectionHandle_e CSelectionBaseTool::GetSelectionHandle()
{
	return m_selectedhandle;
}

// resets state to SELECTION_NONE
void CSelectionBaseTool::CancelSelection(bool bCancelSurfaces, bool bExitCleanup)
{
	m_state = SELECTION_NONE;

	m_selectedhandle = SH_NONE;
	m_mouseover_handle = SH_NONE;

	m_plane_ids[0] = -1;
	m_plane_ids[1] = -1;

	m_translation = vec3_zero;
	m_rotation = vec3_zero;
	m_scaling = vec3_zero;

	m_bIsDragging = false;		// dragging state

	// deselect all objects
	for(int i = 0; i < m_selectedobjects.numElem(); i++)
	{
		m_selectedobjects[i]->RemoveFlags(EDFL_SELECTED);
	}

	// deselect all faces
	for(int i = 0; i < g_pLevel->GetEditableCount() && bCancelSurfaces; i++)
	{
		for(int j = 0; j < g_pLevel->GetEditable(i)->GetSurfaceTextureCount(); j++)
		{
			// remove surface selection flags
			g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->nFlags &= ~STFL_SELECTED;
		}
	}

	// release selected object ptrs
	m_selectedobjects.setNum(0, false);

	if(!bExitCleanup)
	{
		// notify that we updated the selection
		g_editormainframe->GetSurfaceDialog()->UpdateSelection();

		g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();

		// update all views
		g_editormainframe->UpdateAllWindows();

		g_editormainframe->SetStatusText( "Nothing selected", 1);
	}
}

bool CSelectionBaseTool::IsSelected(CBaseEditableObject* pObject, int &selection_id)
{
	for(int i = 0; i < m_selectedobjects.numElem(); i++)
	{
		if(m_selectedobjects[i] == pObject)
		{
			selection_id = i;
			return true;
		}
	}

	return false;
}

void CSelectionBaseTool::SUB_ToggleSelection(CBaseEditableObject* pObject)
{
	int nObjID = -1;

	if(IsSelected(pObject, nObjID))
	{
		// deselect it
		pObject->RemoveFlags(EDFL_SELECTED);
		m_selectedobjects.removeIndex(nObjID);
		return;
	}

	pObject->AddFlags(EDFL_SELECTED);

	m_selectedobjects.append(pObject);
}

bool IsAnyVertsInBBOX(CEditableSurface* pSurface, Volume &sel_box)
{
	for(int i = 0; i < pSurface->GetIndexCount(); i+=3)
	{
		int i0 = pSurface->GetIndices()[i];
		int i1 = pSurface->GetIndices()[i+1];
		int i2 = pSurface->GetIndices()[i+2];

		if(sel_box.IsTriangleInside(pSurface->GetVertices()[i0].position, pSurface->GetVertices()[i1].position, pSurface->GetVertices()[i2].position))
			return true;
	}

	return false;
}

// selects objects. Usually by hitting Enter key
void CSelectionBaseTool::SelectObjects(bool selectOnlySpecified, CBaseEditableObject** pAddObjects, int nObjects)
{
	if(GetSelectionState() == SELECTION_NONE || GetSelectionState() == SELECTION_SELECTED)
		return;

	BoundingBox bbox;
	m_bbox_volume.GetBBOXBack(bbox.minPoint, bbox.maxPoint);
	bbox.Fix();

	if(bbox.minPoint.x == bbox.maxPoint.x)
	{
		bbox.minPoint.x = -MAX_COORD_UNITS;
		bbox.maxPoint.x = MAX_COORD_UNITS;
	}

	if(bbox.minPoint.y == bbox.maxPoint.y)
	{
		bbox.minPoint.y = -MAX_COORD_UNITS;
		bbox.maxPoint.y = MAX_COORD_UNITS;
	}

	if(bbox.minPoint.z == bbox.maxPoint.z)
	{
		bbox.minPoint.z = -MAX_COORD_UNITS;
		bbox.maxPoint.z = MAX_COORD_UNITS;
	}

	m_bbox_volume.LoadBoundingBox(bbox.minPoint, bbox.maxPoint);

	int nGroupsSelected = 0;

	if(!selectOnlySpecified)
	{
		// test visible objects
		for(int i = 0; i < g_pLevel->GetRenderableCount(); i++)
		{
			CBaseEditableObject* pObject = (CBaseEditableObject*)g_pLevel->GetRenderable(i);

			int nSelFlags = m_selection_panel->GetSelectionFlags();

			if(!(nSelFlags & NR_FLAG_BRUSHES) && (pObject->GetType() == EDITABLE_BRUSH))
				continue;

			if(!(nSelFlags & NR_FLAG_SURFACEMODS) && (pObject->GetType() <= EDITABLE_TERRAINPATCH))
				continue;

			if(!(nSelFlags & NR_FLAG_ENTITIES) && ((pObject->GetType() == EDITABLE_ENTITY) || (pObject->GetType() == EDITABLE_DECAL)))
				continue;

			if(pObject->GetFlags() & EDFL_GROUPPED && !m_selection_panel->IsIgnoreGroupsEnabled())
				continue;

			//CBaseEditableObject* pObject = g_pLevel->GetEditable(i);

			bool bIsInside = false;

			if(pObject->GetType() <= EDITABLE_TERRAINPATCH)
			{
				bIsInside = IsAnyVertsInBBOX((CEditableSurface*)pObject, m_bbox_volume);
			}
			else
			{
				bIsInside = m_bbox_volume.IsBoxInside(	pObject->GetBoundingBoxMins().x,pObject->GetBoundingBoxMaxs().x,
														pObject->GetBoundingBoxMins().y,pObject->GetBoundingBoxMaxs().y,
														pObject->GetBoundingBoxMins().z,pObject->GetBoundingBoxMaxs().z);
			}
			
			if(bIsInside)
			{
				if(pObject->GetType() == EDITABLE_GROUP)
				{
					if(m_selection_panel->IsIgnoreGroupsEnabled())
						continue;

					CEditableGroup* pGroup = (CEditableGroup*)pObject;

					for(int j = 0; j < pGroup->GetGroupList()->numElem(); j++)
					{
						// don't select the hidden objects of group
						if(!(pGroup->GetGroupList()->ptr()[j]->GetFlags() & EDFL_HIDDEN))
							SUB_ToggleSelection(pGroup->GetGroupList()->ptr()[j]);
					}

					nGroupsSelected++;

					continue; // don't select group editable
				}

				SUB_ToggleSelection(pObject);
			}
			
		}
	}

	for(int i = 0; i < nObjects; i++)
	{
		CBaseEditableObject* pObject = pAddObjects[i];

		if(pObject->GetType() == EDITABLE_GROUP)
		{
			if(m_selection_panel->IsIgnoreGroupsEnabled())
				continue;

			CEditableGroup* pGroup = (CEditableGroup*)pObject;

			for(int j = 0; j < pGroup->GetGroupList()->numElem(); j++)
			{
				if(!(pGroup->GetGroupList()->ptr()[j]->GetFlags() & EDFL_HIDDEN))
					SUB_ToggleSelection(pGroup->GetGroupList()->ptr()[j]);
			}
			
			nGroupsSelected++;

			continue; // don't select group editable
		}

		SUB_ToggleSelection(pObject);
	}

	if(m_selectedobjects.numElem() == 0)
	{
		CancelSelection();
	}
	else
	{
		SetSelectionState(SELECTION_SELECTED);

		BoundingBox newBox;

		// set selection bbox from all selected objects
		for(int i = 0; i < m_selectedobjects.numElem(); i++)
		{
			DevMsg(2, "Selected id=%d\n", m_selectedobjects[i]->GetObjectID());

			// TODO: rotate them
			newBox.AddVertex(m_selectedobjects[i]->GetBBoxMins());
			newBox.AddVertex(m_selectedobjects[i]->GetBBoxMaxs());
		}

		SetSelectionBBOX(newBox.minPoint, newBox.maxPoint);
	}

	// notify that we updated the selection
	g_editormainframe->GetSurfaceDialog()->UpdateSelection();
	g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();

	g_editormainframe->UpdateAllWindows();


	if(m_selectedobjects.numElem() == 1)
	{
		if(m_selectedobjects[0]->GetType() == EDITABLE_BRUSH)
		{
			CEditableBrush* pBrush = (CEditableBrush*)m_selectedobjects[0];
			g_editormainframe->SetStatusText( varargs("Brush with %d faces", pBrush->GetFaceCount()), 1);
		}
		else if(m_selectedobjects[0]->GetType() == EDITABLE_ENTITY)
		{
			CEditableEntity* pEntity = (CEditableEntity*)m_selectedobjects[0];
			g_editormainframe->SetStatusText( varargs("Entity '%s' %s", pEntity->GetClassname(), pEntity->GetName() ? pEntity->GetName() : " "), 1);
		}
		else if(m_selectedobjects[0]->GetType() == EDITABLE_STATIC || m_selectedobjects[0]->GetType() == EDITABLE_TERRAINPATCH)
		{
			g_editormainframe->SetStatusText( "Static mesh", 1);
		}
	}
	else if(m_selectedobjects.numElem() > 1)
	{
		// check selection
		if(nGroupsSelected > 0)
			g_editormainframe->SetStatusText( varargs("Selected %d objects (in %d groups)", m_selectedobjects.numElem(), nGroupsSelected), 1);
		else
			g_editormainframe->SetStatusText( varargs("Selected %d objects", m_selectedobjects.numElem()), 1);
	}
	else
	{
		// check selection
		g_editormainframe->SetStatusText( "Nothing selected", 1);
	}
}

// dds object to selection, and objects will be selected after DoForEachSelectedObjects
void CSelectionBaseTool::OnDoForEach_AddSelectionObject(CBaseEditableObject* pObject)
{
	m_addToSelection.append(pObject);
}

// transforms by preview
void CSelectionBaseTool::TransformByPreview()
{
	if(GetSelectionState() == SELECTION_NONE)
		return;

	BoundingBox prev_bbox;
	m_bbox_volume.GetBBOXBack(prev_bbox.minPoint, prev_bbox.maxPoint);
	prev_bbox.Fix();

	BoundingBox bbox;

	GetSelectionBBOX( bbox.minPoint, bbox.maxPoint);
	bbox.Fix();

	m_bbox_volume.LoadBoundingBox(bbox.GetMinPoint(), bbox.GetMaxPoint());

	Vector3D snapped_scale = m_scaling;
	Vector3D snapped_move = m_translation;
	Vector3D snapped_rotation = m_rotation;//SnapVector(15, m_rotation);

	g_editormainframe->SnapVector3D(snapped_scale);
	g_editormainframe->SnapVector3D(snapped_move);

	// undo
	if(length(snapped_move) > 0.01f || length(snapped_rotation) > 0.01f || length(snapped_scale-Vector3D(1)) > 0.01f)
		BackupSelectionForUndo();
	
	Vector3D sel_center = GetSelectionCenter();

	if(GetSelectionState() == SELECTION_SELECTED)
	{
		Vector3D prev_box_size = prev_bbox.GetSize();
		Vector3D curr_box_size = bbox.GetSize();

		if(prev_box_size.x == 0)
			prev_box_size.x = 1.0f;
		if(prev_box_size.y == 0)
			prev_box_size.y = 1.0f;
		if(prev_box_size.z == 0)
			prev_box_size.z = 1.0f;

		if(curr_box_size.x == 0)
			curr_box_size.x = 1.0f;
		if(curr_box_size.y == 0)
			curr_box_size.y = 1.0f;
		if(curr_box_size.z == 0)
			curr_box_size.z = 1.0f;

		// scaling factor is valid for all objects
		Vector3D scale_factor = curr_box_size / prev_box_size;
		Vector3D vMove = (bbox.GetCenter() - prev_bbox.GetCenter());
		
		// transform and scale the objects
		for(int i = 0; i < m_selectedobjects.numElem(); i++)
		{
			m_selectedobjects[i]->BeginModify();

			m_selectedobjects[i]->Scale(scale_factor, (m_selectedobjects.numElem() > 1) && m_selection_panel->IsManipulationOverSelectionBox(), -bbox.GetCenter()*0.5f);
			m_selectedobjects[i]->Translate(vMove);
			m_selectedobjects[i]->Rotate(snapped_rotation, (m_selectedobjects.numElem() > 1) && m_selection_panel->IsManipulationOverSelectionBox(), bbox.GetCenter());

			m_selectedobjects[i]->EndModify();
		}

		// update selection box
		BoundingBox newbbox;

		// set selection bbox from all selected objects
		for(int i = 0; i < m_selectedobjects.numElem(); i++)
		{
			// TODO: rotate them
			newbbox.AddVertex(m_selectedobjects[i]->GetBoundingBoxMins());
			newbbox.AddVertex(m_selectedobjects[i]->GetBoundingBoxMaxs());
		}

		SetSelectionBBOX(newbbox.minPoint, newbbox.maxPoint);
	}

	m_scaling = vec3_zero;
	m_translation = vec3_zero;
	m_rotation = vec3_zero;


	// notify that we updated the selection
	g_editormainframe->GetSurfaceDialog()->UpdateSelection();

	g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();

	// update all
	g_editormainframe->UpdateAllWindows();
}

Vector3D sphere_pos(0);

void DkDrawSphere(const Vector3D& origin, float radius, int sides);
void DkDrawFilledSphere(const Vector3D& origin, float radius, int sides);


// draws selection box and objects for movement preview
void CSelectionBaseTool::Render3D(CEditorViewRender* pViewRender)
{
	if(GetSelectionState() == SELECTION_NONE)
		return;

	BoundingBox bbox;

	Vector3D snapped_rotation = m_rotation;//SnapVector(15, m_rotation);

	if(GetSelectionState() != SELECTION_PREPARATION && !g_editormainframe->GetSurfaceDialog()->IsSelectionMaskDisabled())
	{
		m_bbox_volume.GetBBOXBack(bbox.minPoint,bbox.maxPoint);

		// fix bbox
		bbox.Fix();

		// draw unmodified bbox
		if(pViewRender->GetCameraMode() != CPM_PERSPECTIVE && m_bIsDragging)
			pViewRender->DrawSelectionBBox( bbox.minPoint, bbox.maxPoint, ColorRGB(1,0.5f,0),false);

		BoundingBox currentbox;
		GetSelectionBBOX(currentbox.minPoint,currentbox.maxPoint);
		currentbox.Fix();

		Vector3D prev_box_size = bbox.GetSize();
		Vector3D curr_box_size = currentbox.GetSize();

		if(prev_box_size.x <= 0)
			prev_box_size.x = 1.0f;
		if(prev_box_size.y <= 0)
			prev_box_size.y = 1.0f;
		if(prev_box_size.z <= 0)
			prev_box_size.z = 1.0f;

		if(curr_box_size.x <= 0)
			curr_box_size.x = 1.0f;
		if(curr_box_size.y <= 0)
			curr_box_size.y = 1.0f;
		if(curr_box_size.z <= 0)
			curr_box_size.z = 1.0f;

		// scaling factor is valid for all objects
		Vector3D scale_factor = curr_box_size / prev_box_size;
		Vector3D vMove = (currentbox.GetCenter() - bbox.GetCenter()) / scale_factor;

		Matrix4x4 view, proj;
		pViewRender->GetViewProjection(view, proj);

		materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
		materials->SetMatrix(MATRIXMODE_VIEW, view);

		// TODO: draw ghost transformed selected objects
		for(int i = 0; i < m_selectedobjects.numElem(); i++)
		{
			materials->GetConfiguration().wireframeMode = true;
			materials->SetAmbientColor(ColorRGBA(1,0,0,1));
			materials->SetCullMode(CULL_NONE);

			m_selectedobjects[i]->RenderGhost(vMove, scale_factor, snapped_rotation, (m_selectedobjects.numElem() > 1) && m_selection_panel->IsManipulationOverSelectionBox(), -bbox.GetCenter()*0.5f);
		}
	}

	if(g_editormainframe->GetSurfaceDialog()->TerrainWireframeEnabled() && pViewRender->GetCameraMode() == CPM_PERSPECTIVE)
	{
		// TODO: draw ghost transformed selected objects
		for(int i = 0; i < m_selectedobjects.numElem(); i++)
		{
			if(m_selectedobjects[i]->GetType() > EDITABLE_TERRAINPATCH)
				continue;

			materials->GetConfiguration().wireframeMode = true;
			materials->SetAmbientColor(ColorRGBA(1,1,1,1));
			materials->SetCullMode(CULL_NONE);

			m_selectedobjects[i]->RenderGhost(vec3_zero, Vector3D(1), vec3_zero);
		}
	}

	// draw selection box
	if((pViewRender->GetCameraMode() == CPM_PERSPECTIVE) && (GetSelectionState() == SELECTION_PREPARATION))
		DrawSelectionBox(pViewRender);

	g_pShaderAPI->Reset();

	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	materials->SetMatrix(MATRIXMODE_PROJECTION, proj);
	materials->SetMatrix(MATRIXMODE_VIEW, view);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->GetConfiguration().wireframeMode = false;

	materials->SetAmbientColor(ColorRGBA(1,1,1,0.25f));
	materials->BindMaterial(g_pLevel->GetFlatMaterial());
	materials->SetCullMode(CULL_BACK);

	materials->Apply();

	if(g_editormainframe->GetSurfaceDialog()->GetPainterType() != PAINTER_NONE)
	{
		DkDrawSphere(sphere_pos, g_editormainframe->GetSurfaceDialog()->GetPainterSphereRadius(), 32);
		DkDrawFilledSphere(sphere_pos, g_editormainframe->GetSurfaceDialog()->GetPainterSphereRadius(), 16);
	}
}

void CSelectionBaseTool::Render2D(CEditorViewRender* pViewRender)
{
	if(GetSelectionState() == SELECTION_NONE)
		return;

	// draw selection box
	if(pViewRender->GetCameraMode() != CPM_PERSPECTIVE)
	{
		DrawSelectionBox(pViewRender);

		for(int i = 0; i < m_selectedobjects.numElem(); i++)
		{
			if(m_selectedobjects[i]->GetType() == EDITABLE_ENTITY)
			{
				CEditableEntity* pEnt = (CEditableEntity*)m_selectedobjects[i];

				for(int j = 0; j < pEnt->GetTypedVariableCount(); j++)
					pEnt->GetTypedVariable(j)->Render2D(pViewRender);
			}
		}
	}
}

void CSelectionBaseTool::DrawSelectionBox(CEditorViewRender* pView)
{
	BoundingBox bbox;

	// make various selection colors
	ColorRGB selectionbox_color(0.8,1,1);

	if(GetSelectionState() == SELECTION_SELECTED)
		selectionbox_color = g_editorCfg->selection_color;

	GetSelectionBBOX( bbox.minPoint, bbox.maxPoint);

	// draw ghost bbox
	pView->DrawSelectionBBox( bbox.minPoint, bbox.maxPoint, m_bIsDragging ? ColorRGB(0.8,0.8,0) : selectionbox_color, !m_bIsDragging);
}

// does anything with selected objects using callback
void CSelectionBaseTool::DoForEachSelectedObjects(DOFOREACHSELECTED pFunction, void *userdata, bool bUpdateSelection)
{
	if(GetSelectionState() == SELECTION_NONE)
		return;

	for(int i = 0; i < m_selectedobjects.numElem(); i++)
	{
		if((pFunction)(m_selectedobjects[i], userdata))
		{
			g_pLevel->RemoveEditable(m_selectedobjects[i]);
			i--;
		}
		else
		{
			if(bUpdateSelection)
				m_addToSelection.append(m_selectedobjects[i]);
		}
	}

	if(bUpdateSelection)
	{
		CancelSelection();

		SetSelectionState(SELECTION_PREPARATION);

		SelectObjects(true, m_addToSelection.ptr(),m_addToSelection.numElem());
	}
	m_addToSelection.clear();
}

SelectionHandle_e CSelectionBaseTool::GetMouseOverSelectionHandle()
{
	return (m_state == SELECTION_NONE) ? SH_NONE : m_mouseover_handle;
}

// 2D manipultaion
void CSelectionBaseTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	Vector2D screenDims = pViewRender->Get2DDimensions();

	// get bbox from volume
	BoundingBox bbox;
	m_bbox_volume.GetBBOXBack(bbox.minPoint,bbox.maxPoint);
	bbox.Fix();

	// convert bounding box to screen-aligned rectangle, for processing 3d.
	Rectangle_t rect;
	SelectionRectangleFromBBox(pViewRender, bbox.GetMinPoint(), bbox.GetMaxPoint(), rect);

	Rectangle_t windowRect;
	windowRect.vleftTop = 0;
	windowRect.vrightBottom = screenDims;

	Vector2D handles_pos[SH_COUNT];
	SelectionGetHandlePositions(rect, handles_pos, windowRect, 8);

	// fixup rectangle
	rect.Fix();

	// get mouse cursor position
	Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

	Vector3D ray_start, ray_dir;
	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, ray_start, ray_dir, proj*view, true);

	for(int i = 0; i < m_selectedobjects.numElem(); i++)
	{
		if(m_selectedobjects[i]->GetType() == EDITABLE_ENTITY)
		{
			CEditableEntity* pEnt = (CEditableEntity*)m_selectedobjects[i];

			for(int j = 0; j < pEnt->GetTypedVariableCount(); j++)
			{
				if(pEnt->GetTypedVariable(j)->UpdateManipulation2D(pViewRender, mouseEvents, delta3D, delta2D))
					return;
			}
		}
	}

	// check mouse over handle
	if(!m_bIsDragging)
	{
		m_mouseover_handle = SH_NONE;

		// check rectangle. Used to override center handle
		if(rect.IsInRectangle(mouse_position))
		{
			m_mouseover_handle = SH_CENTER;
		}
		else
		{
			// get selected handle
			for(int i = 0; i < SH_COUNT && m_mouseover_handle == SH_NONE; i++)
			{
				if(length(mouse_position - handles_pos[i]) < 4)
				{
					m_mouseover_handle = (SelectionHandle_e)i;
					break;
				}
			}
		}
	}

	// process pressing/depressing mouse events
	if(mouseEvents.GetButton() == wxMOUSE_BTN_LEFT)
	{
		if(mouseEvents.ButtonDown())
		{
			m_selectedhandle = SH_NONE;

			// check rectangle. Used to override center handle
			if(m_mouseover_handle == SH_CENTER)
			{
				m_selectedhandle = SH_CENTER;
			}
			else
			{
				// set a new selection handle from mouseover
				SetSelectionHandle(pViewRender, m_mouseover_handle);

				if(!mouseEvents.ControlDown())
				{
					// cancel selection if no handle is selected
					if(m_selectedhandle == SH_NONE)
						CancelSelection();
				}
			}

			Vector3D mouse_vec = normalize(ray_start - GetSelectionCenter());

			Vector3D rot_direction = sign(view.rows[0].xyz())*view.rows[0].xyz()*mouse_vec + sign(view.rows[1].xyz())*view.rows[1].xyz()*mouse_vec;

			Vector3D vUp = cross(view.rows[2].xyz(), mouse_vec);

			m_beginupvec = normalize(vUp);
			m_beginrightvec = normalize(rot_direction);
		}
		else
		{
			m_bIsDragging = false;

			// Apply movement to selected objects
			TransformByPreview();
		}
	}

	m_bIsDragging = false;


	// process dragging events
	if(mouseEvents.Dragging() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && !mouseEvents.ShiftDown())
	{
		Rectangle_t newRect;
		SelectionRectangleFromBBox(pViewRender, bbox.minPoint,bbox.maxPoint, newRect);

		if(m_selectedhandle != SH_NONE)
		{
			if(mouseEvents.AltDown())
			{
				if(IsDiagonalHandle(m_selectedhandle))
				{
					Vector3D mouse_vec = normalize(ray_start - GetSelectionCenter());

					Vector3D sel_center = GetSelectionCenter();

					Vector3D rot_direction = sign(view.rows[0].xyz())*view.rows[0].xyz()*mouse_vec + sign(view.rows[1].xyz())*view.rows[1].xyz()*mouse_vec;

					Vector3D vUp = normalize(cross(view.rows[2].xyz(), mouse_vec));
					rot_direction = normalize(rot_direction);

					Matrix3x3 mat1(rot_direction, vUp, view.rows[2].xyz());
					Matrix3x3 mat2(m_beginrightvec, m_beginupvec, view.rows[2].xyz());

					Matrix3x3 rotation = !mat2*mat1;

					Vector3D eulers = EulerMatrixXYZ(rotation);
					Vector3D angles = -VRAD2DEG(eulers);
					
					float dist = length(GetSelectionCenter() - ray_start);

					
					debugoverlay->Line3D(sel_center, sel_center+rotation.rows[0]*dist,ColorRGBA(1,0,0,0),ColorRGBA(1,0,0,1));
					debugoverlay->Line3D(sel_center, sel_center+rotation.rows[1]*dist,ColorRGBA(0,1,0,0),ColorRGBA(0,1,0,1));
					debugoverlay->Line3D(sel_center, sel_center+rotation.rows[2]*dist,ColorRGBA(0,0,1,0),ColorRGBA(0,0,1,1));
					
					/*
					debugoverlay->Line3D(sel_center, sel_center+rot_direction*dist,ColorRGBA(1,1,1,0),ColorRGBA(1,1,1,1));

					debugoverlay->Line3D(sel_center, sel_center+vUp*dist,ColorRGBA(0,1,0,0),ColorRGBA(0,1,0,1));
					debugoverlay->Line3D(sel_center, sel_center+view.rows[2].xyz()*dist,ColorRGBA(0,0,1,0),ColorRGBA(0,0,1,1));
					*/

					m_rotation = angles;
				}
			}
			else
			{
				if(m_selectedhandle == SH_CENTER)
				{
					m_translation += delta3D;
				}
				else
				{
					m_scaling += delta3D;
				}
			}

			m_bIsDragging = true;

			g_editormainframe->UpdateAllWindows();
		}

		if(!mouseEvents.ControlDown())
		{
			// if we have no selection, begin new one
			if(m_state == SELECTION_NONE)
			{
				Vector3D snapped_position = ray_start;
				g_editormainframe->SnapVector3D(snapped_position);

				BeginSelectionBox(pViewRender, snapped_position);
			}
		}
		else
		{
			if(m_state != SELECTION_PREPARATION)
			{
				// multiple object selection
				Vector3D snapped_position = ray_start;
				g_editormainframe->SnapVector3D(snapped_position);

				BeginSelectionBox(pViewRender, snapped_position);
			}
		}
	}

	if(mouseEvents.LeftDClick())
	{
		Vector3D x_decompose = view.rows[0].xyz();
		Vector3D y_decompose = view.rows[1].xyz();

		Vector3D snapped_position = ray_start;
		g_editormainframe->SnapVector3D(snapped_position);

		// set a new bbox
		m_bbox_volume.LoadBoundingBox(snapped_position,snapped_position+x_decompose+y_decompose);

		// set state to preparation
		m_state = SELECTION_PREPARATION;

		SetSelectionHandle(pViewRender, SH_CENTER);

		SelectObjects();
	}
}

// begins a new selection box
void CSelectionBaseTool::BeginSelectionBox(CEditorViewRender* pViewRender, Vector3D &start)
{
	// set a new bbox
	m_bbox_volume.LoadBoundingBox(start, start);

	// set state to preparation
	m_state = SELECTION_PREPARATION;

	SetSelectionHandle(pViewRender, SH_BOTTOMRIGHT);
}

// sets selection handle and also computes planes
void CSelectionBaseTool::SetSelectionHandle(CEditorViewRender* pViewRender, SelectionHandle_e handle)
{
	// select a handle
	m_selectedhandle = handle;

	m_plane_ids[0] = -1;
	m_plane_ids[1] = -1;

	if(m_selectedhandle != SH_CENTER && m_selectedhandle != SH_NONE)
	{
		SelectionGetVolumePlanes(pViewRender, m_selectedhandle, m_bbox_volume, m_plane_ids);

		if(m_plane_ids[0] == m_plane_ids[1])
			m_plane_ids[1] = -1;
	}
}

void CSelectionBaseTool::InvertSelection()
{
	DkList<CBaseEditableObject*> pObjects;
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		if(!(g_pLevel->GetEditable(i)->GetFlags() & EDFL_SELECTED))
		{
			pObjects.append(g_pLevel->GetEditable(i));
		}
	}

	CancelSelection();

	SetSelectionState(SELECTION_PREPARATION);

	SelectObjects(true, pObjects.ptr(), pObjects.numElem());
}

// vertex manipulator

void DkDrawSphere(const Vector3D& origin, float radius, int sides)
{
	CMeshBuilder meshbuild(materials->GetDynamicMesh());


	{
		meshbuild.Begin(PRIM_LINES);

		for (int i = 0; i <= sides; i++)
		{
			double ds = sin((i * 2 * PI) / sides);
			double dc = cos((i * 2 * PI) / sides);

			meshbuild.Position3f(
						static_cast<float>(origin[0] + radius * dc),
						static_cast<float>(origin[1] + radius * ds),
						origin[2]
						);

			meshbuild.Color4f(1,1,1,1);

			meshbuild.AdvanceVertex();
		}

		meshbuild.End();
	}

	{
		meshbuild.Begin(PRIM_LINES);

		for (int i = 0; i <= sides; i++)
		{
			double ds = sin((i * 2 * PI) / sides);
			double dc = cos((i * 2 * PI) / sides);

			meshbuild.Position3f(
						static_cast<float>(origin[0] + radius * dc),
						origin[1],
						static_cast<float>(origin[2] + radius * ds)
						);

			meshbuild.AdvanceVertex();
		}

		meshbuild.End();
	}

	{
		meshbuild.Begin(PRIM_LINES);

		for (int i = 0; i <= sides; i++)
		{
			double ds = sin((i * 2 * PI) / sides);
			double dc = cos((i * 2 * PI) / sides);

			meshbuild.Position3f(
						origin[0],
						static_cast<float>(origin[1] + radius * dc),
						static_cast<float>(origin[2] + radius * ds)
						);

			meshbuild.AdvanceVertex();
		}

		meshbuild.End();
	}
}

extern Vector3D v3sphere(float theta, float phi);

void DkDrawFilledSphere(const Vector3D &origin, float radius, int sides)
{
	if (radius <= 0)
		return;

	float dt = PI*2.0f / float(sides);
	float dp = PI / float(sides);

	CMeshBuilder meshbuild(materials->GetDynamicMesh());

	meshbuild.Begin(PRIM_TRIANGLES);
	for (int i = 0; i <= sides - 1; i++)
	{
		for (int j = 0; j <= sides - 2; j++)
		{
			const double t = i * dt;
			const double p = (j * dp) - (PI * 0.5f);

			{
				Vector3D v(origin + (v3sphere(t, p) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t, p + dp) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t + dt, p + dp) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t, p) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t + dt, p + dp) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t + dt, p) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}
		}
	}

	{
		const double p = (sides - 1) * dp - (PI * 0.5f);
		for (int i = 0; i <= sides - 1; i++)
		{
			const double t = i * dt;

			{
				Vector3D v(origin + (v3sphere(t, p) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t + dt, p + dp) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}

			{
				Vector3D v(origin + (v3sphere(t + dt, p) * radius));
				meshbuild.Position3fv(v);
				meshbuild.AdvanceVertex();
			}
		}
	}
	meshbuild.End();
}

void DkDrawCone(const Vector3D& origin, const Vector3D& angles, float distance, float fFov, int sides)
{
	CMeshBuilder meshbuild(materials->GetDynamicMesh());

	Vector3D radAngles = VDEG2RAD(angles);

	float fConeR = distance*sin(DEG2RAD(fFov));
	float fConeL = sqrt(distance*distance + fConeR*fConeR);
	
	meshbuild.Begin(PRIM_LINES);

	meshbuild.Position3fv( origin );
	meshbuild.AdvanceVertex();

	Matrix3x3 dir = !rotateXYZ3(radAngles.x,radAngles.y,radAngles.z);

	meshbuild.Position3fv( origin+dir.rows[2]*distance );
	meshbuild.AdvanceVertex();

	{
		meshbuild.Position3fv( origin );
		meshbuild.AdvanceVertex();

		Vector3D vec = origin + dir.rows[2]*distance;

		meshbuild.Position3fv( vec );
		meshbuild.AdvanceVertex();
	}

	int nTestSides = sides / 4;

	for (int i = 0; i < nTestSides; i++)
	{
		meshbuild.Position3fv( origin );
		meshbuild.AdvanceVertex();

		Matrix3x3 r = rotateXYZ3(0.0f,DEG2RAD(fFov*0.5f), (float)((i * 2.0f * PI) / nTestSides))*dir;
		Vector3D vec = origin+r.rows[0] + r.rows[2]*distance;

		meshbuild.Position3fv( vec );
		meshbuild.AdvanceVertex();
	}

	for (int i = 0; i < sides; i++)
	{
		Matrix3x3 r = rotateXYZ3(0.0f,DEG2RAD(fFov*0.5f),(float)((i * 2.0f * PI) / sides))*dir;
		Vector3D vec = origin+r.rows[0] + r.rows[2]*distance;

		meshbuild.Position3fv( vec );
		meshbuild.AdvanceVertex();
	}

	meshbuild.End();
}

struct searchvertexdata_t
{
	Vector3D line_start;
	Vector3D line_end;

	Vector3D out_pos;

	float min_dist;
};

bool SearchForNearestVertex(CBaseEditableObject* selection, void* userdata)
{
	// don't check entities
	if(selection->GetType() > EDITABLE_TERRAINPATCH)
		return false;

	searchvertexdata_t* pData = (searchvertexdata_t*)userdata;

	CEditableSurface* pSurf = (CEditableSurface*)selection;

	Vector3D intersect_pos;

	float fraction = pSurf->CheckLineIntersection(pData->line_start, pData->line_end, intersect_pos);

	if(fraction < pData->min_dist)
	{
		pData->min_dist = fraction;
		pData->out_pos = intersect_pos;
	}

	/*
	// check mouse ray for nearest verts in selected object
	CEditableSurface* pSurf = (CEditableSurface*)selection;

	for(int j = 0; j < pSurf->GetVertexCount(); j++)
	{
		float line_interp = lineProjection(pData->line_start, pData->line_end, pSurf->GetVertices()[j].position);

		Vector3D interp_point = lerp(pData->line_start, pData->line_end, line_interp);

		float dist = length(interp_point - pSurf->GetVertices()[j].position);
		float dist_to_view = length(interp_point - pData->line_start);

		if(dist < pData->min_dist)
		{
			pData->min_dist = dist;
			//min_dist_to_view = dist_to_view;

			pData->out_pos = pSurf->GetVertices()[j].position;
			g_editormainframe->UpdateAllWindows();
		}
	}
	*/

	return false;
}

struct paintdata_t
{
	bool	inverse;
	float	sphere_radius;
	float	sphere_power;
};

bool PaintVerts(CBaseEditableObject* selection, void* userdata)
{
	// don't check entities
	if(selection->GetType() > EDITABLE_TERRAINPATCH)
		return false;

	paintdata_t* pData = (paintdata_t*)userdata;

	float sphere_radius = pData->sphere_radius;

	CEditableSurface* pSurf = (CEditableSurface*)selection;
	pSurf->BeginModify();

	for(int j = 0; j < pSurf->GetVertexCount(); j++)
	{
		// determine sphere factor
		float distance = length(sphere_pos - pSurf->GetVertices()[j].position);

		float dist_factor = 1.0f - clamp(distance / sphere_radius, 0,1);

		Vector3D dir = pSurf->GetVertices()[j].position - sphere_pos;

		if(length(dir) > sphere_radius)
			continue;

		float fPower = dist_factor * pData->sphere_power * 0.01f;

		Vector3D vAxis(0);
		int nAxis = g_editormainframe->GetSurfaceDialog()->GetPaintAxis();
		vAxis[nAxis] = 1.0f;

		int nPaintLayer = g_editormainframe->GetSurfaceDialog()->GetPaintTextureLayer();
		Vector4D vLayer(0);
		vLayer[nPaintLayer] = 1.0f;

		switch(g_editormainframe->GetSurfaceDialog()->GetPainterType())
		{
			case PAINTER_VERTEX_ADVANCE:
				pSurf->GetVertices()[j].position += vAxis*fPower * (pData->inverse ? -1.0f : 1.0f);
				break;
			case PAINTER_VERTEX_SMOOTH:
				pSurf->GetVertices()[j].position -= dir*vAxis*fabs(fPower)*0.1f;
				break;
			case PAINTER_VERTEX_SETLEVEL:
				pSurf->GetVertices()[j].position -= dir*vAxis;
				break;
			case PAINTER_TEXTURETRANSITION:
				pSurf->GetVertices()[j].vertexPaint += vLayer * (pData->inverse ? -1.0f : 1.0f)*fPower*0.1f;//lerp(pSurf->GetVertices()[j].vertexPaint, vLayer, fPower*0.1f);
				pSurf->GetVertices()[j].vertexPaint = clamp(pSurf->GetVertices()[j].vertexPaint,vec4_zero, Vector4D(1));
				break;
		}

		
	}
	pSurf->EndModify();

	return false;
}

// manipulation for 3D. Much simplified
void CSelectionBaseTool::UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	Vector2D screenDims = pViewRender->Get2DDimensions();

	// get mouse cursor position
	Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

	Vector3D forward = view.rows[2].xyz();

	// setup edit plane
	Plane pl(forward.x,forward.y,forward.z, -dot(forward, GetSelectionCenter()));

	Vector3D ray_start, ray_dir;

	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-(mouse_position.x-delta2D.x),mouse_position.y-delta2D.y), screenDims, ray_start, ray_dir, proj*view, false);

	Vector3D curr_ray_start, curr_ray_dir;

	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, curr_ray_start, curr_ray_dir, proj*view, false);

	/*
	Vector3D check_pos = GetSelectionCenter();
	CBaseEditableObject* pObject = g_pLevel->GetRayNearestObject(ray_start,ray_dir, check_pos);

	if(pObject)
	{
		// redefine plane if we have collsion
		pl = Plane(forward.x,forward.y,forward.z, dot(forward, check_pos));
	}
	*/

	Vector3D intersect1, intersect2;
	pl.GetIntersectionWithRay(curr_ray_start, normalize(curr_ray_dir),intersect1);
	pl.GetIntersectionWithRay(ray_start, normalize(ray_dir),intersect2);

	Vector3D projected_move_delta = (intersect1 - intersect2);

	if(pl.ClassifyPoint(pViewRender->GetAbsoluteCameraPosition()) == CP_FRONT)
		projected_move_delta *= -1;

	// try to select object on clicking.
	if(mouseEvents.GetButton() == wxMOUSE_BTN_LEFT)
	{
		if(mouseEvents.ButtonDown() && !mouseEvents.ShiftDown())
		{
			if(mouseEvents.ControlDown())
			{
				int nSelectionFilter = m_selection_panel->GetSelectionFlags();

				if(m_selection_panel->IsIgnoreGroupsEnabled())
					nSelectionFilter |= NR_FLAG_INCLUDEGROUPPED;
				else
					nSelectionFilter |= NR_FLAG_GROUP;

				CBaseEditableObject* pObject = g_pLevel->GetRayNearestObject(ray_start, ray_start+ray_dir, vec3_zero, nSelectionFilter);
				if(pObject)
				{
					SetSelectionState(SELECTION_PREPARATION);
					SelectObjects(true, &pObject, 1);
				}
			}
			else if(m_state != SELECTION_NONE && g_editormainframe->GetSurfaceDialog()->GetPainterType() == PAINTER_NONE)
			{
				Vector3D pos;
				// get bbox from volume
				if(!m_bbox_volume.IsIntersectsRay(ray_start, normalize(ray_dir), pos))
					CancelSelection();
			}
		}
		else
		{
			// Apply movement to selected objects
			if(m_bIsDragging)
				TransformByPreview();

			m_bIsDragging = false;
		}
	}

	// try to select object on clicking.
	if(mouseEvents.GetButton() == wxMOUSE_BTN_RIGHT)
	{
		if(mouseEvents.ButtonDown() && !mouseEvents.ShiftDown())
		{
			if(mouseEvents.ControlDown())
			{
				Vector3D hit_pos;
				CBaseEditableObject* pObject = g_pLevel->GetRayNearestObject(ray_start, ray_start+ray_dir, hit_pos,NR_FLAG_BRUSHES|NR_FLAG_SURFACEMODS|NR_FLAG_INCLUDEGROUPPED);
				
				if(pObject)
				{
					if(pObject->GetType() == EDITABLE_BRUSH)
					{
						// select only surface
						CEditableBrush *pBrush = (CEditableBrush*)pObject;
						for(int i = 0; i < pBrush->GetSurfaceTextureCount(); i++)
						{
							if(pBrush->GetSurfaceTexture(i)->Plane.ClassifyPoint(hit_pos) == CP_ONPLANE)
							{
								if(!(pBrush->GetSurfaceTexture(i)->nFlags & STFL_SELECTED))
									pBrush->GetSurfaceTexture(i)->nFlags |= STFL_SELECTED;
								else
									pBrush->GetSurfaceTexture(i)->nFlags &= ~STFL_SELECTED;
							}
						}
					}
					else if(pObject->GetType() != EDITABLE_ENTITY)
					{
						// select whole object
						SetSelectionState(SELECTION_PREPARATION);
						SelectObjects(true, &pObject, 1);
					}
				}
				
				g_editormainframe->GetSurfaceDialog()->UpdateSelection();
			}
		}
	}

	if(mouseEvents.ControlDown())
		return;

	if(GetSelectionState() == SELECTION_NONE)
		return;

	// get selection point
	searchvertexdata_t vertex_search;
	vertex_search.line_start = curr_ray_start;
	vertex_search.line_end = curr_ray_start+curr_ray_dir;
	vertex_search.min_dist = MAX_COORD_UNITS;

	DoForEachSelectedObjects(SearchForNearestVertex, &vertex_search);
	sphere_pos = vertex_search.out_pos;

	paintdata_t paint_props;
	paint_props.sphere_power = g_editormainframe->GetSurfaceDialog()->GetPainterPower();
	paint_props.sphere_radius = g_editormainframe->GetSurfaceDialog()->GetPainterSphereRadius();

	// drag selection box, modify terrain
	if(mouseEvents.Dragging() && !mouseEvents.ShiftDown())
	{
		if(mouseEvents.AltDown() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT) && g_editormainframe->GetSurfaceDialog()->GetPainterType() == PAINTER_NONE)
		{

			Vector3D x_decompose = view.rows[0].xyz();
			Vector3D y_decompose = view.rows[1].xyz();
			Vector3D z_decompose = view.rows[2].xyz();

			Vector3D rotationdelta(	PI/64.0*( y_decompose.x*delta2D.x+x_decompose.x*delta2D.y), 
									PI/64.0*( y_decompose.y*delta2D.x+x_decompose.y*delta2D.y),
									PI/64.0*( y_decompose.z*delta2D.x+x_decompose.z*delta2D.y) );

			Vector3D rot = VRAD2DEG(rotationdelta);

			m_rotation += rot*0.5f;
			m_bIsDragging = true;
		}
		else
		{
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
			{
				if(g_editormainframe->GetSurfaceDialog()->GetPainterType() != PAINTER_NONE)
				{
					if(!m_bIsPainting)
						BackupSelectionForUndo();

					m_bIsPainting = true;

					paint_props.inverse = false;

					DoForEachSelectedObjects(PaintVerts, &paint_props);
				}
				else
				{
					m_translation += projected_move_delta;
					m_bIsDragging = true;
				}
			}
			
			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_RIGHT))
			{
				if(g_editormainframe->GetSurfaceDialog()->GetPainterType() != PAINTER_NONE)
				{
					if(!m_bIsPainting)
						BackupSelectionForUndo();

					m_bIsPainting = true;

					paint_props.inverse = true;

					DoForEachSelectedObjects(PaintVerts, &paint_props);
				}
			}
		}
		
		g_editormainframe->UpdateAllWindows();
	}

	if(mouseEvents.ButtonUp(wxMOUSE_BTN_LEFT) || mouseEvents.ButtonUp(wxMOUSE_BTN_RIGHT) && m_bIsPainting)
		m_bIsPainting = false;
}

Vector3D CSelectionBaseTool::GetSelectionCenter()
{
	Vector3D mins, maxs;
	GetSelectionBBOX(mins, maxs);
	
	return (mins+maxs)*0.5f;
}

// selection bbox
void CSelectionBaseTool::GetSelectionBBOX(Vector3D &mins, Vector3D &maxs)
{
	BoundingBox bbox;

	// and now draw transformed ghost bbox
	Volume drawnboxvolume = m_bbox_volume;

	Vector3D snapped_scale = m_scaling;
	Vector3D snapped_move = m_translation;

	g_editormainframe->SnapVector3D(snapped_scale);
	g_editormainframe->SnapVector3D(snapped_move);

	for(int i = 0; i < 2; i++)
	{
		if(m_plane_ids[i] != -1)
		{
			Plane move_plane = drawnboxvolume.GetPlane(m_plane_ids[i]);

			move_plane.offset -= dot(move_plane.normal, snapped_scale);

			drawnboxvolume.SetupPlane(move_plane, m_plane_ids[i]);
		}
	}

	// get bbox back and fix
	drawnboxvolume.GetBBOXBack(bbox.minPoint,bbox.maxPoint);
	bbox.Fix();

	bbox.minPoint += snapped_move;
	bbox.maxPoint += snapped_move;

	mins = bbox.minPoint;
	maxs = bbox.maxPoint;
}

// sets selection box.
void CSelectionBaseTool::SetSelectionBBOX(Vector3D &mins, Vector3D &maxs)
{
	if(mins.x >= maxs.x)
		maxs.x = mins.x+1;

	if(mins.y >= maxs.y)
		maxs.y = mins.y+1;

	if(mins.z >= maxs.z)
		maxs.z = mins.z+1;

	m_bbox_volume.LoadBoundingBox(mins, maxs);
	g_editormainframe->UpdateAllWindows();
}

// realize as global
static CSelectionBaseTool	s_Selection;
static CClipperTool			s_Clipper;
static CBlockTool			s_BlockTool;
static CVertexTool			s_VertexTool;
static CTerrainPatchTool	s_TerrainPatchTool;
static CEntityTool			s_EntityTool;
static CDecalTool			s_DecalTool;

IEditorTool* g_pSelectionTools[Tool_Count] = 
{
	&s_Selection,
	&s_BlockTool,
	NULL,
	&s_TerrainPatchTool,
	&s_EntityTool,
	NULL,
	&s_Clipper,
	&s_VertexTool,
	&s_DecalTool,
};