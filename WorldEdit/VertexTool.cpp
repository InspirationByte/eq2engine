//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Vertex tool
//////////////////////////////////////////////////////////////////////////////////

#include "VertexTool.h"
#include "EditorHeader.h"
#include "EqBrush.h"
#include "EditableSurface.h"
#include "IDebugOverlay.h"

#include "mtriangle_framework.h"

using namespace MTriangle;

// create a terrain patch dialog
class CVertexToolPanel : public wxPanel
{
public:
	CVertexToolPanel(wxWindow* pMultiToolPanel) : wxPanel(pMultiToolPanel,0,0,200,400)
	{
		m_nEditMode = VERTEXEDIT_VERTEX;

		wxString editChoices[] = 
		{
			DKLOC("TOKEN_VERTICES", "Vertices"),
			DKLOC("TOKEN_POLYGONS", "Polygons"),
		};

		m_pEditModeBox = new wxRadioBox(this, -1, DKLOC("TOKEN_VERTEDITMODE", "Edit mode"), wxPoint(5,5), wxSize(190, 80), 2, editChoices);
		m_pEditModeBox->SetSelection(0);
	}

	void OnChangeEditMode(wxCommandEvent& event)
	{
		((CVertexTool*)g_pSelectionTools[Tool_VertexManip])->CancelSelection();
		((CVertexTool*)g_pSelectionTools[Tool_VertexManip])->ClearSelection();
		UpdateAllWindows();
	}

	VertexEditMode_e GetEditMode() 
	{
		if(m_pEditModeBox->GetSelection() != -1)
			m_nEditMode = (VertexEditMode_e)m_pEditModeBox->GetSelection();

		return m_nEditMode;
	}

protected:

	DECLARE_EVENT_TABLE();

	VertexEditMode_e	m_nEditMode;
	wxRadioBox*			m_pEditModeBox;
};

BEGIN_EVENT_TABLE(CVertexToolPanel, wxPanel)
	EVT_RADIOBOX(-1, CVertexToolPanel::OnChangeEditMode)
END_EVENT_TABLE()

// tool settings panel, can be null
wxPanel* CVertexTool::GetToolPanel()
{
	return m_pPanel;
}

void CVertexTool::InitializeToolPanel(wxWindow* pMultiToolPanel)
{
	m_pPanel = new CVertexToolPanel(pMultiToolPanel);
}

void RenderBrushVerts(CEditorViewRender* pViewRender, CEditableBrush* pBrush)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	materials->Setup2D(pViewRender->Get2DDimensions().x, pViewRender->Get2DDimensions().y);

	for(int i = 0; i < pBrush->GetFaceCount(); i++)
	{
		for(int j = 0; j < pBrush->GetFacePolygon(i)->vertices.numElem(); j++)
		{
			Vector2D pointScr;
			if(!PointToScreen(pBrush->GetFacePolygon(i)->vertices[j].position, pointScr, proj*view, pViewRender->Get2DDimensions()))
			{
				// draw only single point
				Vertex2D_t pointA[] = {MAKETEXQUAD(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};

				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, ColorRGBA(1,1,0.5f,1));

				// draw only single point
				Vertex2D_t pointR[] = {MAKETEXRECT(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};
				materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
			}
		}
	}
}

void RenderSurfaceVerts(CEditorViewRender* pViewRender, CEditableSurface* pSurface)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	materials->Setup2D(pViewRender->Get2DDimensions().x, pViewRender->Get2DDimensions().y);

	for(int i = 0; i < pSurface->GetVertexCount(); i++)
	{
		Vector2D pointScr;
		if(!PointToScreen(pSurface->GetVertices()[i].position, pointScr, proj*view, pViewRender->Get2DDimensions()))
		{
			// draw only single point
			Vertex2D_t pointA[] = {MAKETEXQUAD(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, ColorRGBA(1,1,0.5f,1));

			// draw only single point
			Vertex2D_t pointR[] = {MAKETEXRECT(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};
			materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
		}
	}
}

void RenderSurfaceSelection(CEditorViewRender* pViewRender, surfaceSelectionData_t* pData, Vector3D &selectionmove, Vector3D &rotation, Vector3D &rotationcenter)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	materials->Setup2D(pViewRender->Get2DDimensions().x, pViewRender->Get2DDimensions().y);

	for(int i = 0; i < pData->vertex_ids.numElem(); i++)
	{
		int vertex_id = pData->vertex_ids[i];

		Vector3D point3D = pData->pSurface->GetVertices()[vertex_id].position+selectionmove - rotationcenter;
		Matrix3x3 rotMatrix = rotateXYZ3(DEG2RAD(rotation.x),DEG2RAD(rotation.y),DEG2RAD(rotation.z));
		point3D = rotMatrix*point3D;
		point3D += rotationcenter;

		Vector2D pointScr;
		if(!PointToScreen(point3D, pointScr, proj*view, pViewRender->Get2DDimensions()))
		{
			// draw only single point
			Vertex2D_t pointA[] = {MAKETEXQUAD(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, ColorRGBA(1,0,0,1));

			// draw only single point
			Vertex2D_t pointR[] = {MAKETEXRECT(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};
			materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
		}
	}

	pViewRender->SetupRenderMatrices();

	IMeshBuilder* pMesh = g_pShaderAPI->CreateMeshBuilder();

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	materials->SetAmbientColor(ColorRGBA(1,1,1,0.15f));
	materials->SetCullMode(CULL_NONE);
	materials->BindMaterial(g_pLevel->GetFlatMaterial());
	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
	materials->SetDepthStates(true, false);
	g_pShaderAPI->Apply();

	pMesh->Begin(PRIM_TRIANGLES);
	for(int i = 0; i < pData->pSurface->GetIndexCount(); i++)
	{
		int idx = pData->pSurface->GetIndices()[i];

		bool bMove = false;
		if(pData->vertex_ids.findIndex(idx) != -1)
			bMove = true;

		pMesh->Position3fv(pData->pSurface->GetVertices()[idx].position + (bMove ? selectionmove : vec3_zero));
		pMesh->AdvanceVertex();
	}
	pMesh->End();
	
	materials->SetAmbientColor(ColorRGBA(1,1,1,1));
	materials->BindMaterial(g_pLevel->GetFlatMaterial());
	materials->SetRasterizerStates(CULL_NONE,FILL_WIREFRAME);
	g_pShaderAPI->Apply();

	pMesh->Begin(PRIM_TRIANGLES);
	for(int i = 0; i < pData->pSurface->GetIndexCount(); i++)
	{
		int idx0 = pData->pSurface->GetIndices()[i];
		int idx1 = pData->pSurface->GetIndices()[i+1];
		int idx2 = pData->pSurface->GetIndices()[i+2];

		if(pData->vertex_ids.findIndex(idx0) != -1)
		{
			Vector3D point3D = pData->pSurface->GetVertices()[idx0].position + selectionmove - rotationcenter;

			Matrix3x3 rotMatrix = rotateXYZ3(DEG2RAD(rotation.x),DEG2RAD(rotation.y),DEG2RAD(rotation.z));
			point3D = rotMatrix*point3D;
			point3D += rotationcenter;

			pMesh->Position3fv(point3D);

		}
		else
		{
			pMesh->Position3fv(pData->pSurface->GetVertices()[idx0].position);

		}

		pMesh->AdvanceVertex();
	}
	pMesh->End();

	g_pShaderAPI->DestroyMeshBuilder(pMesh);
}

void RenderTriangleSelection(CEditorViewRender* pViewRender, surfaceTriangleSelectionData_t* pData)
{
	
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	pViewRender->SetupRenderMatrices();

	IMeshBuilder* pMesh = g_pShaderAPI->CreateMeshBuilder();

	g_pShaderAPI->Reset(STATE_RESET_VBO);
	materials->SetAmbientColor(ColorRGBA(1,1,1,0.75f));
	materials->SetCullMode(CULL_NONE);
	materials->BindMaterial(g_pLevel->GetFlatMaterial(), 0);
	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
	materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
	materials->SetDepthStates(true, false);
	g_pShaderAPI->Apply();

	pMesh->Begin(PRIM_TRIANGLES);

	int nTriangles = pData->triangle_ids.numElem();

	for(int i = 0; i < nTriangles; i++)
	{
		int idx0 = pData->pSurface->GetIndices()[pData->triangle_ids[i] * 3];
		int idx1 = pData->pSurface->GetIndices()[pData->triangle_ids[i] * 3 + 1];
		int idx2 = pData->pSurface->GetIndices()[pData->triangle_ids[i] * 3 + 2];

		pMesh->Position3fv( pData->pSurface->GetVertices()[idx0].position );
		pMesh->AdvanceVertex();

		pMesh->Position3fv( pData->pSurface->GetVertices()[idx1].position );
		pMesh->AdvanceVertex();

		pMesh->Position3fv( pData->pSurface->GetVertices()[idx2].position );
		pMesh->AdvanceVertex();
	}
	pMesh->End();
	/*
	materials->SetAmbientColor(ColorRGBA(1,1,1,1));
	materials->BindMaterial(g_pLevel->GetFlatMaterial(), 0);
	materials->SetRasterizerStates(CULL_NONE,FILL_WIREFRAME);
	g_pShaderAPI->Apply();

	pMesh->Begin(PRIM_TRIANGLES);
	for(int i = 0; i < pData->pSurface->GetIndexCount(); i++)
	{
		int idx0 = pData->pSurface->GetIndices()[i];
		int idx1 = pData->pSurface->GetIndices()[i+1];
		int idx2 = pData->pSurface->GetIndices()[i+2];

		if(pData->vertex_ids.findIndex(idx0) != -1)
		{
			Vector3D point3D = pData->pSurface->GetVertices()[idx0].position + selectionmove - rotationcenter;

			Matrix3x3 rotMatrix = rotateXYZ(DEG2RAD(rotation.x),DEG2RAD(rotation.y),DEG2RAD(rotation.z)).getRotationComponent();
			point3D = rotMatrix*point3D;
			point3D += rotationcenter;

			pMesh->Position3fv(point3D);
		}
		else
		{
			pMesh->Position3fv(pData->pSurface->GetVertices()[idx0].position);
		}

		pMesh->AdvanceVertex();
	}
	pMesh->End();
	*/
	g_pShaderAPI->DestroyMeshBuilder(pMesh);
	
}

void RenderBrushVertexSelection(CEditorViewRender* pViewRender, brushVertexSelectionData_t* pData, Vector3D &selectionmove, Vector3D &rotation, Vector3D &rotationcenter)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view, proj);

	materials->Setup2D(pViewRender->Get2DDimensions().x, pViewRender->Get2DDimensions().y);

	for(int i = 0; i < pData->selected_polys.numElem(); i++)
	{
		int face_id = pData->selected_polys[i].face_id;
		for(int j = 0; j < pData->selected_polys[i].vertex_ids.numElem(); j++)
		{
			int vertex_id = pData->selected_polys[i].vertex_ids[j];

			Vector3D point3D = pData->pBrush->GetFacePolygon(face_id)->vertices[vertex_id].position+selectionmove - rotationcenter;
			Matrix3x3 rotMatrix = rotateXYZ3(DEG2RAD(rotation.x),DEG2RAD(rotation.y),DEG2RAD(rotation.z));
			point3D = rotMatrix*point3D;
			point3D += rotationcenter;

			Vector2D pointScr;
			if(!PointToScreen(point3D, pointScr, proj*view, pViewRender->Get2DDimensions()))
			{
				// draw only single point
				Vertex2D_t pointA[] = {MAKETEXQUAD(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};

				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, pointA, elementsOf(pointA), NULL, ColorRGBA(1,0,0,1));

				// draw only single point
				Vertex2D_t pointR[] = {MAKETEXRECT(pointScr.x-3,pointScr.y-3,pointScr.x+3,pointScr.y+3, 0)};
				materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, pointR, elementsOf(pointR), NULL, ColorRGBA(0,0,0,1));
			}
		}
	}

	pViewRender->SetupRenderMatrices();

	IMeshBuilder* pMesh = g_pShaderAPI->CreateMeshBuilder();

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	for(int i = 0; i < pData->pBrush->GetFaceCount(); i++)
	{
		int face_id = -1;
		for(int j = 0; j < pData->selected_polys.numElem(); j++)
		{
			if(pData->selected_polys[j].face_id == i)
			{
				face_id = j;
				break;
			}
		}

		materials->SetAmbientColor(ColorRGBA(1,1,1,0.15f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), 0);
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
		materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
		materials->SetDepthStates(true, false);
		g_pShaderAPI->Apply();

		pMesh->Begin(PRIM_TRIANGLE_FAN);
		for(int j = 0; j < pData->pBrush->GetFacePolygon(i)->vertices.numElem(); j++)
		{
			if(face_id != -1)
			{
				int vertex_id = pData->selected_polys[face_id].vertex_ids.findIndex(j);

				if(vertex_id != -1)
				{
					Vector3D point3D = pData->pBrush->GetFacePolygon(i)->vertices[j].position + selectionmove - rotationcenter;
					Matrix3x3 rotMatrix = rotateXYZ3(DEG2RAD(rotation.x),DEG2RAD(rotation.y),DEG2RAD(rotation.z));
					point3D = rotMatrix*point3D;
					point3D += rotationcenter;

					pMesh->Position3fv(point3D);
				}
				else
					pMesh->Position3fv(pData->pBrush->GetFacePolygon(i)->vertices[j].position);
			}
			else
				pMesh->Position3fv(pData->pBrush->GetFacePolygon(i)->vertices[j].position);

			pMesh->AdvanceVertex();
		}
		pMesh->End();

		materials->SetAmbientColor(ColorRGBA(1));
		materials->BindMaterial(g_pLevel->GetFlatMaterial(), 0);
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD);
		materials->SetRasterizerStates(CULL_NONE,FILL_SOLID);
		materials->SetDepthStates(true, false);
		g_pShaderAPI->Apply();

		pMesh->Begin(PRIM_LINE_STRIP);
		for(int j = 0; j < pData->pBrush->GetFacePolygon(i)->vertices.numElem(); j++)
		{
			if(face_id != -1)
			{
				int vertex_id = pData->selected_polys[face_id].vertex_ids.findIndex(j);

				if(vertex_id != -1)
					pMesh->Position3fv(pData->pBrush->GetFacePolygon(i)->vertices[j].position + selectionmove);
				else
					pMesh->Position3fv(pData->pBrush->GetFacePolygon(i)->vertices[j].position);
			}
			else
				pMesh->Position3fv(pData->pBrush->GetFacePolygon(i)->vertices[j].position);

			pMesh->AdvanceVertex();
		}
		pMesh->End();
	}

	g_pShaderAPI->DestroyMeshBuilder(pMesh);
}

CVertexTool::CVertexTool()
{

}

// rendering of clipper
void CVertexTool::Render3D(CEditorViewRender* pViewRender)
{
	g_pSelectionTools[0]->Render3D(pViewRender);

	DkList<CBaseEditableObject*> *objs = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();

	Vector3D snapped_move = m_selectionoffset;
	SnapVector3D(snapped_move);

	if( m_pPanel->GetEditMode() == VERTEXEDIT_VERTEX )
	{
		for(int i = 0; i < objs->numElem(); i++)
		{
			if(objs->ptr()[i]->GetType() == EDITABLE_BRUSH)
			{
				RenderBrushVerts(pViewRender, (CEditableBrush*)objs->ptr()[i]);
			}
			else if(objs->ptr()[i]->GetType() <= EDITABLE_TERRAINPATCH)
			{
				RenderSurfaceVerts(pViewRender, (CEditableSurface*)objs->ptr()[i]);
			}
		}

		for(int i = 0; i < m_vertexselection.selectedBrushes.numElem(); i++)
		{
			RenderBrushVertexSelection(pViewRender, &m_vertexselection.selectedBrushes[i], snapped_move, m_rotation, m_selectioncenter);
		}

		for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
		{
			RenderSurfaceSelection(pViewRender, &m_vertexselection.selectedSurfaces[i], snapped_move, m_rotation, m_selectioncenter);
		}
	}
	else if( m_pPanel->GetEditMode() == VERTEXEDIT_TRIANGLE )
	{
		for(int i = 0; i < m_selectedTriangles.selectedSurfaces.numElem(); i++)
		{
			RenderTriangleSelection( pViewRender, &m_selectedTriangles.selectedSurfaces[i] );
		}
	}

	CSelectionBaseTool::Render3D(pViewRender);
}

void CVertexTool::Render2D(CEditorViewRender* pViewRender)
{
	CSelectionBaseTool::Render2D(pViewRender);
}

void CVertexTool::DrawSelectionBox(CEditorViewRender* pViewRender)
{
	CSelectionBaseTool::DrawSelectionBox(pViewRender);
}

struct selectiondata_t
{
	vertexSelectionData_t*		pSelectionData;
	triangleSelectionData_t*	pTriangleData;
	BoundingBox					selectionbox;
};

bool SelectTriangles(CBaseEditableObject* selection, void* userdata)
{
	selectiondata_t data = *(selectiondata_t*)userdata;

	if(selection->GetType() <= EDITABLE_TERRAINPATCH)
	{
		CEditableSurface* pSurface = (CEditableSurface*)selection;

		bool bAdded = false;
		int added_id = -1;

		// check already selected
		for(int i = 0; i < data.pTriangleData->selectedSurfaces.numElem(); i++)
		{
			if(data.pTriangleData->selectedSurfaces[i].pSurface == pSurface)
			{
				added_id = i;
				bAdded = true;
				break;
			}
		}

		surfaceTriangleSelectionData_t selectiondata;
		selectiondata.pSurface = pSurface;

		for(int i = 0; i < pSurface->GetIndexCount()/3; i++)
		{
			for(int j = 0; j < 3; j++)
			{
				int idx = pSurface->GetIndices()[i*3 + j];

				if(data.selectionbox.Contains(pSurface->GetVertices()[idx].position))
				{
					if(!bAdded)
					{
						added_id = data.pTriangleData->selectedSurfaces.append( selectiondata );
						bAdded = true;
					}

					data.pTriangleData->selectedSurfaces[added_id].triangle_ids.addUnique( i );

					break;
				}
			}
		}
	}

	return false;
}

bool SelectVertices(CBaseEditableObject* selection, void* userdata)
{
	selectiondata_t data = *(selectiondata_t*)userdata;

	if(selection->GetType() == EDITABLE_BRUSH)
	{
		bool bAdded = false;
		int added_id = -1;

		CEditableBrush* pBrush = (CEditableBrush*)selection;

		// already selected
		for(int i = 0; i < data.pSelectionData->selectedBrushes.numElem(); i++)
		{
			if(data.pSelectionData->selectedBrushes[i].pBrush == pBrush)
			{
				bAdded = true;
				added_id = i;
				break;
			}
		}

		for(int i = 0; i < pBrush->GetFaceCount(); i++)
		{
			brushpolyselected_t poly;
			poly.face_id = i;

			for(int j = 0; j < pBrush->GetFacePolygon(i)->vertices.numElem(); j++)
			{
				if(data.selectionbox.Contains(pBrush->GetFacePolygon(i)->vertices[j].position))
				{
					if(!bAdded)
					{
						brushVertexSelectionData_t selectiondata;
						selectiondata.pBrush = pBrush;

						added_id = data.pSelectionData->selectedBrushes.append(selectiondata);
						bAdded = true;
					}

					poly.vertex_ids.addUnique(j);
				}
			}

			if(poly.vertex_ids.numElem() > 0)
			{
				bool found = false;
				for(int j = 0; j < data.pSelectionData->selectedBrushes[added_id].selected_polys.numElem(); j++)
				{
					if(data.pSelectionData->selectedBrushes[added_id].selected_polys[j].face_id == i)
					{
						data.pSelectionData->selectedBrushes[added_id].selected_polys[j].vertex_ids.append(poly.vertex_ids);
						found = true;
					}
				}

				// continue if not found
				if(!found)
					data.pSelectionData->selectedBrushes[added_id].selected_polys.append(poly);
			}
		}
	}
	else if(selection->GetType() <= EDITABLE_TERRAINPATCH)
	{
		CEditableSurface* pSurface = (CEditableSurface*)selection;

		bool bAdded = false;
		int added_id = -1;

		// check already selected
		for(int i = 0; i < data.pSelectionData->selectedSurfaces.numElem(); i++)
		{
			if(data.pSelectionData->selectedSurfaces[i].pSurface == pSurface)
			{
				added_id = i;
				bAdded = true;
				break;
			}
		}


		surfaceSelectionData_t selectiondata;
		selectiondata.pSurface = pSurface;

		for(int i = 0; i < pSurface->GetIndexCount(); i++)
		{
			int idx = pSurface->GetIndices()[i];
			if(data.selectionbox.Contains(pSurface->GetVertices()[idx].position))
			{
				if(!bAdded)
				{
					added_id = data.pSelectionData->selectedSurfaces.append(selectiondata);
					bAdded = true;
				}

				data.pSelectionData->selectedSurfaces[added_id].vertex_ids.addUnique(idx);
			}
		}
	}

	return false;
}


struct raySelectionData_t
{
	vertexSelectionData_t*			pSelectionData;
	triangleSelectionData_t*		pTriangleSelData;

	Vector3D				start;
	Vector3D				end;
};

bool SelectTrianglesRay(CBaseEditableObject* selection, void* userdata)
{
	raySelectionData_t data = *(raySelectionData_t*)userdata;

	int nSelectedTriangle = -1;

	if(selection->GetType() <= EDITABLE_TERRAINPATCH)
	{
		CEditableSurface* pSurface = (CEditableSurface*)selection;

		bool bAdded = false;
		int added_id = -1;

		// check already selected
		for(int i = 0; i < data.pTriangleSelData->selectedSurfaces.numElem(); i++)
		{
			if(data.pTriangleSelData->selectedSurfaces[i].pSurface == pSurface)
			{
				added_id = i;
				bAdded = true;
				break;
			}
		}

		int nNearestIDXIDX = -1;

		surfaceTriangleSelectionData_t selectiondata;
		selectiondata.pSurface = pSurface;

		nSelectedTriangle = pSurface->GetInstersectingTriangle( data.start, data.end );

		if(!bAdded && nSelectedTriangle != -1)
		{
			added_id = data.pTriangleSelData->selectedSurfaces.append( selectiondata );
			bAdded = true;
		}

		if(nSelectedTriangle != -1)
			data.pTriangleSelData->selectedSurfaces[added_id].triangle_ids.addUnique( nSelectedTriangle );
	}

	return false;
}

bool SelectVerticesRay(CBaseEditableObject* selection, void* userdata)
{
	raySelectionData_t data = *(raySelectionData_t*)userdata;

	//float fNearestToRayPoint = MAX_COORD_UNITS;

	int nNearestVertex = -1;
	float fNearestDist = MAX_COORD_UNITS;

	if(selection->GetType() == EDITABLE_BRUSH)
	{
		bool bAdded = false;
		int added_id = -1;

		CEditableBrush* pBrush = (CEditableBrush*)selection;

		// already selected
		for(int i = 0; i < data.pSelectionData->selectedBrushes.numElem(); i++)
		{
			if(data.pSelectionData->selectedBrushes[i].pBrush == pBrush)
			{
				bAdded = true;
				added_id = i;
				break;
			}
		}

		for(int i = 0; i < pBrush->GetFaceCount(); i++)
		{
			brushpolyselected_t poly;
			poly.face_id = i;

			for(int j = 0; j < pBrush->GetFacePolygon(i)->vertices.numElem(); j++)
			{
				Vector3D position = pBrush->GetFacePolygon(i)->vertices[j].position;
				float fProj = lineProjection(data.start, data.end, position);

				Vector3D interpPos = lerp(data.start, data.end, fProj);

				float nPointToProjDist = length(interpPos-position);

			//	float fRayToPointDist = length(interpPos-data.start);

				if(fProj > 0.0f && fProj < 1.0f && nPointToProjDist < fNearestDist)
				{
					//fNearestToRayPoint = fRayToPointDist;
					fNearestDist = nPointToProjDist;
					nNearestVertex = j;
				}
			}

			if(!bAdded && nNearestVertex != -1)
			{
				brushVertexSelectionData_t selectiondata;
				selectiondata.pBrush = pBrush;

				added_id = data.pSelectionData->selectedBrushes.append(selectiondata);
				bAdded = true;
			}

			if(nNearestVertex != -1)
				poly.vertex_ids.addUnique(nNearestVertex);

			if(poly.vertex_ids.numElem() > 0)
			{
				bool found = false;
				for(int j = 0; j < data.pSelectionData->selectedBrushes[added_id].selected_polys.numElem(); j++)
				{
					if(data.pSelectionData->selectedBrushes[added_id].selected_polys[j].face_id == i)
					{
						data.pSelectionData->selectedBrushes[added_id].selected_polys[j].vertex_ids.append(poly.vertex_ids);
						found = true;
					}
				}

				// continue if not found
				if(!found)
					data.pSelectionData->selectedBrushes[added_id].selected_polys.append(poly);
			}
		}
	}
	else if(selection->GetType() <= EDITABLE_TERRAINPATCH)
	{
		CEditableSurface* pSurface = (CEditableSurface*)selection;

		bool bAdded = false;
		int added_id = -1;

		// check already selected
		for(int i = 0; i < data.pSelectionData->selectedSurfaces.numElem(); i++)
		{
			if(data.pSelectionData->selectedSurfaces[i].pSurface == pSurface)
			{
				added_id = i;
				bAdded = true;
				break;
			}
		}

		int nNearestIDXIDX = -1;

		surfaceSelectionData_t selectiondata;
		selectiondata.pSurface = pSurface;

		for(int i = 0; i < pSurface->GetIndexCount(); i++)
		{
			int idx = pSurface->GetIndices()[i];

			Vector3D position = pSurface->GetVertices()[idx].position;
			float fProj = lineProjection(data.start, data.end, position);

			Vector3D interpPos = lerp(data.start, data.end, fProj);

			float nPointToProjDist = length(interpPos-position);

			//float fRayToPointDist = length(interpPos-data.start);

			if(fProj > 0.0f && fProj < 1.0f && nPointToProjDist < fNearestDist)
			{
				//fNearestToRayPoint = fRayToPointDist;
				fNearestDist = nPointToProjDist;
				nNearestVertex = idx;
				nNearestIDXIDX = i;
			}

			/*
			if(data.selectionbox.Contains(pSurface->GetVertices()[i].position))
			{
				if(!bAdded)
				{
					added_id = data.pSelectionData->selectedSurfaces.append(selectiondata);
					bAdded = true;
				}

				data.pSelectionData->selectedSurfaces[added_id].vertex_ids.append(i);
			}
			*/
		}

		if(!bAdded && nNearestVertex != -1)
		{
			added_id = data.pSelectionData->selectedSurfaces.append(selectiondata);
			bAdded = true;
		}

		if(nNearestVertex != -1)
		{
			data.pSelectionData->selectedSurfaces[added_id].vertex_ids.addUnique(nNearestVertex);
		}
	}

	return false;
}


void CVertexTool::UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
{
	if(m_pPanel->GetEditMode() != VERTEXEDIT_VERTEX)
	{
		//if(mouseEvents.ControlDown() || GetMouseOverSelectionHandle() != SH_NONE)
			CSelectionBaseTool::UpdateManipulation2D(pViewRender, mouseEvents, delta3D, delta2D);

		UpdateSelectionCenter();

		UpdateAllWindows();

		return;
	}

	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	Vector2D screenDims = pViewRender->Get2DDimensions();
	// get mouse cursor position
	Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

	Vector3D ray_start, ray_dir;
	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, ray_start, ray_dir, proj*view, true);

	if(GetMouseOverSelectionHandle() == SH_NONE)
	{
		if(m_vertexselection.selectedBrushes.numElem() == 0 && m_vertexselection.selectedSurfaces.numElem() == 0)
		{
			if(mouseEvents.Button(wxMOUSE_BTN_LEFT) && mouseEvents.ButtonDown())
			{
				// apply current verts
				CancelSelection();
				ApplyVertexManipulation();
				
				// select vertex
				selectiondata_t data;
				data.pSelectionData = &m_vertexselection;

				Vector2D screenDims = pViewRender->Get2DDimensions();
				float fInvDistance = pViewRender->GetView()->GetFOV() / screenDims.y;

				data.selectionbox.minPoint = (ray_start-Vector3D(5*fInvDistance))*view.rows[0].xyz() + (ray_start-Vector3D(5*fInvDistance))*view.rows[1].xyz();
				data.selectionbox.maxPoint = (ray_start+Vector3D(5*fInvDistance))*view.rows[0].xyz() + (ray_start+Vector3D(5*fInvDistance))*view.rows[1].xyz();
			
				data.selectionbox.Fix();

				if(data.selectionbox.minPoint.x == data.selectionbox.maxPoint.x)
				{
					data.selectionbox.minPoint.x = -MAX_COORD_UNITS;
					data.selectionbox.maxPoint.x = MAX_COORD_UNITS;
				}

				if(data.selectionbox.minPoint.y == data.selectionbox.maxPoint.y)
				{
					data.selectionbox.minPoint.y = -MAX_COORD_UNITS;
					data.selectionbox.maxPoint.y = MAX_COORD_UNITS;
				}

				if(data.selectionbox.minPoint.z == data.selectionbox.maxPoint.z)
				{
					data.selectionbox.minPoint.z = -MAX_COORD_UNITS;
					data.selectionbox.maxPoint.z = MAX_COORD_UNITS;
				}

				((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(SelectVertices, &data,false);
				
			}
		}
		else
		{
			if(mouseEvents.Button(wxMOUSE_BTN_LEFT) && mouseEvents.AltDown() && !mouseEvents.ShiftDown() && !mouseEvents.Dragging())
			{
				if(mouseEvents.ButtonDown())
				{
					Vector3D mouse_vec = normalize(m_selectioncenter - ray_start);
					Vector3D rot_direction = sign(view.rows[0].xyz())*view.rows[0].xyz()*mouse_vec + sign(view.rows[1].xyz())*view.rows[1].xyz()*mouse_vec;

					Vector3D vUp = cross(view.rows[2].xyz(), mouse_vec);

					m_beginupvec = vUp;
					m_beginrightvec = rot_direction;
				}
				else
				{
					ApplyVertexManipulation(false);
				}
			}

			if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
			{
				if(mouseEvents.Dragging() && !mouseEvents.ShiftDown() && !mouseEvents.ControlDown())
				{
					if(mouseEvents.AltDown())
					{
						Vector3D mouse_vec = normalize(m_selectioncenter - ray_start);

						Vector3D rot_direction = sign(view.rows[0].xyz())*view.rows[0].xyz()*mouse_vec + sign(view.rows[1].xyz())*view.rows[1].xyz()*mouse_vec;

						Vector3D vUp = cross(view.rows[2].xyz(), mouse_vec);

						Matrix3x3 mat1(rot_direction, vUp, view.rows[2].xyz());
						Matrix3x3 mat2(m_beginrightvec, m_beginupvec, view.rows[2].xyz());

						Matrix3x3 rotation = !mat2*mat1;

						Vector3D eulers = EulerMatrixXYZ(rotation);
						Vector3D angles = -VRAD2DEG(eulers);

						m_rotation = angles;
					}
					else
						m_selectionoffset += delta3D;
				}
			}

			if(mouseEvents.ButtonUp(wxMOUSE_BTN_LEFT))
			{
				ApplyVertexManipulation(false, true);
				UpdateAllWindows();
			}
		}
	}

	if(m_vertexselection.selectedBrushes.numElem() == 0 && m_vertexselection.selectedSurfaces.numElem() == 0 || mouseEvents.ControlDown() || GetMouseOverSelectionHandle() != SH_NONE)
		CSelectionBaseTool::UpdateManipulation2D(pViewRender, mouseEvents, delta3D, delta2D);

	UpdateSelectionCenter();

	UpdateAllWindows();
}

// 3D manipulation for clipper is unsupported
void CVertexTool::UpdateManipulation3D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D)
{
	Matrix4x4 view, proj;
	pViewRender->GetViewProjection(view,proj);

	Vector2D screenDims = pViewRender->Get2DDimensions();

	// get mouse cursor position
	Vector2D mouse_position(mouseEvents.GetX(),mouseEvents.GetY());

	Vector3D forward = view.rows[2].xyz();

	// setup edit plane
	Plane pl(forward.x,forward.y,forward.z, -dot(forward, m_selectioncenter));

	Vector3D ray_start, ray_dir;

	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-(mouse_position.x-delta2D.x),mouse_position.y-delta2D.y), screenDims, ray_start, ray_dir, proj*view, false);

	Vector3D curr_ray_start, curr_ray_dir;

	// get ray direction (why we've not still fixing the 'screenDims.x-mouse_position.x'?)
	ScreenToDirection(pViewRender->GetAbsoluteCameraPosition(), Vector2D(screenDims.x-mouse_position.x,mouse_position.y), screenDims, curr_ray_start, curr_ray_dir, proj*view, false);

	Vector3D intersect1, intersect2;
	if(	!pl.GetIntersectionWithRay(curr_ray_start, normalize(curr_ray_dir),intersect1) ||
		!pl.GetIntersectionWithRay(ray_start, normalize(ray_dir),intersect2))
	{
		intersect1 = vec3_zero;
		intersect2 = delta3D;
	}

	Vector3D projected_move_delta = intersect1-intersect2;

	if(mouseEvents.Dragging() && mouseEvents.ButtonIsDown(wxMOUSE_BTN_MIDDLE) && !mouseEvents.ShiftDown() && !mouseEvents.ControlDown())
	{
		MoveSelectionTexCoord(delta2D*0.5f);
	}
	if(!mouseEvents.ShiftDown())
	{
		if(mouseEvents.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			if(mouseEvents.ControlDown())
			{
				raySelectionData_t data;
				data.pSelectionData = &m_vertexselection;
				data.pTriangleSelData = &m_selectedTriangles;
				data.start = curr_ray_start;
				data.end = curr_ray_start+curr_ray_dir;

				if(mouseEvents.ControlDown())
					ApplyVertexManipulation(false);

				if(m_pPanel->GetEditMode() == VERTEXEDIT_VERTEX)
					((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects( SelectVerticesRay, &data, false );
				else
					((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects( SelectTrianglesRay, &data, false );

				CancelSelection();

				UpdateSelectionCenter();
			}
			else if(mouseEvents.Dragging())
			{
				m_selectionoffset += projected_move_delta;
			}
		}

		if(mouseEvents.ButtonUp(wxMOUSE_BTN_LEFT))
		{
			ApplyVertexManipulation(false, true);
			UpdateAllWindows();
		}

		if(mouseEvents.ButtonDown(wxMOUSE_BTN_RIGHT))
		{
			Vector3D hit_pos;
			CBaseEditableObject* pObject = g_pLevel->GetRayNearestObject(ray_start, ray_start+ray_dir, hit_pos, NR_FLAG_BRUSHES | NR_FLAG_SURFACEMODS | NR_FLAG_INCLUDEGROUPPED);

			if(pObject)
			{
				AddVertexToCurrentEdge(hit_pos);
				UpdateAllWindows();
			}
		}
	}
}

void CVertexTool::MoveSelectionTexCoord(Vector2D &delta)
{
	for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
	{
		IMaterial* pMaterial = m_vertexselection.selectedSurfaces[i].pSurface->GetMaterial();
		ITexture* pMaterialTex = pMaterial->GetBaseTexture();

		int texWide = 1;
		int texTall = 1;

		if(pMaterialTex)
		{
			texWide = pMaterialTex->GetWidth();
			texTall = pMaterialTex->GetHeight();
		}

		m_vertexselection.selectedSurfaces[i].pSurface->BeginModify();
		for(int j = 0; j < m_vertexselection.selectedSurfaces[i].vertex_ids.numElem(); j++)
		{
			int vID = m_vertexselection.selectedSurfaces[i].vertex_ids[j];

			Vector2D vDelta(delta.x/texWide, delta.y/texTall);

			m_vertexselection.selectedSurfaces[i].pSurface->GetVertices()[vID].texcoord += vDelta;
		}
		m_vertexselection.selectedSurfaces[i].pSurface->EndModify();
	}
}

void CVertexTool::MoveSelectedVerts(Vector3D &delta, bool bApplyBrushes)
{
	if(bApplyBrushes)
	{
		for(int i = 0; i < m_vertexselection.selectedBrushes.numElem(); i++)
		{
			for(int j = 0; j < m_vertexselection.selectedBrushes[i].selected_polys.numElem(); j++)
			{
				int face_id = m_vertexselection.selectedBrushes[i].selected_polys[j].face_id;

				for(int k = 0; k < m_vertexselection.selectedBrushes[i].selected_polys[j].vertex_ids.numElem(); k++)
				{
					int v_id = m_vertexselection.selectedBrushes[i].selected_polys[j].vertex_ids[k];

					Vector3D vertex = m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(face_id)->vertices[v_id].position;

					Matrix3x3 rotMatrix = rotateXYZ3(DEG2RAD(m_rotation.x),DEG2RAD(m_rotation.y),DEG2RAD(m_rotation.z));
					vertex = m_selectioncenter + rotMatrix*(vertex-m_selectioncenter);
					vertex += delta;

					m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(face_id)->vertices[v_id].position = vertex;
				}
			}
		}
	}

	for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
	{
		for(int j = 0; j < m_vertexselection.selectedSurfaces[i].vertex_ids.numElem(); j++)
		{
			int vID = m_vertexselection.selectedSurfaces[i].vertex_ids[j];

			Vector3D vertex = m_vertexselection.selectedSurfaces[i].pSurface->GetVertices()[vID].position;

			Matrix3x3 rotMatrix = rotateXYZ3(DEG2RAD(m_rotation.x),DEG2RAD(m_rotation.y),DEG2RAD(m_rotation.z));
			vertex = m_selectioncenter + rotMatrix*(vertex-m_selectioncenter);
			vertex += delta;

			m_vertexselection.selectedSurfaces[i].pSurface->GetVertices()[vID].position = vertex;
		}
	}

	UpdateSelectionCenter();
}

void CVertexTool::UpdateSelectionCenter()
{
	BoundingBox bbox;
	bbox.Reset();

	for(int i = 0; i < m_vertexselection.selectedBrushes.numElem(); i++)
	{
		for(int j = 0; j < m_vertexselection.selectedBrushes[i].selected_polys.numElem(); j++)
		{
			int face_id = m_vertexselection.selectedBrushes[i].selected_polys[j].face_id;

			for(int k = 0; k < m_vertexselection.selectedBrushes[i].selected_polys[j].vertex_ids.numElem(); k++)
			{
				int v_id = m_vertexselection.selectedBrushes[i].selected_polys[j].vertex_ids[k];
				bbox.AddVertex(m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(face_id)->vertices[v_id].position);
			}
		}
	}

	for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
	{
		for(int j = 0; j < m_vertexselection.selectedSurfaces[i].vertex_ids.numElem(); j++)
		{
			int vID = m_vertexselection.selectedSurfaces[i].vertex_ids[j];
			bbox.AddVertex(m_vertexselection.selectedSurfaces[i].pSurface->GetVertices()[vID].position);
		}
	}

	m_selectioncenter = bbox.GetCenter()+m_selectionoffset;
}

void CVertexTool::SnapSelectedVertsToGrid()
{
	for(int i = 0; i < m_vertexselection.selectedBrushes.numElem(); i++)
	{
		for(int j = 0; j < m_vertexselection.selectedBrushes[i].selected_polys.numElem(); j++)
		{
			int face_id = m_vertexselection.selectedBrushes[i].selected_polys[j].face_id;

			for(int k = 0; k < m_vertexselection.selectedBrushes[i].selected_polys[j].vertex_ids.numElem(); k++)
			{
				int v_id = m_vertexselection.selectedBrushes[i].selected_polys[j].vertex_ids[k];
				SnapVector3D(m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(face_id)->vertices[v_id].position);
			}
		}
	}

	for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
	{
		for(int j = 0; j < m_vertexselection.selectedSurfaces[i].vertex_ids.numElem(); j++)
		{
			int vID = m_vertexselection.selectedSurfaces[i].vertex_ids[j];
			SnapVector3D(m_vertexselection.selectedSurfaces[i].pSurface->GetVertices()[vID].position);
		}
	}

	UpdateAllWindows();
}

/*
	DkList<int> extruded;
	DkList<int> extrudedidx;

	int nIdxs = verts.numElem();

	
	//for(int i = 0; i < edges.numElem(); i++)
	//{
	//	int i0 = edges[i].idx[0];
	//	int i1 = edges[i].idx[1];

	//	int idx0 = extrudedidx.findIndex(i0);
	//	int idx1 = extrudedidx.findIndex(i1);

	//	if(idx0 == -1)
	//	{
	//		eqlevelvertex_t v(verts[i0]);
	//		idx0 = verts.append(v);
	//		extruded.append(idx0);
	//		extrudedidx.append(i0);
	//	}
	//	else
	//		idx0 = extruded[idx0];

	//	if(idx1 == -1)
	//	{
	//		eqlevelvertex_t v(verts[i1]);
	//		idx1 = verts.append(v);
	//		extruded.append(idx1);
	//		extrudedidx.append(i1);
	//	}
	//	else
	//		idx1 = extruded[idx1];

	//	new_indices.append(i1);
	//	new_indices.append(i0);
	//	new_indices.append(idx0);

	//	new_indices.append(idx0);
	//	new_indices.append(idx1);
	//	new_indices.append(i1);
	//}
	

	for(int i = 0; i < edges.numElem(); i++)
	{
		int i0 = edges[i].idx[0];
		int i1 = edges[i].idx[1];

		eqlevelvertex_t v(verts[i0]);
		int idx0 = verts.append(v);

		eqlevelvertex_t v2(verts[i1]);
		int idx1 = verts.append(v2);

		new_indices.append(i1);
		new_indices.append(i0);
		new_indices.append(idx0);

		new_indices.append(idx0);
		new_indices.append(idx1);
		new_indices.append(i1);
	}
*/

void ExtrudeEdgesOfSurface(DkList<medge_t>& edges, CEditableSurface* pSurface, surfaceSelectionData_t* pData)
{
	DkList<eqlevelvertex_t>		verts;
	DkList<int>					indices;

	verts.resize(pSurface->GetVertexCount());
	indices.resize(pSurface->GetIndexCount());

	for(int i = 0; i < pSurface->GetVertexCount(); i++)
		verts.append(pSurface->GetVertices()[i]);

	for(int i = 0; i < pSurface->GetIndexCount(); i++)
		indices.append(pSurface->GetIndices()[i]);

	
	DkList<eqlevelvertex_t> new_verts;
	DkList<int>				new_indices;

	// clear selection now!
	pData->vertex_ids.clear();

	DkList<int> extruded;
	DkList<int> extrudedidx;

	int nIdxs = verts.numElem();
	
	for(int i = 0; i < edges.numElem(); i++)
	{
		int i0 = edges[i].idx[0];
		int i1 = edges[i].idx[1];

		int idx0 = extrudedidx.findIndex(i0);
		int idx1 = extrudedidx.findIndex(i1);

		if(idx0 == -1)
		{
			eqlevelvertex_t v(verts[i0]);
			idx0 = verts.append(v);
			extruded.append(idx0);
			extrudedidx.append(i0);
		}
		else
			idx0 = extruded[idx0];

		if(idx1 == -1)
		{
			eqlevelvertex_t v(verts[i1]);
			idx1 = verts.append(v);
			extruded.append(idx1);
			extrudedidx.append(i1);
		}
		else
			idx1 = extruded[idx1];

		new_indices.append(i1);
		new_indices.append(i0);
		new_indices.append(idx0);

		new_indices.append(idx0);
		new_indices.append(idx1);
		new_indices.append(i1);

		pData->vertex_ids.addUnique(idx0);
		pData->vertex_ids.addUnique(idx1);
	}
	

	/*
	for(int i = 0; i < nEdges; i++)
	{
		int i0 = edges[i].idx[0];
		int i1 = edges[i].idx[1];

		eqlevelvertex_t v(verts[i0]);
		int idx0 = verts.append(v);

		eqlevelvertex_t v2(verts[i1]);
		int idx1 = verts.append(v2);

		new_indices.append(i1);
		new_indices.append(i0);
		new_indices.append(idx0);

		new_indices.append(idx0);
		new_indices.append(idx1);
		new_indices.append(i1);
	}
	*/

	indices.append(new_indices);

	IMaterial* pMat = pSurface->GetMaterial();

	pSurface->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),indices.numElem());
	pSurface->SetMaterial(pMat);
	
}

void CVertexTool::ExtrudeEdges()
{
	for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
	{
		DkList<medge_t> edges;

		CAdjacentTriangleGraph triGraph;
		triGraph.Build(	m_vertexselection.selectedSurfaces[i].pSurface->GetIndices(),
						m_vertexselection.selectedSurfaces[i].pSurface->GetIndexCount());

		DkList<mtriangle_t>* triangles = triGraph.GetTriangles();

		// rearrange it first
		//m_vertexselection.selectedSurfaces[i].vertex_ids.sort( int_sort_compare );

		int num_edges = m_vertexselection.selectedSurfaces[i].vertex_ids.numElem() / 2;
		if(num_edges == 0)
			continue;
		
		int nEdges = 0;

		for(int j = 0; j < triangles->numElem(); j++)
		{
			for(int k = 0; k < 3; k++)
			{
				int v_idx1 = m_vertexselection.selectedSurfaces[i].vertex_ids.findIndex(triangles->ptr()[j].edges[k].idx[0]);
				int v_idx2 = m_vertexselection.selectedSurfaces[i].vertex_ids.findIndex(triangles->ptr()[j].edges[k].idx[1]);

				// display free edges
				if(!triangles->ptr()[j].edge_connections[k] && v_idx1 != -1 && v_idx2 != -1)
				{
					edges.append(triangles->ptr()[j].edges[k]);
				}
			}
		}
		
		//for(int j = 0; j < edges.numElem(); j++)
		ExtrudeEdgesOfSurface(edges, m_vertexselection.selectedSurfaces[i].pSurface, &m_vertexselection.selectedSurfaces[i]);
	}
}

void CVertexTool::OnDeactivate()
{
	CancelSelection();
	ApplyVertexManipulation();
}

// applies vertex manipulations
void CVertexTool::ApplyVertexManipulation(bool clear, bool bApplyBrushes)
{
	// Create undo
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

	Vector3D snapped_move = m_selectionoffset;
	SnapVector3D(snapped_move);

	MoveSelectedVerts(snapped_move, bApplyBrushes);

	m_selectionoffset = vec3_zero;
	m_rotation = vec3_zero;

	if(bApplyBrushes)
	{
		for(int i = 0; i < m_vertexselection.selectedBrushes.numElem(); i++)
		{
			for(int j = 0; j < m_vertexselection.selectedBrushes[i].pBrush->GetFaceCount(); j++)
			{
				Vector3D v0,v1,v2;
				int nVerts = m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(j)->vertices.numElem();

				v0 = SnapVector(1, m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(j)->vertices[0].position);
				v1 = SnapVector(1, m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(j)->vertices[1].position);
				v2 = SnapVector(1, m_vertexselection.selectedBrushes[i].pBrush->GetFacePolygon(j)->vertices[2].position);

				m_vertexselection.selectedBrushes[i].pBrush->GetFace(j)->Plane = Plane(v0,v1,v2);
			}
		}

		for(int i = 0; i < m_vertexselection.selectedBrushes.numElem(); i++)
		{
			m_vertexselection.selectedBrushes[i].pBrush->CreateFromPlanes();
			m_vertexselection.selectedBrushes[i].pBrush->CalculateBBOX();

			m_vertexselection.selectedBrushes[i].pBrush->UpdateRenderBuffer();
		}
	}

	for(int i = 0; i < m_vertexselection.selectedSurfaces.numElem(); i++)
	{
		m_vertexselection.selectedSurfaces[i].pSurface->BeginModify();
		m_vertexselection.selectedSurfaces[i].pSurface->EndModify();
	}

	if(clear)
	{
		m_vertexselection.selectedBrushes.clear();
		m_vertexselection.selectedSurfaces.clear();
	}
}

void CVertexTool::AddVertexToCurrentEdge(Vector3D &point)
{
	// check if we have single surface selected
	if(m_vertexselection.selectedSurfaces.numElem() > 1)
	{
		MsgWarning("You cannot add vertex when multiple objects are selected\n");
		return;
	}

	if(!m_vertexselection.selectedSurfaces.numElem())
		return;

	CEditableSurface* pSurf = m_vertexselection.selectedSurfaces[0].pSurface;

	if(m_vertexselection.selectedSurfaces[0].vertex_ids.numElem() != 2)
	{
		MsgWarning("You need to select only two neighbouring verts\n");
		return;
	}

	DkList<eqlevelvertex_t>		verts;
	DkList<int>					indices;

	verts.resize(pSurf->GetVertexCount());
	indices.resize(pSurf->GetIndexCount());

	for(int i = 0; i < pSurf->GetVertexCount(); i++)
		verts.append(pSurf->GetVertices()[i]);
	for(int i = 0; i < pSurf->GetIndexCount(); i++)
		indices.append(pSurf->GetIndices()[i]);

	Vector3D edgePos = (m_vertexselection.selectedSurfaces[0].vertex_ids[0] + m_vertexselection.selectedSurfaces[0].vertex_ids[1]) * 0.5f;
	Vector3D edge = m_vertexselection.selectedSurfaces[0].vertex_ids[0] - m_vertexselection.selectedSurfaces[0].vertex_ids[1];

	Vector3D vertEdgePos = point - edgePos;

	eqlevelvertex_t new_vert( point, Vector2D(dot(edge, vertEdgePos)*0.005f,dot(vertEdgePos,vertEdgePos)*0.005f), pSurf->GetSurfaceTexture(0)->Plane.normal );

	int idx0 = m_vertexselection.selectedSurfaces[0].vertex_ids[0];
	int idx1 = m_vertexselection.selectedSurfaces[0].vertex_ids[1];
	int idx2 = verts.append(new_vert);

	indices.append(idx0);
	indices.append(idx1);
	indices.append(idx2);

	/*
	if(idx0 > idx2)
	{
		int temp = idx2;
		idx2 = idx0;
		idx0 = temp;
	}
	*/

	IMaterial* pMat = pSurf->GetMaterial();
	pSurf->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),indices.numElem());
	pSurf->SetMaterial(pMat);

	m_vertexselection.selectedSurfaces[0].vertex_ids[0] = idx2;

	UpdateSelectionCenter();

	// Create undo
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();
}

void CVertexTool::AddTriangle()
{
	// check if we have single surface selected
	if(m_vertexselection.selectedSurfaces.numElem() > 1)
	{
		MsgWarning("You cannot add vertex when multiple objects are selected\n");
		return;
	}

	if(!m_vertexselection.selectedSurfaces.numElem())
		return;

	CEditableSurface* pSurf = m_vertexselection.selectedSurfaces[0].pSurface;

	if(m_vertexselection.selectedSurfaces[0].vertex_ids.numElem() != 3)
	{
		MsgWarning("You need to select only three verts\n");
		return;
	}

	DkList<eqlevelvertex_t>		verts;
	DkList<int>					indices;

	verts.resize(pSurf->GetVertexCount());
	indices.resize(pSurf->GetIndexCount());

	for(int i = 0; i < pSurf->GetVertexCount(); i++)
		verts.append(pSurf->GetVertices()[i]);
	for(int i = 0; i < pSurf->GetIndexCount(); i++)
		indices.append(pSurf->GetIndices()[i]);

	int idx0 = m_vertexselection.selectedSurfaces[0].vertex_ids[0];
	int idx1 = m_vertexselection.selectedSurfaces[0].vertex_ids[1];
	int idx2 = m_vertexselection.selectedSurfaces[0].vertex_ids[2];

	indices.append(idx0);
	indices.append(idx1);
	indices.append(idx2);

	IMaterial* pMat = pSurf->GetMaterial();
	pSurf->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),indices.numElem());
	pSurf->SetMaterial(pMat);

	m_vertexselection.selectedSurfaces[0].vertex_ids[0] = idx2;

	UpdateSelectionCenter();

	// Create undo
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();
}

int fnComp(const void* idx1, const void* idx2)
{
	int n1 = *(int*)idx1;
	int n2 = *(int*)idx2;

	return n1 - n2;
}

int ComputeTriangleIndexByIndices(int *ids, int* indices, int nIndices)
{
	int nTriangle = 0;
	int nTris = nIndices / 3;

	// TODO: need a rolling function that checks this triangle

	for(int i = 0; i < nTris; i++)
	{
		// checks for next two ids
		if(indices[i*3] == ids[0])
		{
			if(indices[i*3+1] == ids[1])
			{
				if(indices[i*3+2] == ids[2])
					return nTriangle;
			}
			else if(indices[i*3+1] == ids[2])
			{
				if(indices[i*3+2] == ids[1])
					return nTriangle;
			}
		}
		else if(indices[i*3] == ids[1])
		{
			if(indices[i*3+1] == ids[0])
			{
				if(indices[i*3+2] == ids[2])
					return nTriangle;
			}
			else if(indices[i*3+1] == ids[2])
			{
				if(indices[i*3+2] == ids[0])
					return nTriangle;
			}
		}
		else if(indices[i*3] == ids[2])
		{
			if(indices[i*3+1] == ids[0])
			{
				if(indices[i*3+2] == ids[1])
					return nTriangle;
			}
			else if(indices[i*3+1] == ids[1])
			{
				if(indices[i*3+2] == ids[0])
					return nTriangle;
			}
		}

		nTriangle++;
	}

	return -1;
}

void CVertexTool::FlipQuads()
{
	if(m_pPanel->GetEditMode() == VERTEXEDIT_TRIANGLE)
	{
		for(int i = 0; i < m_selectedTriangles.selectedSurfaces.numElem(); i++)
		{
			CEditableSurface* pSurf = m_selectedTriangles.selectedSurfaces[i].pSurface;

			int nTriangles = m_selectedTriangles.selectedSurfaces[i].triangle_ids.numElem();

			if(nTriangles != 2)
				continue;

			int tri_id[2];

			if(m_selectedTriangles.selectedSurfaces[i].triangle_ids[0] > m_selectedTriangles.selectedSurfaces[i].triangle_ids[1])
			{
				tri_id[1] = m_selectedTriangles.selectedSurfaces[i].triangle_ids[0];
				tri_id[0] = m_selectedTriangles.selectedSurfaces[i].triangle_ids[1];
			}
			else
			{
				tri_id[0] = m_selectedTriangles.selectedSurfaces[i].triangle_ids[0];
				tri_id[1] = m_selectedTriangles.selectedSurfaces[i].triangle_ids[1];
			}

			DkList<eqlevelvertex_t>		verts;
			DkList<int>					indices;

			verts.resize(pSurf->GetVertexCount());
			indices.resize(pSurf->GetIndexCount());

			for(int j = 0; j < pSurf->GetVertexCount(); j++)
				verts.append(pSurf->GetVertices()[j]);

			// first, detect the triangles we want to flip
			for(int j = 0; j < pSurf->GetIndexCount(); j++)
				indices.append(pSurf->GetIndices()[j]);

			// find the shared edge
			int tri_1[3] = {indices[tri_id[0] * 3],
							indices[tri_id[0] * 3 + 1],
							indices[tri_id[0] * 3 + 2]};

			int tri_2[3] = {indices[tri_id[1] * 3],
							indices[tri_id[1] * 3 + 1],
							indices[tri_id[1] * 3 + 2]};

			int nSharedVerts = 0;
			int edgeIdx1[2];
			int edgeIdx2[2];

			int freeVerts[2];
			int nFreeVerts = 0;

			for(int j = 0; j < 3; j++)
			{
				for(int k = 0; k < 3; k++)
				{
					if(tri_1[j] == tri_2[k])
					{
						edgeIdx1[nSharedVerts] = tri_1[j];
						edgeIdx2[nSharedVerts] = tri_2[k];
						nSharedVerts++;
					}
				}
			}

			for(int j = 0; j < 3; j++)
			{
				if(tri_1[j] != edgeIdx1[0] && tri_1[j] != edgeIdx1[1])
				{
					freeVerts[nFreeVerts] = tri_1[j];
					nFreeVerts++;
				}
			}

			for(int j = 0; j < 3; j++)
			{
				if(tri_2[j] != edgeIdx2[0] && tri_2[j] != edgeIdx2[1])
				{
					freeVerts[nFreeVerts] = tri_2[j];
					nFreeVerts++;
				}
			}

			if(nSharedVerts == 2 && nFreeVerts == 2)
			{
				if(edgeIdx1[0] > edgeIdx1[1])
				{
					int tmp = edgeIdx1[0];
					edgeIdx1[0] = edgeIdx1[1];
					edgeIdx1[1] = tmp;
				}

				int orig_quad[4] = {freeVerts[0], edgeIdx1[0], edgeIdx1[1], freeVerts[1]};
				int quad_idx[4] = {edgeIdx1[0], freeVerts[0], freeVerts[1], edgeIdx1[1]};
				
				bool flip = true;
				//if(orig_quad[0] != tri_1[0] || orig_quad[1] != tri_1[1] || orig_quad[2] != tri_1[2])
				//	flip = false;

				if(flip)
				{
					indices[tri_id[0] * 3]   = quad_idx[0];
					indices[tri_id[0] * 3+1] = quad_idx[1];
					indices[tri_id[0] * 3+2] = quad_idx[2];
				
					indices[tri_id[1] * 3]   = quad_idx[3];
					indices[tri_id[1] * 3+1] = quad_idx[2];
					indices[tri_id[1] * 3+2] = quad_idx[1];
				}
				else
				{
					indices[tri_id[0] * 3]   = quad_idx[2];
					indices[tri_id[0] * 3+1] = quad_idx[1];
					indices[tri_id[0] * 3+2] = quad_idx[0];
				
					indices[tri_id[1] * 3]   = quad_idx[1];
					indices[tri_id[1] * 3+1] = quad_idx[2];
					indices[tri_id[1] * 3+2] = quad_idx[3];
				}

				IMaterial* pMat = pSurf->GetMaterial();
				pSurf->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),indices.numElem());
				pSurf->SetMaterial(pMat);
				
			}
		}
	}
}

// comparsion operator
bool compare_vert_nonormal(const eqlevelvertex_t &u, const eqlevelvertex_t &v)
{
	if(u.position == v.position && u.texcoord == v.texcoord)
		return true;

	return false;
}

int AddUniqueVertexToListNoNormal(eqlevelvertex_t vert, DkList<eqlevelvertex_t> &list)
{
	for(int i = 0; i < list.numElem(); i++)
	{
		if(compare_vert_nonormal( list[i], vert ))
			return i;
	}

	return list.append( vert );
}

void CVertexTool::MakeSmoothingGroup()
{
	if(m_pPanel->GetEditMode() == VERTEXEDIT_TRIANGLE)
	{
		for(int i = 0; i < m_selectedTriangles.selectedSurfaces.numElem(); i++)
		{
			CEditableSurface* pSurf = m_selectedTriangles.selectedSurfaces[i].pSurface;

			int nTriangles = m_selectedTriangles.selectedSurfaces[i].triangle_ids.numElem();

			if(nTriangles == 0)
				continue;

			DkList<eqlevelvertex_t>		vertices;
			DkList<int>					indices;

			int* vert_remap = new int[pSurf->GetVertexCount()];

			vertices.resize(pSurf->GetVertexCount());
			indices.resize(pSurf->GetIndexCount());

			for(int j = 0; j < pSurf->GetVertexCount(); j++)
				vert_remap[j] = -1;

#define ADD_REMAPPED_VERTEX(idx, vertex)		\
{												\
	if(vert_remap[ idx ] != -1)					\
		indices.append( vert_remap[ idx ] );	\
	else										\
	{											\
		int currVerts = vertices.numElem();		\
		vert_remap[idx] = currVerts;			\
		indices.append( currVerts );			\
		vertices.append( vertex );				\
	}											\
}

			int nTriangle = 0;

			// add unselected polygons
			for(int j = 0; j < pSurf->GetIndexCount(); j+=3, nTriangle++)
			{
				if(m_selectedTriangles.selectedSurfaces[i].triangle_ids.findIndex( nTriangle ) != -1)
					continue;
				
				int idx0 = pSurf->GetIndices()[j];
				int idx1 = pSurf->GetIndices()[j+1];
				int idx2 = pSurf->GetIndices()[j+2];

				ADD_REMAPPED_VERTEX(idx0, pSurf->GetVertices()[idx0])
				ADD_REMAPPED_VERTEX(idx1, pSurf->GetVertices()[idx1])
				ADD_REMAPPED_VERTEX(idx2, pSurf->GetVertices()[idx2])
			}

			DkList<eqlevelvertex_t> smVerts;
			DkList<int>				smIndices;

			smVerts.resize(pSurf->GetVertexCount());
			smIndices.resize(pSurf->GetIndexCount());

			// add selected as cool with fixup
			for(int j = 0; j < nTriangles; j++)
			{
				int triId = m_selectedTriangles.selectedSurfaces[i].triangle_ids[j];

				int idx0 = pSurf->GetIndices()[triId*3];
				int idx1 = pSurf->GetIndices()[triId*3+1];
				int idx2 = pSurf->GetIndices()[triId*3+2];

				int nIdx0 = AddUniqueVertexToListNoNormal( pSurf->GetVertices()[idx0], smVerts);
				int nIdx1 = AddUniqueVertexToListNoNormal( pSurf->GetVertices()[idx1], smVerts);
				int nIdx2 = AddUniqueVertexToListNoNormal( pSurf->GetVertices()[idx2], smVerts);

				smIndices.append(nIdx0);
				smIndices.append(nIdx1);
				smIndices.append(nIdx2);
			}

			int firstVert = vertices.numElem();

			for(int j = 0; j < smVerts.numElem(); j++)
				vertices.append( smVerts[j] );

			for(int j = 0; j < smIndices.numElem(); j++)
				indices.append( firstVert + smIndices[j] );

			// deselect triangles
			m_selectedTriangles.selectedSurfaces[i].triangle_ids.clear();

			delete [] vert_remap;

			IMaterial* pMat = pSurf->GetMaterial();
			pSurf->MakeCustomMesh( vertices.ptr(), indices.ptr(), vertices.numElem(), indices.numElem() );
			pSurf->SetMaterial(pMat);
		}
	}
}

void RemovePolygonsFromSelection(surfaceTriangleSelectionData_t* data)
{
	CEditableSurface* pSurf = data->pSurface;

	int nTriangles = data->triangle_ids.numElem();

	if(nTriangles == 0)
		return;

	DkList<eqlevelvertex_t>		vertices;
	DkList<int>					indices;

	int* vert_remap = new int[pSurf->GetVertexCount()];

	vertices.resize(pSurf->GetVertexCount());
	indices.resize(pSurf->GetIndexCount());

	for(int j = 0; j < pSurf->GetVertexCount(); j++)
		vert_remap[j] = -1;

#define ADD_REMAPPED_VERTEX(idx, vertex)		\
{												\
if(vert_remap[ idx ] != -1)					\
indices.append( vert_remap[ idx ] );	\
else										\
{											\
int currVerts = vertices.numElem();		\
vert_remap[idx] = currVerts;			\
indices.append( currVerts );			\
vertices.append( vertex );				\
}											\
}

	int nTriangle = 0;

	// add unselected polygons
	for(int j = 0; j < pSurf->GetIndexCount(); j+=3, nTriangle++)
	{
		if(data->triangle_ids.findIndex( nTriangle ) != -1)
			continue;
				
		int idx0 = pSurf->GetIndices()[j];
		int idx1 = pSurf->GetIndices()[j+1];
		int idx2 = pSurf->GetIndices()[j+2];

		ADD_REMAPPED_VERTEX(idx0, pSurf->GetVertices()[idx0])
		ADD_REMAPPED_VERTEX(idx1, pSurf->GetVertices()[idx1])
		ADD_REMAPPED_VERTEX(idx2, pSurf->GetVertices()[idx2])
	}

	// deselect triangles
	data->triangle_ids.clear();

	delete [] vert_remap;

	// DONE
	IMaterial* pMat = pSurf->GetMaterial();
	pSurf->MakeCustomMesh( vertices.ptr(), indices.ptr(), vertices.numElem(), indices.numElem() );
	pSurf->SetMaterial(pMat);
}

void CVertexTool::RemovePolygons()
{
	if(m_pPanel->GetEditMode() == VERTEXEDIT_TRIANGLE)
	{
		for(int i = 0; i < m_selectedTriangles.selectedSurfaces.numElem(); i++)
		{
			RemovePolygonsFromSelection(&m_selectedTriangles.selectedSurfaces[i]);
		}
	}
}

void CVertexTool::MakeSurfaceFromSelection()
{
	if(m_pPanel->GetEditMode() == VERTEXEDIT_TRIANGLE)
	{
		for(int i = 0; i < m_selectedTriangles.selectedSurfaces.numElem(); i++)
		{
			CEditableSurface* pSurf = m_selectedTriangles.selectedSurfaces[i].pSurface;

			int nTriangles = m_selectedTriangles.selectedSurfaces[i].triangle_ids.numElem();

			if(nTriangles == 0)
				continue;

			DkList<eqlevelvertex_t> smVerts;
			DkList<int>				smIndices;

			smVerts.resize(pSurf->GetVertexCount());
			smIndices.resize(pSurf->GetIndexCount());

			// add selected as cool with fixup
			for(int j = 0; j < nTriangles; j++)
			{
				int triId = m_selectedTriangles.selectedSurfaces[i].triangle_ids[j];

				int idx0 = pSurf->GetIndices()[triId*3];
				int idx1 = pSurf->GetIndices()[triId*3+1];
				int idx2 = pSurf->GetIndices()[triId*3+2];

				int nIdx0 = AddUniqueVertexToListNoNormal( pSurf->GetVertices()[idx0], smVerts);
				int nIdx1 = AddUniqueVertexToListNoNormal( pSurf->GetVertices()[idx1], smVerts);
				int nIdx2 = AddUniqueVertexToListNoNormal( pSurf->GetVertices()[idx2], smVerts);

				smIndices.append(nIdx0);
				smIndices.append(nIdx1);
				smIndices.append(nIdx2);
			}
			
			//Msg("Selected tris: %d, added: %g, verts: %d\n", nTriangles, float(smIndices.numElem()) / 3.0f, smVerts.numElem());

			IMaterial* pMat = pSurf->GetMaterial();

			CEditableSurface* pNewSurf = new CEditableSurface;
			pNewSurf->MakeCustomMesh( smVerts.ptr(), smIndices.ptr(), smVerts.numElem(), smIndices.numElem());
			pNewSurf->SetMaterial(pMat);
			pNewSurf->GetSurfaceTexture(0)->nFlags |= pSurf->GetSurfaceTexture(0)->nFlags;

			g_pLevel->AddEditable( pNewSurf );

			RemovePolygonsFromSelection( &m_selectedTriangles.selectedSurfaces[i] );
		}
	}
}

void CVertexTool::FlipTriangleOrders()
{
	if(m_pPanel->GetEditMode() == VERTEXEDIT_TRIANGLE)
	{
		for(int i = 0; i < m_selectedTriangles.selectedSurfaces.numElem(); i++)
		{
			CEditableSurface* pSurf = m_selectedTriangles.selectedSurfaces[i].pSurface;

			int nTriangles = m_selectedTriangles.selectedSurfaces[i].triangle_ids.numElem();

			DkList<eqlevelvertex_t>		verts;
			DkList<int>					indices;

			verts.resize(pSurf->GetVertexCount());
			indices.resize(pSurf->GetIndexCount());

			for(int j = 0; j < pSurf->GetVertexCount(); j++)
				verts.append(pSurf->GetVertices()[j]);

			// first, detect the triangles we want to flip
			for(int j = 0; j < pSurf->GetIndexCount(); j++)
				indices.append(pSurf->GetIndices()[j]);

			for(int j = 0; j < nTriangles; j++)
			{
				int idx0 = indices[m_selectedTriangles.selectedSurfaces[i].triangle_ids[j] * 3];
				int idx1 = indices[m_selectedTriangles.selectedSurfaces[i].triangle_ids[j] * 3 + 1];
				int idx2 = indices[m_selectedTriangles.selectedSurfaces[i].triangle_ids[j] * 3 + 2];

				indices[m_selectedTriangles.selectedSurfaces[i].triangle_ids[j] * 3]		= idx2;
				indices[m_selectedTriangles.selectedSurfaces[i].triangle_ids[j] * 3 + 2]	= idx0;

				verts[idx0].normal *= -1;
				verts[idx1].normal *= -1;
				verts[idx2].normal *= -1;
			}

			IMaterial* pMat = pSurf->GetMaterial();
			pSurf->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),indices.numElem());
			pSurf->SetMaterial(pMat);
		}
	}
	else if(m_pPanel->GetEditMode() == VERTEXEDIT_VERTEX)
	{
		// Create undo
		((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

		// check if we have single surface selected
		if(m_vertexselection.selectedSurfaces.numElem() > 1)
		{
			MsgWarning("You cannot add vertex when multiple objects are selected\n");
			return;
		}

		if(!m_vertexselection.selectedSurfaces.numElem())
			return;

		CEditableSurface* pSurf = m_vertexselection.selectedSurfaces[0].pSurface;

		if(m_vertexselection.selectedSurfaces[0].vertex_ids.numElem() < 3)
		{
			MsgWarning("You need to select three or more verts!\n");
			return;
		}

		DkList<eqlevelvertex_t>		verts;
		DkList<int>					indices;

		verts.resize(pSurf->GetVertexCount());
		indices.resize(pSurf->GetIndexCount());

		for(int i = 0; i < pSurf->GetVertexCount(); i++)
			verts.append(pSurf->GetVertices()[i]);

		// first, detect the triangles we want to flip
		for(int i = 0; i < pSurf->GetIndexCount(); i++)
			indices.append(pSurf->GetIndices()[i]);

		int nSelIdxs = m_vertexselection.selectedSurfaces[0].vertex_ids.numElem();

		// flip 'em all
		for(int i = 0; i < nSelIdxs; i++)
		{
			if(i+3 > nSelIdxs)
				break;

			int idx[3];

			idx[0] = m_vertexselection.selectedSurfaces[0].vertex_ids[i];
			idx[1] = m_vertexselection.selectedSurfaces[0].vertex_ids[i+1];
			idx[2] = m_vertexselection.selectedSurfaces[0].vertex_ids[i+2];

			// find selected triangle in the mesh
			int nTriangleId = ComputeTriangleIndexByIndices(idx, indices.ptr(), indices.numElem());

			if(nTriangleId == -1)
			{
				Msg("bad triangle selected for this operation!\n");
				continue; // bad triangle
			}

			int temp = indices[nTriangleId*3];

			indices[nTriangleId*3] = indices[nTriangleId*3+2];
			indices[nTriangleId*3+2] = temp;
		}

		IMaterial* pMat = pSurf->GetMaterial();
		pSurf->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),indices.numElem());
		pSurf->SetMaterial(pMat);

		UpdateSelectionCenter();
	}

	// Create undo
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();
}

void CVertexTool::RemoveSelectedVerts()
{
	// check if we have single surface selected
	if(m_vertexselection.selectedSurfaces.numElem() > 1)
	{
		MsgWarning("You cannot add vertex when multiple objects are selected\n");
		return;
	}

	if(!m_vertexselection.selectedSurfaces.numElem())
		return;

	CEditableSurface* pSurf = m_vertexselection.selectedSurfaces[0].pSurface;

	DkList<eqlevelvertex_t>		verts;
	DkList<int>					indices;

	verts.resize(pSurf->GetVertexCount());
	indices.resize(pSurf->GetIndexCount());

	DkList<int>					use_indices;
	use_indices.resize(pSurf->GetIndexCount());

	for(int i = 0; i < pSurf->GetIndexCount(); i+=3)
	{
		int found_idx[3];
		found_idx[0] = m_vertexselection.selectedSurfaces[0].vertex_ids.findIndex(pSurf->GetIndices()[i]);
		found_idx[1] = m_vertexselection.selectedSurfaces[0].vertex_ids.findIndex(pSurf->GetIndices()[i+1]);
		found_idx[2] = m_vertexselection.selectedSurfaces[0].vertex_ids.findIndex(pSurf->GetIndices()[i+2]);

		if(found_idx[0] == -1 && found_idx[1] == -1 && found_idx[2] == -1)
		{
			use_indices.append(pSurf->GetIndices()[i]);
			use_indices.append(pSurf->GetIndices()[i+1]);
			use_indices.append(pSurf->GetIndices()[i+2]);
		}
	}

	for(int i = 0; i < use_indices.numElem(); i++)
	{
		int idx = verts.addUnique(pSurf->GetVertices()[use_indices[i]]);
		indices.append(idx);
	}

	int triangle_count = indices.numElem() / 3;

	IMaterial* pMat = pSurf->GetMaterial();
	pSurf->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(),triangle_count*3);
	pSurf->SetMaterial(pMat);

	UpdateSelectionCenter();

	// Create undo
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

	ClearSelection();
}

void CVertexTool::ClearSelection()
{
	m_vertexselection.selectedBrushes.clear();
	m_vertexselection.selectedSurfaces.clear();

	m_selectedTriangles.selectedSurfaces.clear();
}

// down/up key events
void CVertexTool::OnKey(wxKeyEvent& event, bool bDown, CEditorViewRender* pViewRender)
{
	if(!bDown)
	{
		if(event.GetKeyCode() == WXK_SPACE)
		{
			ExtrudeEdges();
			UpdateAllWindows();
		}
		else if(event.ControlDown())
		{
			if(event.GetRawKeyCode() == 'G')
			{
				SnapSelectedVertsToGrid();
				SnapVector3D(m_selectionoffset);
			}
			else if(event.GetRawKeyCode() == 'F')
			{
				FlipTriangleOrders();
				UpdateAllWindows();
			}
			else if(event.GetRawKeyCode() == 'R')
			{
				FlipQuads();
				UpdateAllWindows();
			}
		}
		else
		{
			if(event.GetRawKeyCode() == 'F')
			{
				AddTriangle();
				UpdateAllWindows();
			}
			if(event.GetRawKeyCode() == 'S')
			{
				MakeSmoothingGroup();
				UpdateAllWindows();
			}
			if(event.GetRawKeyCode() == 'D')
			{
				MakeSurfaceFromSelection();
				UpdateAllWindows();
			}
		}

		if(event.m_keyCode == WXK_DELETE)
		{
			RemovePolygons();
			RemoveSelectedVerts();
			UpdateAllWindows();
		}

		if(event.m_keyCode == WXK_RETURN)
		{
			if(GetSelectionState() == SELECTION_PREPARATION)
			{
				// select verts
				selectiondata_t data;
				data.pSelectionData = &m_vertexselection;
				data.pTriangleData = &m_selectedTriangles;

				GetSelectionBBOX(data.selectionbox.minPoint, data.selectionbox.maxPoint);
				data.selectionbox.Fix();

				// apply current vertices if we've got changes TODO: tell user?
				ApplyVertexManipulation(false);

				if(data.selectionbox.minPoint.x == data.selectionbox.maxPoint.x)
				{
					data.selectionbox.minPoint.x = -MAX_COORD_UNITS;
					data.selectionbox.maxPoint.x = MAX_COORD_UNITS;
				}

				if(data.selectionbox.minPoint.y == data.selectionbox.maxPoint.y)
				{
					data.selectionbox.minPoint.y = -MAX_COORD_UNITS;
					data.selectionbox.maxPoint.y = MAX_COORD_UNITS;
				}

				if(data.selectionbox.minPoint.z == data.selectionbox.maxPoint.z)
				{
					data.selectionbox.minPoint.z = -MAX_COORD_UNITS;
					data.selectionbox.maxPoint.z = MAX_COORD_UNITS;
				}

				if(m_pPanel->GetEditMode() == VERTEXEDIT_VERTEX)
					((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects( SelectVertices, &data, false );
				else
					((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects( SelectTriangles, &data, false );

				CancelSelection();

				UpdateSelectionCenter();
			}
		}
		else if(event.GetKeyCode() == WXK_ESCAPE)
		{
			ClearSelection();
			CancelSelection();

			ApplyVertexManipulation();
		}
	}
}