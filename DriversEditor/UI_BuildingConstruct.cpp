//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Building editor for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "UI_BuildingConstruct.h"
#include "EditorMain.h"
#include "FontCache.h"

CBuildingLayerEditDialog::~CBuildingLayerEditDialog()
{
	// Disconnect Events
	m_newBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
	m_delBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
	m_height->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeHeight ), NULL, this );
	m_repeat->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( CBuildingLayerEditDialog::ChangeRepeat ), NULL, this );
	m_interval->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( CBuildingLayerEditDialog::ChangeInterval ), NULL, this );
	m_typeSel->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeType ), NULL, this );
	m_btnChoose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
	m_previewBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
}

CBuildingLayerEditDialog::CBuildingLayerEditDialog( wxWindow* parent ) 
	: wxDialog( parent, wxID_ANY, wxT("Building floor layer set constructor - <filename>"), wxDefaultPosition,  wxSize( 756,558 ), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP )
{
	m_pFont = NULL;

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );
	
	m_renderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	//m_renderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );

	m_pSwapChain = materials->CreateSwapChain(m_renderPanel->GetHandle());
	
	bSizer18->Add( m_renderPanel, 1, wxEXPAND | wxALL, 5 );
	
	m_panel18 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer12;
	sbSizer12 = new wxStaticBoxSizer( new wxStaticBox( m_panel18, wxID_ANY, wxT("Layers") ), wxVERTICAL );
	
	m_newBtn = new wxButton( m_panel18, LAYEREDIT_NEW, wxT("New"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer12->Add( m_newBtn, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_delBtn = new wxButton( m_panel18, LAYEREDIT_DELETE, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer12->Add( m_delBtn, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer21->Add( sbSizer12, 0, wxEXPAND, 5 );
	
	m_propertyBox = new wxStaticBoxSizer( new wxStaticBox( m_panel18, wxID_ANY, wxT("Properties") ), wxVERTICAL );
	
	m_propertyBox->Add( new wxStaticText( m_panel18, wxID_ANY, wxT("Height"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxRIGHT|wxLEFT, 5 );
	
	m_height = new wxTextCtrl( m_panel18, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_propertyBox->Add( m_height, 0, wxRIGHT|wxLEFT, 5 );
	
	m_propertyBox->Add( new wxStaticText( m_panel18, wxID_ANY, wxT("Repeat / Interval"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxRIGHT|wxLEFT, 5 );
	
	wxGridSizer* gSizer4;
	gSizer4 = new wxGridSizer( 0, 2, 0, 0 );
	
	m_repeat = new wxSpinCtrl( m_panel18, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), wxSP_ARROW_KEYS, 0, 100, 0 );
	gSizer4->Add( m_repeat, 0, wxALL, 5 );
	
	m_interval = new wxSpinCtrl( m_panel18, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), wxSP_ARROW_KEYS, 0, 100, 0 );
	gSizer4->Add( m_interval, 0, wxALL, 5 );
	
	
	m_propertyBox->Add( gSizer4, 0, wxEXPAND, 5 );
	
	wxString m_typeSelChoices[] = { wxT("Model"), wxT("Texture") };
	int m_typeSelNChoices = sizeof( m_typeSelChoices ) / sizeof( wxString );
	m_typeSel = new wxChoice( m_panel18, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_typeSelNChoices, m_typeSelChoices, 0 );
	m_typeSel->SetSelection( 0 );
	m_propertyBox->Add( m_typeSel, 0, wxALL|wxEXPAND, 5 );
	
	m_btnChoose = new wxButton( m_panel18, LAYEREDIT_CHOOSEMODEL, wxT("Choose..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_propertyBox->Add( m_btnChoose, 0, wxALL, 5 );
	
	
	bSizer21->Add( m_propertyBox, 1, wxEXPAND, 5 );

	m_propertyBox->ShowItems(false);
	
	m_previewBtn = new wxButton( m_panel18, LAYEREDIT_TOGGLEPREVIEW, wxT("Toggle preview"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_previewBtn, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_panel18->SetSizer( bSizer21 );
	m_panel18->Layout();
	bSizer21->Fit( m_panel18 );
	bSizer18->Add( m_panel18, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer18 );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_newBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
	m_delBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
	m_height->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeHeight ), NULL, this );
	m_repeat->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( CBuildingLayerEditDialog::ChangeRepeat ), NULL, this );
	m_interval->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( CBuildingLayerEditDialog::ChangeInterval ), NULL, this );
	m_typeSel->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeType ), NULL, this );
	m_btnChoose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
	m_previewBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );

	m_renderPanel->Connect( wxEVT_IDLE, wxIdleEventHandler( CBuildingLayerEditDialog::OnIdle ), NULL, this );
	m_renderPanel->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CBuildingLayerEditDialog::OnEraseBackground ), NULL, this );

	m_renderPanel->Connect(wxEVT_MOTION, wxMouseEventHandler( CBuildingLayerEditDialog::OnMouseMotion ), NULL, this);
}

void CBuildingLayerEditDialog::OnMouseMotion(wxMouseEvent& event)
{
	m_mousePos.x = event.GetX();
	m_mousePos.y = event.GetY();

	Redraw();
}

void CBuildingLayerEditDialog::Redraw()
{
	if(!materials)
		return;

	if(!m_pFont)
		m_pFont = g_fontCache->GetFont("debug", 0);

	int w, h;
	m_renderPanel->GetSize(&w, &h);
	g_pShaderAPI->SetViewport(0, 0, w,h);

	materials->GetConfiguration().wireframeMode = false;
	materials->SetAmbientColor( color4_white );

	BlendStateParam_t blendParams;
	blendParams.alphaTest = false;
	blendParams.alphaTestRef = 1.0f;
	blendParams.blendEnable = false;
	blendParams.srcFactor = BLENDFACTOR_ONE;
	blendParams.dstFactor = BLENDFACTOR_ZERO;
	blendParams.mask = COLORMASK_ALL;
	blendParams.blendFunc = BLENDFUNC_ADD;

	eqFontStyleParam_t fontParam;
	fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
	fontParam.textColor = ColorRGBA(1,1,1,1);

	if( materials->BeginFrame() )
	{
		g_pShaderAPI->Clear(true, true, false, ColorRGBA(0,0,0,0));
		materials->Setup2D(w,h);

		int nLine = 0;
		int nItem = 0;

		Rectangle_t screenRect(0,0, w,h);
		screenRect.Fix();

		float scrollbarpercent = GetScrollPos(wxVERTICAL);

		int numItems = 0;

		const float fSize = 128.0f;

		for(int i = 0; i < m_layerColl.layers.numElem(); i++)
		{
			float x_offset = 16 + nItem*(fSize+16);

			if(x_offset + fSize > w)
			{
				numItems = i;
				break;
			}

			numItems++;
			nItem++;
		}

		if(numItems > 0)
		{
			nItem = 0;

			for(int i = 0; i < m_layerColl.layers.numElem(); i++)
			{
				buildLayer_t& elem = m_layerColl.layers[i];

				if(nItem >= numItems)
				{
					nItem = 0;
					nLine++;
				}

				float x_offset = 16 + nItem*(fSize+16);
				float y_offset = 8 + nLine*(fSize+48);

				y_offset -= scrollbarpercent*(fSize+48);

				Rectangle_t check_rect(x_offset, y_offset, x_offset + fSize,y_offset + fSize);
				check_rect.Fix();

				if( check_rect.vleftTop.y > screenRect.vrightBottom.y )
					break;

				if( !screenRect.IsIntersectsRectangle(check_rect) )
				{
					nItem++;
					continue;
				}

				float texture_aspect = 1.0f;
				float x_scale = 1.0f;
				float y_scale = 1.0f;

				ITexture* pTex = g_pShaderAPI->GetErrorTexture();

				if(elem.type == LAYER_TEXTURE)
				{
					if(elem.material && !elem.material->IsError())
					{
						// preload
						materials->PutMaterialToLoadingQueue( elem.material );

						if(elem.material->GetState() == MATERIAL_LOAD_OK && elem.material->GetBaseTexture())
							pTex = elem.material->GetBaseTexture();
					}
				}
				else
				{
					// TODO: model snapshot by it's container
				}

				texture_aspect = pTex->GetWidth() / pTex->GetHeight();

				if(pTex->GetWidth() > pTex->GetHeight())
					y_scale /= texture_aspect;

				Rectangle_t name_rect(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 400);

				Vertex2D_t verts[] = {MAKETEXQUAD(x_offset, y_offset, x_offset + fSize*x_scale,y_offset + fSize*y_scale, 0)};
				Vertex2D_t name_line[] = {MAKETEXQUAD(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 25, 0)};

				if(elem.type == LAYER_TEXTURE && elem.atlEntry)
				{
					verts[0].m_vTexCoord = elem.atlEntry->rect.GetLeftTop();
					verts[1].m_vTexCoord = elem.atlEntry->rect.GetLeftBottom();
					verts[2].m_vTexCoord = elem.atlEntry->rect.GetRightTop();
					verts[3].m_vTexCoord = elem.atlEntry->rect.GetRightBottom();
				}

				// draw name panel
				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25,0.25,1,1), &blendParams);

				// mouseover rectangle
				if( check_rect.IsInRectangle( m_mousePos ) )
				{
					m_mouseoverItem = i;

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, ColorRGBA(1,0.5f,0.5f,1), &blendParams);

					Vertex2D rectVerts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};

					materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(1,0,0,1));
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25,0.1,0.25,1), &blendParams);
				}
				else
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, color4_white, &blendParams);

				// draw selection rectangle
				if(m_selectedItem == i)
				{
					Vertex2D rectVerts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};

					materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(0,1,0,1));
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(1,0.25,0.25,1), &blendParams);
				}

				// render layer name text
				m_pFont->RenderText("pls sel model/tex", name_rect.vleftTop, fontParam);

				nItem++;
			}
		}

		materials->EndFrame(m_pSwapChain);
	}
}

void CBuildingLayerEditDialog::OnBtnsClick( wxCommandEvent& event )
{
	switch(event.GetId())
	{
		case LAYEREDIT_NEW:
		{
			m_selectedItem = m_layerColl.layers.append( buildLayer_t() );

			UpdateSelection();

			break;
		}
		case LAYEREDIT_DELETE:
		{
			break;
		}
		case LAYEREDIT_CHOOSEMODEL:
		{
			break;
		}
		case LAYEREDIT_TOGGLEPREVIEW:
		{
			break;
		}
	}
}

void CBuildingLayerEditDialog::UpdateSelection()
{
	if(m_selectedItem == -1)
	{
		m_selLayer = NULL;
		m_propertyBox->ShowItems(false);
		Layout();
		return;
	}

	m_selLayer = &m_layerColl.layers[m_selectedItem];

	m_propertyBox->ShowItems(true);
	Layout();

	m_height->SetValue( varargs("%g", m_selLayer->height) );
	m_repeat->SetValue( m_selLayer->repeatTimes );
	m_interval->SetValue( m_selLayer->repeatInterval );
	m_typeSel->SetSelection( m_selLayer->type );
}

void CBuildingLayerEditDialog::ChangeHeight( wxCommandEvent& event )
{
	if(m_selLayer) m_selLayer->height = atoi(m_height->GetValue());
}

void CBuildingLayerEditDialog::ChangeRepeat( wxSpinEvent& event )
{
	if(m_selLayer) m_selLayer->repeatTimes = m_repeat->GetValue();
}

void CBuildingLayerEditDialog::ChangeInterval( wxSpinEvent& event )
{
	if(m_selLayer) m_selLayer->repeatInterval = m_interval->GetValue();
}

void CBuildingLayerEditDialog::ChangeType( wxCommandEvent& event )
{
	if(m_selLayer) m_selLayer->type = m_typeSel->GetSelection();
}

//------------------------------------------------------------------------
// the editor panel
//-----------------------------------------------------------------------------

CUI_BuildingConstruct::CUI_BuildingConstruct( wxWindow* parent )
	 : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize), CBaseTilebasedEditor()
{
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_modelPicker = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_modelPicker->SetForegroundColour( wxColour( 0, 0, 0 ) );
	m_modelPicker->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	
	bSizer9->Add( m_modelPicker, 1, wxEXPAND|wxALL, 5 );
	
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
	
	m_filtertext = new wxTextCtrl( m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), 0 );
	m_filtertext->SetMaxLength( 0 ); 
	fgSizer1->Add( m_filtertext, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer2->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	bSizer10->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 0, 3, 0, 0 );
	
	
	bSizer10->Add( gSizer2, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer20;
	sbSizer20 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Sets") ), wxVERTICAL );
	
	m_button5 = new wxButton( m_pSettingsPanel, wxID_ANY, wxT("Create..."), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_button5, 0, wxALL, 5 );
	
	m_button3 = new wxButton( m_pSettingsPanel, wxID_ANY, wxT("Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_button3, 0, wxALL, 5 );
	
	m_button8 = new wxButton( m_pSettingsPanel, wxID_ANY, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_button8, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizer20, 1, wxEXPAND, 5 );
	
	
	m_pSettingsPanel->SetSizer( bSizer10 );
	m_pSettingsPanel->Layout();
	bSizer9->Add( m_pSettingsPanel, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer9 );
	this->Layout();
	
	// Connect Events
	m_filtertext->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CUI_BuildingConstruct::OnFilterChange ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_BuildingConstruct::OnCreateClick ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_BuildingConstruct::OnEditClick ), NULL, this );
	m_button8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_BuildingConstruct::OnDeleteClick ), NULL, this );

	m_layerEditDlg = new CBuildingLayerEditDialog(g_pMainFrame);
}

CUI_BuildingConstruct::~CUI_BuildingConstruct()
{
}

// Virtual event handlers, overide them in your derived class
void CUI_BuildingConstruct::OnFilterChange( wxCommandEvent& event )
{

}

void CUI_BuildingConstruct::OnCreateClick( wxCommandEvent& event )
{
	// TODO: newLayer
	m_layerEditDlg->ShowModal();
}

void CUI_BuildingConstruct::OnEditClick( wxCommandEvent& event )
{
	// TODO: set existing layers
	m_layerEditDlg->ShowModal();
}

void CUI_BuildingConstruct::OnDeleteClick( wxCommandEvent& event )
{

}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::InitTool()
{

}

void CUI_BuildingConstruct::ShutdownTool()
{

}

void CUI_BuildingConstruct::Update_Refresh()
{

}

void CUI_BuildingConstruct::OnKey(wxKeyEvent& event, bool bDown)
{

}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	
}

void CUI_BuildingConstruct::OnRender()
{
	
}