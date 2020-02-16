//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightmap editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#include "UI_RoadEditor.h"
#include "world.h"

#include "materialsystem/MeshBuilder.h"

IMaterial* g_helpersMaterial = nullptr;

CUI_RoadEditor::CUI_RoadEditor( wxWindow* parent) : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize )
{
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer5->Add( new wxStaticText( this, wxID_ANY, wxT("Type (T)"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_typeSelChoices[] = { wxT("Straight"), wxT("Junction"), wxT("Parking lot"), wxT("Pavement") };
	int m_typeSelNChoices = sizeof( m_typeSelChoices ) / sizeof( wxString );
	m_typeSel = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( 100,-1 ), m_typeSelNChoices, m_typeSelChoices, 0 );
	m_typeSel->SetSelection( 0 );
	fgSizer5->Add( m_typeSel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	fgSizer5->Add( new wxStaticText( this, wxID_ANY, wxT("Direction (SPC)"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_rotationSelChoices[] = { wxT("North"), wxT("East"), wxT("South"), wxT("West") };
	int m_rotationSelNChoices = sizeof( m_rotationSelChoices ) / sizeof( wxString );
	m_rotationSel = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_rotationSelNChoices, m_rotationSelChoices, 0 );
	m_rotationSel->SetSelection( 0 );
	fgSizer5->Add( m_rotationSel, 0, wxALL|wxEXPAND, 5 );

	m_parking = new wxCheckBox(this, wxID_ANY, wxT("Is parking"));

	fgSizer5->Add( m_parking, 0, wxALL|wxEXPAND, 5 );

	this->SetSizer( fgSizer5 );
	this->Layout();

	m_typeSel->Connect(wxEVT_CHOICE, (wxObjectEventFunction)&CUI_RoadEditor::OnRotationOrTypeTextChanged, NULL, this);
	m_rotationSel->Connect(wxEVT_CHOICE, (wxObjectEventFunction)&CUI_RoadEditor::OnRotationOrTypeTextChanged, NULL, this);

	m_type = ROADTYPE_STRAIGHT;
	m_rotation = 0;

	m_trafficDir = NULL;
	m_trafficDirVar = NULL;
	m_trafficParking = NULL;
	m_pavement = NULL;
}

CUI_RoadEditor::~CUI_RoadEditor()
{
}

int	CUI_RoadEditor::GetRoadType()
{
	return m_type;
}

void CUI_RoadEditor::SetRoadType(int type)
{
	if(type == ROADTYPE_NOROAD)
		return;

	m_typeSel->SetSelection(type-1);

	m_type = type;
}

int CUI_RoadEditor::GetRotation()
{
	return m_rotation;
}

void CUI_RoadEditor::SetRotation(int rot)
{
	m_rotationSel->SetSelection(rot);
	m_rotation = rot;
}

void CUI_RoadEditor::OnRotationOrTypeTextChanged(wxCommandEvent& event)
{
	m_rotation = m_rotationSel->GetSelection();
	m_type = m_typeSel->GetSelection()+1;

	m_parking->SetValue(false);
}

// IEditorTool stuff

void CUI_RoadEditor::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  )
{
	int tileIdx = m_selectedRegion->GetHField()->m_sizew*ty + tx;
	levroadcell_t* roadCell = &m_selectedRegion->m_roads[tileIdx];

	if(event.ControlDown() || event.AltDown())
	{
		// select height editing mode
		if(event.AltDown())
		{
			if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
			{
				// left mouse button used for picking
				SetRoadType(roadCell->type);
				SetRotation(roadCell->direction);
			}
		}
	}
	else
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			roadCell->type = GetRoadType();
			roadCell->direction = GetRotation();

			roadCell->flags = 0;

			if( m_parking->GetValue() )
				roadCell->flags |= ROAD_FLAG_PARKING;
		}
		else if(event.ButtonUp(wxMOUSE_BTN_MIDDLE))
		{
			PaintLine(	m_globalTile_lineStart.x,m_globalTile_lineStart.y,
						m_globalTile_lineEnd.x, m_globalTile_lineEnd.y);
		}
		else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
		{
			// remove
			roadCell->type = ROADTYPE_NOROAD;
			roadCell->flags = 0;
		}
	}
}

void CUI_RoadEditor::PaintLine(int x0, int y0, int x1, int y1)
{
    float dX,dY,iSteps;
    float xInc,yInc,iCount,x,y;

    dX = x1 - x0;
    dY = y1 - y0;

    if (fabs(dX) > fabs(dY))
    {
        iSteps = fabs(dX);
    }
    else
    {
        iSteps = fabs(dY);
    }

    xInc = dX/iSteps;
    yInc = dY/iSteps;

    x = x0;
    y = y0;

	Vector2D dir(dX,dY);
	IVector2D idir = normalize(dir);

	int dirIdx = GetDirectionIndex(idir);

    for (iCount=0; iCount <= iSteps; iCount++)
    {
		//float percentage = (float)iCount/(float)iSteps;

		PaintPointGlobal( floor(x), floor(y), dirIdx);

        x += xInc;
        y += yInc;
    }

    return;
}

void CUI_RoadEditor::PaintPointGlobal(int x, int y, int direction)
{
	CLevelRegion* pReg = NULL;
	IVector2D local;
	g_pGameWorld->m_level.GlobalToLocalPoint(IVector2D(x,y), local, &pReg);

	if(!pReg)
		return;

	int tileIdx = pReg->GetHField()->m_sizew*local.y + local.x;
	levroadcell_t* roadCell = &pReg->m_roads[tileIdx];

	if(!IsJunctionType((ERoadType)roadCell->type))
		roadCell->type = GetRoadType();

	roadCell->direction = direction;
	roadCell->flags = 0;
}

void CUI_RoadEditor::OnKey(wxKeyEvent& event, bool bDown)
{
	// hotkeys
	if(!bDown)
	{
		if(event.m_keyCode == WXK_SPACE)
		{
			m_rotation += 1;

			if(m_rotation > 3)
				m_rotation = 0;

			SetRotation(m_rotation);
		}
		else if(event.GetRawKeyCode() == 'T')
		{
			m_type += 1;

			if(m_type > 4)
				m_type = 1;

			SetRoadType(m_type);
		}
	}
}

void ListQuadTex(const Vector3D &v1, const Vector3D &v2, const Vector3D& v3, const Vector3D& v4, int rotate, const ColorRGBA &color, DkList<Vertex3D_t> &verts)
{
	float dxv[4] = NEIGHBOR_OFFS_DX(0.5f, 0.5);
	float dyv[4] = NEIGHBOR_OFFS_DY(0.5f, 0.5);

	int rot1 = rotate;
	int rot2 = rotate+1;
	int rot3 = rotate+2;
	int rot4 = rotate+3;

	if(rot1 > 3)
		rot1 -= 4;
	if(rot2 > 3)
		rot2 -= 4;
	if(rot3 > 3)
		rot3 -= 4;
	if(rot4 > 3)
		rot4 -= 4;

	verts.append(Vertex3D_t(v3, Vector2D(dxv[rot3], dyv[rot3]), color));
	verts.append(Vertex3D_t(v2, Vector2D(dxv[rot2], dyv[rot2]), color));
	verts.append(Vertex3D_t(v1, Vector2D(dxv[rot1], dyv[rot1]), color));

	verts.append(Vertex3D_t(v4, Vector2D(dxv[rot4], dyv[rot4]), color));
	verts.append(Vertex3D_t(v3, Vector2D(dxv[rot3], dyv[rot3]), color));
	verts.append(Vertex3D_t(v1, Vector2D(dxv[rot1], dyv[rot1]), color));
}

extern void EdgeIndexToVertex(int edge, int& vi1, int& vi2);

void CUI_RoadEditor::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(m_selectedRegion)
	{
		CHeightTileFieldRenderable* field = m_selectedRegion->m_heightfield[0];

		field->DebugRender(false, m_mouseOverTileHeight, GridSize());

		//materials->BindMaterial(g_helpersMaterial);
		
		g_pShaderAPI->SetTexture(g_helpersMaterial->GetBaseTexture(), NULL, 0);
		materials->SetDepthStates(false, false);
		materials->SetRasterizerStates(CULL_BACK, FILL_SOLID);
		materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA);

		materials->BindMaterial(materials->GetDefaultMaterial());
		
		CMeshBuilder meshBuilder(materials->GetDynamicMesh());
		meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

		// don't forget about texture size
		Vector2D texSize(g_helpersMaterial->GetBaseTexture()->GetWidth(), g_helpersMaterial->GetBaseTexture()->GetHeight());
		Vector2D oneByTexSize(1.0f / texSize.x, 1.0f / texSize.y);

		for(int x = 0; x < field->m_sizew; x++)
		{
			for(int y = 0; y < field->m_sizeh; y++)
			{
				int pt_idx = y*field->m_sizew + x;

				levroadcell_t* cell = &m_selectedRegion->m_roads[pt_idx];

				if(cell->type == ROADTYPE_NOROAD && cell->flags == 0)
					continue;

				float dxv[4] = NEIGHBOR_OFFS_DX(x, 0.5f);
				float dyv[4] = NEIGHBOR_OFFS_DY(y, 0.5f);

				hfieldtile_t& tile = field->m_points[pt_idx];

				Vector3D p1(dxv[0] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[0] * HFIELD_POINT_SIZE);
				Vector3D p2(dxv[1] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[1] * HFIELD_POINT_SIZE);
				Vector3D p3(dxv[2] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[2] * HFIELD_POINT_SIZE);
				Vector3D p4(dxv[3] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[3] * HFIELD_POINT_SIZE);

				p1 += field->m_position;
				p2 += field->m_position;
				p3 += field->m_position;
				p4 += field->m_position;

				ColorRGBA tileColor(0.75, 0.75, 0.75, 0.5f);

				TexAtlasEntry_t* entry = m_trafficDir;

				if(cell->type == ROADTYPE_STRAIGHT)
				{
					if(cell->flags & ROAD_FLAG_PARKING)
						tileColor[cell->direction] += 0.25f;
					else
						tileColor[cell->direction] += 0.25f;
				}
				else if(cell->type == ROADTYPE_JUNCTION)
				{
					entry = m_trafficDirVar;
				}
				else if(cell->type == ROADTYPE_PARKINGLOT)
				{
					tileColor.x = 1.0f;
					entry = m_trafficParking;
				}
				else if (cell->type == ROADTYPE_PAVEMENT)
				{
					tileColor.x = 1.0f;
					tileColor.y = 1.0f;
					tileColor.z = 1.0f;

					entry = m_pavement;
				}

				float angle = cell->direction*90.0f;

				Matrix2x2 rotation = rotate2(DEG2RAD(angle));

				Rectangle_t rect(entry->rect);
				rect.vleftTop *= texSize;
				rect.vrightBottom *= texSize;

				Vector2D center = rect.GetCenter();

				// rotate the vertices
				Vector2D tc1 = (rotation * (rect.GetLeftBottom() - center) + center)*oneByTexSize;
				Vector2D tc2 = (rotation * (rect.GetRightBottom() - center) + center)*oneByTexSize;
				Vector2D tc3 = (rotation * (rect.GetLeftTop() - center) + center)*oneByTexSize;
				Vector2D tc4 = (rotation * (rect.GetRightTop() - center) + center)*oneByTexSize;

				meshBuilder.Color4fv(tileColor);
				meshBuilder.TexturedQuad3(p4, p3, p1, p2, tc1,tc2,tc3,tc4);
			}
		}

		meshBuilder.End();
	}

	CBaseTilebasedEditor::OnRender();
}

void CUI_RoadEditor::InitTool()
{
	g_helpersMaterial = materials->GetMaterial("editor/helpers");
	materials->PutMaterialToLoadingQueue(g_helpersMaterial);
	g_helpersMaterial->Ref_Grab();

	if (g_helpersMaterial->GetAtlas())
	{
		m_trafficDir = g_helpersMaterial->GetAtlas()->FindEntry("traffic_dir");
		m_trafficDirVar = g_helpersMaterial->GetAtlas()->FindEntry("traffic_dir_variant");
		m_trafficParking = g_helpersMaterial->GetAtlas()->FindEntry("traffic_dir_parking");
		m_pavement = g_helpersMaterial->GetAtlas()->FindEntry("pavement");
	}
}

void CUI_RoadEditor::ReloadTool()
{

}

void CUI_RoadEditor::ShutdownTool()
{
	materials->FreeMaterial(g_helpersMaterial);
	g_helpersMaterial = nullptr;
}

void CUI_RoadEditor::Update_Refresh()
{

}