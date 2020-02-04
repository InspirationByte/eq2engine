//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightmap editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#include "UI_HeightEdit.h"
#include "EditorLevel.h"
#include "EditorMain.h"

#include "MaterialAtlasList.h"

#include "TextureView.h"

#include "IDebugOverlay.h"
#include "math/rectangle.h"
#include "math/boundingbox.h"
#include "math/math_util.h"

#include "MeshBuilder.h"

#include "world.h"

class CMaterialReplaceDialog : public wxDialog 
{
public:
		
	~CMaterialReplaceDialog()
	{
		// Disconnect Events
		m_selBtn1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnSelect1 ), NULL, this );
		m_selBtn2->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnSelect2 ), NULL, this );
		m_replaceBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnReplaceClick ), NULL, this );
	}

	CMaterialReplaceDialog( wxWindow* parent ) : 
		wxDialog( parent, wxID_ANY, wxT("Replace heightmap material"), wxDefaultPosition, wxSize( 791,256 ), wxDEFAULT_DIALOG_STYLE  )
	{
		this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
		wxBoxSizer* bSizer13;
		bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
		wxStaticBoxSizer* sbSizer7;
		sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Which to replace") ), wxVERTICAL );
	
		m_preview1 = new CTextureView( this, wxDefaultPosition, wxSize( 128,128 ) );
		m_preview1->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
		sbSizer7->Add( m_preview1, 0, wxALL, 5 );
	
		m_name1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
		sbSizer7->Add( m_name1, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
		m_selBtn1 = new wxButton( this, wxID_ANY, wxT("From selection"), wxDefaultPosition, wxDefaultSize, 0 );
		sbSizer7->Add( m_selBtn1, 0, wxALL, 5 );
	
	
		bSizer13->Add( sbSizer7, 1, wxEXPAND|wxRIGHT, 5 );
	
		wxStaticBoxSizer* sbSizer71;
		sbSizer71 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Replace to") ), wxVERTICAL );
	
		m_preview2 = new CTextureView( this, wxDefaultPosition, wxSize( 128,128 ) );
		m_preview2->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
		sbSizer71->Add( m_preview2, 0, wxALL, 5 );
	
		m_name2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
		sbSizer71->Add( m_name2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
		m_selBtn2 = new wxButton( this, wxID_ANY, wxT("From selection"), wxDefaultPosition, wxDefaultSize, 0 );
		sbSizer71->Add( m_selBtn2, 0, wxALL, 5 );
	
	
		bSizer13->Add( sbSizer71, 1, wxEXPAND, 5 );
	
		wxBoxSizer* bSizer14;
		bSizer14 = new wxBoxSizer( wxVERTICAL );
	
		m_replaceBtn = new wxButton( this, wxID_ANY, wxT("Replace All"), wxDefaultPosition, wxDefaultSize, 0 );
		bSizer14->Add( m_replaceBtn, 0, wxALL|wxALIGN_BOTTOM, 5 );
	

		bSizer13->Add( bSizer14, 0, wxEXPAND, 5 );
	
	
		this->SetSizer( bSizer13 );
		this->Layout();
	
		this->Centre( wxBOTH );
	
		// Connect Events
		m_selBtn1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnSelect1 ), NULL, this );
		m_selBtn2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnSelect2 ), NULL, this );
		m_replaceBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnReplaceClick ), NULL, this );
		m_replaceAllBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CMaterialReplaceDialog::OnReplaceClick ), NULL, this );
	}


protected:

	void OnSelect1( wxCommandEvent& event )
	{

	}

	void OnSelect2( wxCommandEvent& event )
	{

	}

	void OnReplaceClick( wxCommandEvent& event )
	{

	}
		
	CTextureView* m_preview1;
	wxTextCtrl* m_name1;
	wxButton* m_selBtn1;
	CTextureView* m_preview2;
	wxTextCtrl* m_name2;
	wxButton* m_selBtn2;
	wxButton* m_replaceBtn;
	wxButton* m_replaceAllBtn;

	matAtlasElem_t m_selection1;
	matAtlasElem_t m_selection2;
};



//-----------------------------------------------------------------------------------------------------------------------
// CMaterialAtlasList holder
//-----------------------------------------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CUI_HeightEdit, wxPanel)
	//EVT_SIZE(CTextureListPanel::OnSize)
	EVT_CLOSE(CUI_HeightEdit::OnClose)

END_EVENT_TABLE()

CUI_HeightEdit::CUI_HeightEdit(wxWindow* parent) : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize), CBaseTilebasedEditor()
{
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* colSizer2;
	colSizer2 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer9;
	sbSizer9 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("What paint:") ), wxVERTICAL );
	
	m_paintMaterial = new wxCheckBox( this, wxID_ANY, wxT("material"), wxDefaultPosition, wxDefaultSize, 0 );
	m_paintMaterial->SetValue(true); 
	sbSizer9->Add( m_paintMaterial, 0, wxALL, 5 );
	
	m_paintRotation = new wxCheckBox( this, wxID_ANY, wxT("rotation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_paintRotation->SetValue(true); 
	sbSizer9->Add( m_paintRotation, 0, wxALL, 5 );
	
	m_paintFlags = new wxCheckBox( this, wxID_ANY, wxT("flags"), wxDefaultPosition, wxDefaultSize, 0 );
	m_paintFlags->SetValue(true); 
	sbSizer9->Add( m_paintFlags, 0, wxALL, 5 );
	
	
	colSizer2->Add( sbSizer9, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* m_f;
	m_f = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Tile Flags") ), wxVERTICAL );
	
	m_detached = new wxCheckBox( this, wxID_ANY, wxT("detached"), wxDefaultPosition, wxDefaultSize, 0 );
	m_f->Add( m_detached, 0, wxALL, 5 );
	
	m_addWall = new wxCheckBox( this, wxID_ANY, wxT("add wall"), wxDefaultPosition, wxDefaultSize, 0 );
	m_f->Add( m_addWall, 0, wxALL, 5 );
	
	m_wallCollide = new wxCheckBox( this, wxID_ANY, wxT("wall collision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_f->Add( m_wallCollide, 0, wxALL, 5 );
	
	m_noCollide = new wxCheckBox( this, wxID_ANY, wxT("no collide"), wxDefaultPosition, wxDefaultSize, 0 );
	m_f->Add( m_noCollide, 0, wxALL, 5 );
	
	
	colSizer2->Add( m_f, 0, wxEXPAND, 5 );
	
	
	bSizer5->Add( colSizer2, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_heightPaintModeChoices[] = { wxT("Add"), wxT("Smooth"), wxT("Set") };
	int m_heightPaintModeNChoices = sizeof( m_heightPaintModeChoices ) / sizeof( wxString );
	m_heightPaintMode = new wxRadioBox( this, wxID_ANY, wxT("Height paint"), wxDefaultPosition, wxDefaultSize, m_heightPaintModeNChoices, m_heightPaintModeChoices, 1, wxRA_SPECIFY_COLS );
	m_heightPaintMode->SetSelection( 2 );
	bSizer3->Add( m_heightPaintMode, 0, wxBOTTOM|wxLEFT, 5 );
	
	m_quadratic = new wxCheckBox( this, wxID_ANY, wxT("Quadratic radius"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_quadratic, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("Height"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_height = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, -32767, 32767, 0 );
	fgSizer7->Add( m_height, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("Radius"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_radius = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 0, 32767, 0 );
	fgSizer7->Add( m_radius, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("LAYER"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_layer = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 0, ENGINE_REGION_MAX_HFIELDS-1, 0 );
	fgSizer7->Add( m_layer, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer3->Add( fgSizer7, 0, 0, 5 );
	
	bSizer5->Add( bSizer3, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_texPanel = new CMaterialAtlasList(this);
	
	bSizer9->Add( m_texPanel, 1, wxEXPAND|wxALL, 5 );
	
	m_pSettingsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,150 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Search and Filter") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer1->Add( new wxStaticText( m_pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_filtertext = new wxTextCtrl( m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	m_filtertext->SetMaxLength( 0 ); 
	fgSizer1->Add( m_filtertext, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pSettingsPanel, wxID_ANY, wxT("Tags"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pTags = new wxTextCtrl( m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	m_pTags->SetMaxLength( 0 ); 
	fgSizer1->Add( m_pTags, 0, wxALL, 5 );
	
	
	sbSizer2->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	m_onlyusedmaterials = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("SHOW ONLY USED MATERIALS"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_onlyusedmaterials, 0, wxALL, 5 );
	
	m_pSortByDate = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("SORT BY DATE"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_pSortByDate, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Display") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer2->Add( new wxStaticText( m_pSettingsPanel, wxID_ANY, wxT("Preview size"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_pPreviewSizeChoices[] = { wxT("64"), wxT("128"), wxT("256") };
	int m_pPreviewSizeNChoices = sizeof( m_pPreviewSizeChoices ) / sizeof( wxString );
	m_pPreviewSize = new wxChoice( m_pSettingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPreviewSizeNChoices, m_pPreviewSizeChoices, 0 );
	m_pPreviewSize->SetSelection( 0 );
	fgSizer2->Add( m_pPreviewSize, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_pAspectCorrection = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("ASPECT CORRECTION"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_pAspectCorrection, 0, wxALL, 5 );
	
	
	sbSizer3->Add( fgSizer2, 0, wxEXPAND, 5 );
	
	
	bSizer10->Add( sbSizer3, 0, wxEXPAND, 5 );
	
	
	m_pSettingsPanel->SetSizer( bSizer10 );
	m_pSettingsPanel->Layout();
	bSizer9->Add( m_pSettingsPanel, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer5->Add( bSizer9, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer5 );
	this->Layout();

	m_filtertext->Connect(wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CUI_HeightEdit::OnFilterTextChanged, NULL, this);
	m_onlyusedmaterials->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_HeightEdit::OnChangePreviewParams, NULL, this);
	m_pSortByDate->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_HeightEdit::OnChangePreviewParams, NULL, this);
	m_pPreviewSize->Connect(wxEVT_COMMAND_CHOICE_SELECTED, (wxObjectEventFunction)&CUI_HeightEdit::OnChangePreviewParams, NULL, this);
	m_pAspectCorrection->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_HeightEdit::OnChangePreviewParams, NULL, this);

	m_layer->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED, (wxObjectEventFunction)&CUI_HeightEdit::OnLayerSpinChanged, NULL, this);

	m_height->SetValue(1);
	m_radius->SetValue(0);

	//------------------------------------------------------------

	m_rotation = 0;
	m_selectedHField = 0;

	m_isLineMode = false;
}

void CUI_HeightEdit::OnChangePreviewParams(wxCommandEvent& event)
{
	int values[] =
	{
		128,
		256,
		512,
	};

	m_texPanel->SetPreviewParams(values[m_pPreviewSize->GetSelection()], m_pAspectCorrection->GetValue());

	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

void CUI_HeightEdit::OnFilterTextChanged(wxCommandEvent& event)
{
	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

void CUI_HeightEdit::OnLayerSpinChanged(wxCommandEvent& event)
{
	m_selectedHField = m_layer->GetValue();
}

void CUI_HeightEdit::OnClose(wxCloseEvent& event)
{
	Hide();
}

IMaterial* CUI_HeightEdit::GetSelectedMaterial()
{
	return m_texPanel->GetSelectedMaterial();
}

int CUI_HeightEdit::GetSelectedAtlasIndex() const
{
	return m_texPanel->GetSelectedAtlas();
}

void CUI_HeightEdit::ReloadMaterialList()
{
	m_texPanel->ReloadMaterialList();
}

CMaterialAtlasList*	CUI_HeightEdit::GetTexturePanel()
{
	return m_texPanel;
}

void CUI_HeightEdit::SetRotation(int rot)
{
	m_rotation = rot;
}

int CUI_HeightEdit::GetRotation()
{
	return m_rotation;
}

int CUI_HeightEdit::GetHeightfieldFlags()
{
	int flags = 0;

	flags |= m_addWall->GetValue()		? EHTILE_ADDWALL : 0;
	flags |= m_wallCollide->GetValue()	? EHTILE_COLLIDE_WALL : 0;
	flags |= m_detached->GetValue()		? EHTILE_DETACHED : 0;
	flags |= m_noCollide->GetValue()	? EHTILE_NOCOLLIDE : 0;

	return flags;
}

void CUI_HeightEdit::SetHeightfieldFlags(int flags)
{
	m_addWall->SetValue((flags & EHTILE_ADDWALL) > 0);
	m_wallCollide->SetValue((flags & EHTILE_COLLIDE_WALL) > 0);
	m_detached->SetValue((flags & EHTILE_DETACHED) > 0);
	m_noCollide->SetValue((flags & EHTILE_NOCOLLIDE) > 0);
}

int	CUI_HeightEdit::GetAddHeight() const
{
	return m_height->GetValue();
}

int CUI_HeightEdit::GetStartHeight() const
{
	CLevelRegion* reg;
	IVector2D tilePos;

	if(!g_pGameWorld->m_level.GetRegionAndTileAt(m_globalTile_lineStart, &reg, tilePos))
		return GetAddHeight();

	hfieldtile_t* tile = reg->GetHField(0)->GetTile(tilePos.x,tilePos.y);
	return tile->height;
}

int	CUI_HeightEdit::GetEndHeight() const
{
	CLevelRegion* reg;
	IVector2D tilePos;

	if(!g_pGameWorld->m_level.GetRegionAndTileAt(m_globalTile_lineEnd, &reg, tilePos))
		return GetStartHeight();

	hfieldtile_t* tile = reg->GetHField(0)->GetTile(tilePos.x,tilePos.y);
	return tile->height;
}

bool CUI_HeightEdit::IsLineMode() const
{
	return m_isLineMode;
}

int	CUI_HeightEdit::GetRadius()
{
	return m_radius->GetValue();
}

void CUI_HeightEdit::SetRadius(int radius)
{
	m_radius->SetValue(radius);
}

void CUI_HeightEdit::SetHeight(int height)
{
	m_height->SetValue(height);
}

EEditMode CUI_HeightEdit::GetEditMode() const
{
	return (EEditMode)m_heightPaintMode->GetSelection();
}

int CUI_HeightEdit::GetEditorPaintFlags() const
{
	int flags = 0;
	flags |= m_paintMaterial->GetValue() ? HEDIT_PAINT_MATERIAL : 0;
	flags |= m_paintRotation->GetValue() ? HEDIT_PAINT_ROTATION : 0;
	flags |= m_paintFlags->GetValue() ? HEDIT_PAINT_FLAGS : 0;

	return flags;
}

//----------------------------------------------------------------

bool WRAP_PaintFieldModify(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileFieldRenderable* field, hfieldtile_t* tile, TILEPAINTFUNC func, int flags, float percent)
{
	if(!(flags & HEDIT_PAINT_NO_HISTORY))
		g_pEditorActionObserver->BeginModify( field );
	
	bool result = func(rx, ry, px, py, edit, field, tile, flags, percent);

	if(result &&
		(rx <= 0 || ry <= 0 ||
		rx >= field->m_sizew || ry >= field->m_sizeh))
		return true;

	return result;
}

void CUI_HeightEdit::PaintHeightfieldPointGlobal(int gx, int gy, TILEPAINTFUNC func, float percent, int addFlags)
{
	CLevelRegion* pReg = NULL;
	IVector2D local;
	g_pGameWorld->m_level.GlobalToLocalPoint(IVector2D(gx,gy), local, &pReg);

	if(pReg)
	{
		CHeightTileFieldRenderable* hfield = pReg->GetHField(m_selectedHField);

		CHeightTileFieldRenderable* field = hfield;
		hfieldtile_t* tile = hfield->GetTileAndNeighbourField(local.x, local.y, (CHeightTileField**)&field);

		if(!tile)
			return;

		bool hasChanges = WRAP_PaintFieldModify(local.x, local.y, local.x, local.y, this, field, tile, func, GetEditorPaintFlags() | addFlags, percent);

		if( hasChanges )
		{
			field->SetChanged();

			/*
			// TODO: push it to the change list

			// check if neighbour needs change too
			for(int i = 0; i < 8; i++)
			{
				CHeightTileFieldRenderable* neighbour = field;
				hfieldtile_t* ntile = field->GetTileAndNeighbourField(ix+neighbour_x[i], iy+neighbour_y[i], (CHeightTileField**)&neighbour);

				if(neighbour && neighbour != field)
					neighbour->SetChanged();
			}
			*/
		}
	}
}

void CUI_HeightEdit::PaintHeightfieldGlobal(int gx, int gy, TILEPAINTFUNC func, float percent, int addFlags)
{
	CLevelRegion* pReg = NULL;
	IVector2D local;
	g_pGameWorld->m_level.GlobalToLocalPoint(IVector2D(gx,gy), local, &pReg);

	//int neighbour_x[8] = NEIGHBOR_OFFS_XDX(0, 1);
	//int neighbour_y[8] = NEIGHBOR_OFFS_YDY(0, 1);

	if(pReg)
	{
		CHeightTileFieldRenderable* hfield = pReg->GetHField(m_selectedHField);

		bool quadraticRadius = m_quadratic->GetValue();

		int radius = GetRadius();

		// paint every tile in radius
		for(int x = 0; x < radius*2; x++)
		{
			for(int y = 0; y < radius*2; y++)
			{
				int rx = x-radius;
				int ry = y-radius;

				Vector2D rvec(rx, ry);

				bool doPaint = abs(rx) < radius && abs(ry) < radius;

				if(!quadraticRadius)
					doPaint = length(rvec) < radius;

				if(doPaint)
				{
					CHeightTileFieldRenderable* field = hfield;
					hfieldtile_t* tile = hfield->GetTileAndNeighbourField(local.x+rx, local.y+ry, (CHeightTileField**)&field);

					if(!tile)
						continue;
				
					int prx = local.x+rx;
					int pry = local.y+ry;

					// calculate tile address
					int ix = ROLLING_VALUE(prx, field->m_sizew);
					int iy = ROLLING_VALUE(pry, field->m_sizeh);

					bool hasChanges = WRAP_PaintFieldModify(rx, ry, ix, iy, this, field, tile, func, GetEditorPaintFlags() | addFlags, percent);

					if( hasChanges )
					{
						field->SetChanged();

						/*
						// TODO: push it to the change list

						// check if neighbour needs change too
						for(int i = 0; i < 8; i++)
						{
							CHeightTileFieldRenderable* neighbour = field;
							hfieldtile_t* ntile = field->GetTileAndNeighbourField(ix+neighbour_x[i], iy+neighbour_y[i], (CHeightTileField**)&neighbour);

							if(neighbour && neighbour != field)
								neighbour->SetChanged();
						}
						*/
					}
				}
			}
		}
	}
}

void CUI_HeightEdit::PaintHeightfieldLocal(int px, int py, TILEPAINTFUNC func, float percent, int addFlags)
{
	IVector2D globalPoint;
	g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(px,py), m_selectedRegion, globalPoint);

	PaintHeightfieldGlobal(globalPoint.x,globalPoint.y,func,percent, addFlags);
}

void CUI_HeightEdit::PaintHeightfieldPointLocal(int px, int py, TILEPAINTFUNC func, float percent, int addFlags)
{
	IVector2D globalPoint;
	g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(px,py), m_selectedRegion, globalPoint);

	PaintHeightfieldPointGlobal(globalPoint.x,globalPoint.y,func,percent, addFlags);
}

bool TexPaintFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent)
{
	bool isSet = false;
	
	if(flags & HEDIT_PAINT_MATERIAL)
	{
		isSet = field->SetPointMaterial(px, py, edit->GetSelectedMaterial());

		if(tile->atlasIdx != edit->GetSelectedAtlasIndex())
		{
			tile->atlasIdx = edit->GetSelectedAtlasIndex();
			isSet = true;
		}
	}
		

	isSet = isSet || (edit->GetRotation() != tile->rotatetex) || (tile->flags != edit->GetHeightfieldFlags());

	if( !isSet )
		return false;

	if(flags & HEDIT_PAINT_FLAGS)
	{
		tile->flags = edit->GetHeightfieldFlags();
	}

	if(flags & HEDIT_PAINT_ROTATION)
	{
		tile->rotatetex = edit->GetRotation();
	}

	g_pMainFrame->NotifyUpdate();

	return true;
}


bool NullTexPaintFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent)
{
	if(flags & HEDIT_PAINT_MATERIAL)
	{
		bool result = field->SetPointMaterial(px, py, NULL);

		if(result)
			g_pMainFrame->NotifyUpdate();

		return result;
	}
	
	return false;
}

bool HeightPaintUpFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent)
{
	EEditMode mode = edit->GetEditMode();

	if(edit->GetRadius() == 0)
		return false;

	float rVal = clamp(1.0f - length(Vector2D(rx,ry)) / float(edit->GetRadius()), 0.0f, 1.0f);

	if(mode == HEDIT_ADD)
	{
		tile->height += (float)edit->GetAddHeight()*rVal;
	}
	else
	{
		if(edit->IsLineMode() && mode == HEDIT_SMOOTH)
		{
			short newHeight = floor(lerp((float)edit->GetStartHeight(), (float)edit->GetEndHeight(), percent));
			tile->height = lerp(newHeight, tile->height, rVal);
		}
		else
		{
			if(mode == HEDIT_SMOOTH)
			{
				float move = ((float)tile->height - (float)edit->GetAddHeight())*HFIELD_HEIGHT_STEP*rVal;

				if(move != 0.0f)
					tile->height -= sign(move);
			}
			else if(mode == HEDIT_SET)
			{
				if(tile->height == edit->GetAddHeight())
					return false;

				tile->height = edit->GetAddHeight();
			}
		}
	}

	g_pMainFrame->NotifyUpdate();

	return true;
}

bool HeightPaintDnFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent)
{
	EEditMode mode = edit->GetEditMode();

	if(edit->GetRadius() == 0)
		return false;

	float rVal = clamp(1.0f - length(Vector2D(rx,ry)) / float(edit->GetRadius()), 0.0f, 1.0f);

	if(mode == HEDIT_ADD)
	{
		tile->height -= (float)edit->GetAddHeight()*rVal;
	}
	else if(mode == HEDIT_SMOOTH)
	{
		float move = (tile->height - edit->GetAddHeight())*HFIELD_HEIGHT_STEP*rVal;

		if(move != 0.0f)
			tile->height -= sign(move);
	}
	else if(mode == HEDIT_SET)
	{
		if(tile->height == edit->GetAddHeight())
			return false;

		tile->height = edit->GetAddHeight();
	}

	g_pMainFrame->NotifyUpdate();

	return true;
}

bool DrawHeightFieldTileFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent)
{
	if (tile)
	{
		Vector3D box_pos(px*HFIELD_POINT_SIZE, (tile->height+1)*HFIELD_HEIGHT_STEP, py*HFIELD_POINT_SIZE);
		Vector3D box_size(HFIELD_POINT_SIZE, 0.0f, HFIELD_POINT_SIZE);

		box_pos += field->m_position - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

		debugoverlay->Box3D(box_pos, box_pos + box_size, ColorRGBA(1, 1, 1, 0.25f));
	}

	return false;
}

void CUI_HeightEdit::ProcessMouseEvents( wxMouseEvent& event )
{
	CBaseTilebasedEditor::ProcessMouseEvents(event);

	if(!event.MiddleIsDown() && event.ControlDown())
	{
		int mouse_wheel = event.GetWheelRotation();
		SetRadius(GetRadius()+sign(mouse_wheel));
	}

	if (event.ButtonUp())
		g_pEditorActionObserver->EndAction();
}

void CUI_HeightEdit::PaintHeightfieldLine(int x0, int y0, int x1, int y1, TILEPAINTFUNC func, ELineMode mode)
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

    for (iCount=0; iCount <= iSteps; iCount++)
    {
		float percentage = (float)iCount/(float)iSteps;

		if(mode == HEDIT_LINEMODE_RADIUS)
			PaintHeightfieldGlobal(floor(x),floor(y), func, percentage);
		else
		{
			Vector2D widthDir(dY,dX);
			widthDir = normalize(widthDir);

			int radius = GetRadius();

			for(int rd = -(GetRadius()-1); rd < radius; rd++)
			{
				PaintHeightfieldPointGlobal(floor(x)+widthDir.x*rd,floor(y)+widthDir.y*rd, func, percentage);
			}
		}

        x += xInc;
        y += yInc;
    }

    return;
}

void CUI_HeightEdit::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  )
{
	if(event.ControlDown() || event.AltDown())
	{
		// select height editing mode
		if(event.ControlDown())
		{
			if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
			{
				m_globalTile_pointSet = false;
				PaintHeightfieldLocal(tx, ty, HeightPaintUpFunc);
			}
			else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
			{
				m_globalTile_pointSet = false;
				PaintHeightfieldLocal(tx, ty, HeightPaintDnFunc);
			}
			else if(event.ButtonUp(wxMOUSE_BTN_MIDDLE))
			{
				m_isLineMode = true;

				PaintHeightfieldLine(	m_globalTile_lineStart.x,m_globalTile_lineStart.y,
										m_globalTile_lineEnd.x, m_globalTile_lineEnd.y,
										HeightPaintUpFunc, HEDIT_LINEMODE_WIDTH/*GetEditMode() == HEDIT_SET ? HEDIT_LINEMODE_WIDTH : HEDIT_LINEMODE_RADIUS*/);

				m_isLineMode = false;
			}
		}
		else if(event.AltDown())
		{
			if( event.ButtonIsDown(wxMOUSE_BTN_LEFT) && m_selectedRegion )
			{
				// left mouse button used for picking
				EEditMode mode = GetEditMode();

				if(mode == HEDIT_ADD)
					SetHeight( 0 );
				else
					SetHeight(tile->height);

				SetRotation( tile->rotatetex );
				SetHeightfieldFlags(tile->flags);
				

				//SetRadius(1);

				if(tile->texture != -1)
				{
					IMaterial* mat = m_selectedRegion->GetHField(m_selectedHField)->m_materials[tile->texture]->material;

					m_texPanel->SelectMaterial( mat, tile->atlasIdx );
				}
				else
					m_texPanel->SelectMaterial(NULL, 0);

				m_texPanel->Redraw();
			}
		}
	}
	else
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			m_globalTile_pointSet = false;
			PaintHeightfieldLocal(tx, ty, TexPaintFunc);
		}
		else if(event.ButtonUp(wxMOUSE_BTN_MIDDLE))
		{
			m_isLineMode = true;

			// line
			PaintHeightfieldLine(	m_globalTile_lineStart.x,m_globalTile_lineStart.y,
									m_globalTile_lineEnd.x, m_globalTile_lineEnd.y, 
									TexPaintFunc, GetEditMode() == HEDIT_SET ? HEDIT_LINEMODE_WIDTH : HEDIT_LINEMODE_RADIUS);

			m_isLineMode = false;
		}
		else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
		{
			m_globalTile_pointSet = false;
			PaintHeightfieldLocal(tx, ty, NullTexPaintFunc);
		}
	}
}

void CUI_HeightEdit::OnKey(wxKeyEvent& event, bool bDown)
{
	// hotkeys
	if(!bDown)
	{
		if(event.GetRawKeyCode() == 'Q')
		{
			m_detached->SetValue(!m_detached->GetValue());
		}
		else if(event.GetRawKeyCode() == 'W')
		{
			m_addWall->SetValue(!m_addWall->GetValue());
		}
		else if(event.GetRawKeyCode() == 'E')
		{
			m_wallCollide->SetValue(!m_wallCollide->GetValue());
		}
		else if(event.GetRawKeyCode() == 'R')
		{
			m_noCollide->SetValue(!m_noCollide->GetValue());
		}
		if(event.m_keyCode == WXK_SPACE)
		{
			m_rotation += 1;

			if(m_rotation > 3)
				m_rotation = 0;
		}
	}
}

void CUI_HeightEdit::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	CBaseTilebasedEditor::OnRender();

	if(m_selectedRegion)
	{
		bool quadraticRadius = m_quadratic->GetValue();

		hfieldtile_t* tile = m_selectedRegion->GetHField(m_selectedHField)->GetTile(m_mouseOverTile.x, m_mouseOverTile.y);

		if(tile)
		{
			Vector3D box_pos(m_mouseOverTile.x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, m_mouseOverTile.y*HFIELD_POINT_SIZE);
			box_pos += m_selectedRegion->GetHField(m_selectedHField)->m_position - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

			debugoverlay->Text3D(box_pos, -1, ColorRGBA(1,1,1,1), 0.0f, "height: %d\nlayer: %d", tile->height, m_selectedHField);

			// draw circle
			if (!quadraticRadius)
			{
				ColorRGBA color2(0.2, 0.2, 0.2, 0.8);

				BlendStateParam_t blending;
				blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
				blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

				g_pShaderAPI->SetTexture(NULL, 0, 0);
				materials->SetBlendingStates(blending);
				materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
				materials->SetDepthStates(false, false);

				// draw a radius circle
				CMeshBuilder meshBuilder(materials->GetDynamicMesh());

				box_pos += Vector3D(0.5f, 0, 0.5f) * HFIELD_POINT_SIZE;

				meshBuilder.Begin(PRIM_LINE_STRIP);

				meshBuilder.Color4f(1.0f, 1.0f, 0.0f, 0.8f);
				for (int i = 0; i < 33; i++)
				{
					float angle = 360.0f*(float)i / 32.0f;

					float si, co;
					SinCos(DEG2RAD(angle), &si, &co);

					Vector3D circleAngleVec = Vector3D(si, 0, co)*GetRadius()*HFIELD_POINT_SIZE;

					circleAngleVec.y = HFIELD_HEIGHT_STEP;

					meshBuilder.Position3fv(box_pos + circleAngleVec);
					meshBuilder.AdvanceVertex();
				}


				meshBuilder.End();
			}

			PaintHeightfieldLocal(m_mouseOverTile.x, m_mouseOverTile.y, DrawHeightFieldTileFunc, 1.0f, HEDIT_PAINT_NO_HISTORY);
		}
	}
}

void CUI_HeightEdit::InitTool()
{
	ReloadMaterialList();
}

void CUI_HeightEdit::OnLevelUnload()
{
	m_texPanel->SelectMaterial(NULL,0);
	CBaseTilebasedEditor::OnLevelUnload();
}

void CUI_HeightEdit::Update_Refresh()
{
	m_texPanel->Redraw();
}