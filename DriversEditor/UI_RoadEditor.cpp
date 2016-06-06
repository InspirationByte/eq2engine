//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightmap editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#include "UI_RoadEditor.h"
#include "world.h"

CUI_RoadEditor::CUI_RoadEditor( wxWindow* parent) : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize )
{
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer5->Add( new wxStaticText( this, wxID_ANY, wxT("Type (T)"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_typeSelChoices[] = { wxT("Straight"), wxT("Junction"), wxT("Parking lot") };
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

int GetDirectionIndex(const IVector2D& vec)
{
	int rX[4] = ROADNEIGHBOUR_OFFS_X(0);
	int rY[4] = ROADNEIGHBOUR_OFFS_Y(0);

	IVector2D v = clamp(vec, IVector2D(-1,-1),IVector2D(1,1));

	for(int i = 0; i < 4; i++)
	{
		IVector2D r(rX[i], rY[i]);

		if(r == v)
			return i;
	}

	return 0;
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

	if(roadCell->type != ROADTYPE_JUNCTION)
		roadCell->type = GetRoadType();

	roadCell->direction = direction;
	roadCell->flags = 0;
}

void CUI_RoadEditor::ProcessMouseEvents( wxMouseEvent& event )
{
	CBaseTilebasedEditor::ProcessMouseEvents(event);
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

			if(m_type > 3)
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

		field->DebugRender(false, m_mouseOverTileHeight);

		DkList<Vertex3D_t> straight_verts(64);
		straight_verts.resize(field->m_sizew*field->m_sizeh*6);

		DkList<Vertex3D_t> variant_verts(64);
		variant_verts.resize(field->m_sizew*field->m_sizeh*6);

		DkList<Vertex3D_t> parking_verts(64);
		parking_verts.resize(field->m_sizew*field->m_sizeh*6);

		for(int x = 0; x < field->m_sizew; x++)
		{
			for(int y = 0; y < field->m_sizeh; y++)
			{
				int pt_idx = y*field->m_sizew + x;

				levroadcell_t* cell = &m_selectedRegion->m_roads[pt_idx];

				if(cell->type == ROADTYPE_NOROAD && cell->flags == 0)
					continue;

				float dxv[4] = NEIGHBOR_OFFS_DX(x, 0.5);
				float dyv[4] = NEIGHBOR_OFFS_DY(y, 0.5);

				hfieldtile_t& tile = field->m_points[pt_idx];

				Vector3D p1(dxv[0] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[0] * HFIELD_POINT_SIZE);
				Vector3D p2(dxv[1] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[1] * HFIELD_POINT_SIZE);
				Vector3D p3(dxv[2] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[2] * HFIELD_POINT_SIZE);
				Vector3D p4(dxv[3] * HFIELD_POINT_SIZE, float(tile.height)*HFIELD_HEIGHT_STEP+0.1f, dyv[3] * HFIELD_POINT_SIZE);

				p1 += field->m_position;
				p2 += field->m_position;
				p3 += field->m_position;
				p4 += field->m_position;

				ColorRGBA tileColor(0.75,0.75,0.75,0.5f);

				if(cell->type == ROADTYPE_STRAIGHT)
				{
					if(cell->flags & ROAD_FLAG_PARKING)
					{
						tileColor[cell->direction] += 0.25f;
						ListQuadTex(p1, p2, p3, p4, cell->direction, tileColor, parking_verts);
					}
					else
					{
						tileColor[cell->direction] += 0.25f;
						ListQuadTex(p1, p2, p3, p4, cell->direction, tileColor, straight_verts);
					}
				}
				else if(cell->type == ROADTYPE_JUNCTION)
				{
					ListQuadTex(p1, p2, p3, p4, cell->direction, tileColor, variant_verts);
				}
				else if(cell->type == ROADTYPE_PARKINGLOT)
				{
					tileColor.x = 1.0f;
					ListQuadTex(p1, p2, p3, p4, cell->direction, tileColor, parking_verts);
				}
			}
		}

		DepthStencilStateParams_t depth;

		depth.depthTest = false;
		depth.depthWrite = false;
		depth.depthFunc = COMP_LEQUAL;

		BlendStateParam_t blend;

		blend.blendEnable = true;
		blend.srcFactor = BLENDFACTOR_SRC_ALPHA;
		blend.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

		RasterizerStateParams_t raster;

		raster.cullMode = CULL_BACK;
		raster.fillMode = FILL_SOLID;
		raster.multiSample = true;
		raster.scissor = false;

		if(straight_verts.numElem())
			materials->DrawPrimitivesFFP(PRIM_TRIANGLES, straight_verts.ptr(), straight_verts.numElem(), m_trafficDir->GetBaseTexture(), color4_white, &blend, &depth, &raster);

		if(variant_verts.numElem())
			materials->DrawPrimitivesFFP(PRIM_TRIANGLES, variant_verts.ptr(), variant_verts.numElem(), m_trafficDirVar->GetBaseTexture(), color4_white, &blend, &depth, &raster);
		
		if(parking_verts.numElem())
			materials->DrawPrimitivesFFP(PRIM_TRIANGLES, parking_verts.ptr(), parking_verts.numElem(), m_trafficParking->GetBaseTexture(), color4_white, &blend, &depth, &raster);
	}

	CBaseTilebasedEditor::OnRender();
}

void CUI_RoadEditor::InitTool()
{
	m_trafficDir = materials->FindMaterial("traffic_dir");
	m_trafficDirVar = materials->FindMaterial("traffic_dir_variant");
	m_trafficParking = materials->FindMaterial("traffic_dir_parking");

	materials->PutMaterialToLoadingQueue(m_trafficDir);
	materials->PutMaterialToLoadingQueue(m_trafficDirVar);
	materials->PutMaterialToLoadingQueue(m_trafficParking);

	m_trafficDir->Ref_Grab();
	m_trafficDirVar->Ref_Grab();
	m_trafficParking->Ref_Grab();
}

void CUI_RoadEditor::ReloadTool()
{

}

void CUI_RoadEditor::Update_Refresh()
{

}