//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Building editor for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "UI_BuildingConstruct.h"
#include "UI_HeightEdit.h"

#include "EditorMain.h"
#include "FontCache.h"
#include "FontLayoutBuilders.h"

#define PREVIEW_BOX_SIZE 256

CLayerModel::CLayerModel() : m_model(NULL)
{
}

CLayerModel::~CLayerModel()
{
	delete m_model;
}

void CLayerModel::RefreshPreview()
{
	if(!m_dirtyPreview)
		return;

	m_dirtyPreview = false;

	if(!CreatePreview( PREVIEW_BOX_SIZE ))
		return;

	static ITexture* pTempRendertarget = g_pShaderAPI->CreateRenderTarget(PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, FORMAT_RGBA8);

	Vector3D	camera_rotation(35,225,0);
	Vector3D	camera_target(0);
	float		camDistance = 0.0f;
	
	BoundingBox bbox;

	bbox = m_model->m_bbox;

	camDistance = length(m_model->m_bbox.GetSize())+0.5f;
	camera_target = m_model->m_bbox.GetCenter();

	Vector3D forward, right;
	AngleVectors(camera_rotation, &forward, &right);

	Vector3D cam_pos = camera_target - forward*camDistance;

	// setup perspective
	Matrix4x4 mProjMat = perspectiveMatrixY(DEG2RAD(60), PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE, 0.05f, 512.0f);

	Matrix4x4 mViewMat = rotateZXY4(DEG2RAD(-camera_rotation.x),DEG2RAD(-camera_rotation.y),DEG2RAD(-camera_rotation.z));
	mViewMat.translate(-cam_pos);

	materials->SetMatrix(MATRIXMODE_PROJECTION, mProjMat);
	materials->SetMatrix(MATRIXMODE_VIEW, mViewMat);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->SetFogInfo(FogInfo_t());

	// setup render
	g_pShaderAPI->ChangeRenderTarget( pTempRendertarget );

	g_pShaderAPI->Clear(true, true, false, ColorRGBA(0.25,0.25,0.25,1.0f));

	m_model->PreloadTextures();
	materials->Wait();
	m_model->Render(0, bbox);

	//Render(0.0f, bbox, true, 0);

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// copy rendertarget
	UTIL_CopyRendertargetToTexture(pTempRendertarget, m_preview);
}

//---------------------------------------------------------------------------------------

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
	: wxDialog( parent, wxID_ANY, wxT("Building floor layer set constructor"), wxDefaultPosition,  wxSize( 756,558 ), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP )
{
	m_pFont = NULL;

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );
	
	m_renderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	//m_renderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );

	m_renderPanel->SetDropTarget(this);

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
	
	wxString m_typeSelChoices[] = { wxT("Texture"), wxT("Model") };
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
	m_renderPanel->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(CBuildingLayerEditDialog::OnMouseScroll), NULL, this);
	m_renderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CBuildingLayerEditDialog::OnMouseClick), NULL, this);
	m_renderPanel->Connect(wxEVT_SCROLLBAR, wxScrollWinEventHandler(CBuildingLayerEditDialog::OnScrollbarChange), NULL, this);

	m_preview = false;
	m_layerColl = NULL;
}

void CBuildingLayerEditDialog::OnScrollbarChange(wxScrollWinEvent& event)
{
	if(event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_TOP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else
		SetScrollPos(wxVERTICAL, event.GetPosition(), true);

	Redraw();
}

void CBuildingLayerEditDialog::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos =  GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation()/100, true);
}

void CBuildingLayerEditDialog::OnMouseClick(wxMouseEvent& event)
{
	if(m_preview)
	{
	
	}
	else
	{
		// set selection to mouse over
		m_selectedItem = m_mouseoverItem;

		UpdateSelection();
	}
}

void CBuildingLayerEditDialog::OnMouseMotion(wxMouseEvent& event)
{
	m_mousePos.x = event.GetX();
	m_mousePos.y = event.GetY();

	Redraw();
}

bool CBuildingLayerEditDialog::OnDropPoiner(wxCoord x, wxCoord y, void* ptr, EDragDropPointerType type)
{
	if(!m_selLayer)
		return false;

	if(type == DRAGDROP_PTR_COMPOSITE_MATERIAL)
	{
		matAtlasElem_t* elem = (matAtlasElem_t*)ptr;

		m_selLayer->type = LAYER_TEXTURE;
		m_selLayer->material = elem->material;
		m_selLayer->atlEntry = elem->entry;
	}
	else if(type == DRAGDROP_PTR_OBJECTCONTAINER)
	{
	
	}


	return true;
}

void CBuildingLayerEditDialog::Redraw()
{
	if(m_preview)
		RenderPreview();
	else
		RenderList();
}

void CBuildingLayerEditDialog::RenderPreview()
{
	if(!materials)
		return;

	int w, h;
	m_renderPanel->GetSize(&w, &h);
	g_pShaderAPI->SetViewport(0, 0, w,h);

	materials->GetConfiguration().wireframeMode = false;
	materials->SetAmbientColor( color4_white );
}

void CBuildingLayerEditDialog::RenderList()
{
	if(!m_layerColl)
		return;

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

	CRectangleTextLayoutBuilder rectLayout;

	eqFontStyleParam_t fontParam;
	fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
	fontParam.textColor = ColorRGBA(1,1,1,1);
	fontParam.layoutBuilder = &rectLayout;

	m_mouseoverItem = -1;

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

		for(int i = 0; i < m_layerColl->layers.numElem(); i++)
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

			for(int i = 0; i < m_layerColl->layers.numElem(); i++)
			{
				buildLayer_t& elem = m_layerColl->layers[i];

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
					if(elem.model)
						pTex = elem.model->GetPreview();
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
				rectLayout.SetRectangle(name_rect);

				if(elem.type == LAYER_TEXTURE && elem.material)
				{
					EqString material_name = elem.material->GetName();
					material_name.Replace( CORRECT_PATH_SEPARATOR, '\n' );

					m_pFont->RenderText(material_name.c_str(), name_rect.vleftTop, fontParam);
				}
				else if(elem.type == LAYER_MODEL && elem.model)
				{
					m_pFont->RenderText(elem.model->m_name.c_str(), name_rect.vleftTop, fontParam);
				}
				else
					m_pFont->RenderText("drag&drop material or model", name_rect.vleftTop, fontParam);

				//else if(elem.type == LAYER_MODEL)
				//	m_pFont->RenderText("pls sel model", name_rect.vleftTop, fontParam);

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
			m_selectedItem = m_layerColl->layers.append( buildLayer_t() );

			UpdateSelection();

			break;
		}
		case LAYEREDIT_DELETE:
		{
			if(m_selectedItem >= 0)
			{
				if(m_selLayer->type >= LAYER_MODEL)
					delete m_selLayer->model;

				m_layerColl->layers.removeIndex(m_selectedItem);
				m_selectedItem--;
			}
			
			UpdateSelection();
			break;
		}
		case LAYEREDIT_CHOOSEMODEL:
		{
			if(!m_selLayer)
			{
				wxMessageBox("First you need to select layer!");
				return;
			}

			wxFileDialog* file = new wxFileDialog(NULL, "Open OBJ/ESM", "./", "*.*", "Model files (*.obj, *.esm)|*.obj;*.esm", wxFD_FILE_MUST_EXIST | wxFD_OPEN);

			Hide();

			if(file->ShowModal() == wxID_OK)
			{
				wxArrayString paths;
				EqString path( file->GetPath().wchar_str() );

				dsmmodel_t model;

				if( !LoadSharedModel(&model, path.c_str()) )
				{
					Show();
					return;
				}
					
				CLevelModel* pLevModel = new CLevelModel();
				pLevModel->CreateFrom( &model );
				FreeDSM(&model);

				CLayerModel* lmodel = m_selLayer->model;

				if(!lmodel)
					lmodel = new CLayerModel();
				else
					delete lmodel->m_model;

				lmodel->SetDirtyPreview();

				lmodel->m_model = pLevModel;
				lmodel->m_name = path.Path_Extract_Name().Path_Strip_Ext().c_str();

				m_selLayer->model = lmodel;

				lmodel->RefreshPreview();
			}

			Show();

			break;
		}
		case LAYEREDIT_TOGGLEPREVIEW:
		{
			m_preview = !m_preview;
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

	m_selLayer = &m_layerColl->layers[m_selectedItem];

	m_propertyBox->ShowItems(true);
	Layout();

	m_height->SetValue( varargs("%g", m_selLayer->height) );
	m_repeat->SetValue( m_selLayer->repeatTimes );
	m_interval->SetValue( m_selLayer->repeatInterval );
	m_typeSel->SetSelection( m_selLayer->type );
	m_btnChoose->Enable( m_selLayer->type >= LAYER_MODEL );
}

void CBuildingLayerEditDialog::SetLayerCollection(buildLayerColl_t* coll)
{
	m_layerColl = coll;
}

buildLayerColl_t* CBuildingLayerEditDialog::GetLayerCollection() const
{
	return m_layerColl;
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
	if(m_selLayer)
	{
		if(m_selLayer->type == m_typeSel->GetSelection())
			return;

		if(m_selLayer->type >= LAYER_MODEL)
		{
			delete m_selLayer->model;
			m_selLayer->model = NULL;
		}

		m_selLayer->type = m_typeSel->GetSelection();

		m_btnChoose->Enable( m_selLayer->type >= LAYER_MODEL );
	}
}
//-----------------------------------------------------------------------------
// Layer collection list
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CBuildingLayerList, wxPanel)
    EVT_ERASE_BACKGROUND(CBuildingLayerList::OnEraseBackground)
    EVT_IDLE(CBuildingLayerList::OnIdle)
	EVT_SCROLLWIN(CBuildingLayerList::OnScrollbarChange)
	EVT_MOTION(CBuildingLayerList::OnMouseMotion)
	EVT_MOUSEWHEEL(CBuildingLayerList::OnMouseScroll)
	EVT_LEFT_DOWN(CBuildingLayerList::OnMouseClick)
	EVT_SIZE(CBuildingLayerList::OnSizeEvent)
END_EVENT_TABLE()

CBuildingLayerList::CBuildingLayerList(CUI_BuildingConstruct* parent) : wxPanel( parent, 0,0,640,480 )
{
	m_swapChain = NULL;

	SetScrollbar(wxVERTICAL, 0, 8, 100);

	m_nPreviewSize = 128;
	m_bldConstruct = parent;
}

void CBuildingLayerList::OnSizeEvent(wxSizeEvent &event)
{
	if(!IsShown())
		return;

	if(materials && g_pMainFrame)
	{
		int w, h;
		g_pMainFrame->GetSize(&w, &h);

		RefreshScrollbar();

		Redraw();
	}
}

void CBuildingLayerList::OnIdle(wxIdleEvent &event)
{
	Redraw();
}

void CBuildingLayerList::OnEraseBackground(wxEraseEvent& event)
{
}

void CBuildingLayerList::OnScrollbarChange(wxScrollWinEvent& event)
{
	if(event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_TOP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if(event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else
		SetScrollPos(wxVERTICAL, event.GetPosition(), true);

	Redraw();
}
		
void CBuildingLayerList::OnMouseMotion(wxMouseEvent& event)
{
	m_mousePos.x = event.GetX();
	m_mousePos.y = event.GetY();

	Redraw();
}

void CBuildingLayerList::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos =  GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation()/100, true);
}

void CBuildingLayerList::OnMouseClick(wxMouseEvent& event)
{
	// set selection to mouse over
	m_selection = m_mouseOver;
}

void CBuildingLayerList::ReloadList()
{

}

buildLayerColl_t* CBuildingLayerList::GetSelectedLayerColl() const
{
	return NULL;
}

void CBuildingLayerList::ChangeFilter(const wxString& filter)
{
	if(!IsShown())
		return;

	if(m_filter == filter)
		return;

	// remember filter string
	m_filter = filter;

	UpdateAndFilterList();
}

void CBuildingLayerList::UpdateAndFilterList()
{
	m_selection = -1;

	m_filteredList.clear();

	for(int i = 0; i < m_layerCollections.numElem(); i++)
	{
		// filter material path
		if( m_filter.Length() > 0 )
		{
			int foundIndex = m_layerCollections[i]->name.Find(m_filter.c_str());

			if(foundIndex == -1)
				continue;
		}

		// add just material
		m_filteredList.append( m_layerCollections[i] );
	}

	RefreshScrollbar();
	Redraw();
}

void CBuildingLayerList::SetPreviewParams(int preview_size, bool bAspectFix)
{
	if(m_nPreviewSize != preview_size)
		RefreshScrollbar();

	m_nPreviewSize = preview_size;
}

void CBuildingLayerList::RefreshScrollbar()
{
	int w, h;
	GetSize(&w, &h);

	wxRect rect = GetScreenRect();
	w = rect.GetWidth();
	h = rect.GetHeight();

	int numItems = 0;
	int nItem = 0;

	float fSize = (float)m_nPreviewSize;

	for(int i = 0; i < m_filteredList.numElem(); i++)
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
		int estimated_lines = m_filteredList.numElem() / numItems;

		SetScrollbar(wxVERTICAL, 0, 8, estimated_lines + 10);
	}
}

void CBuildingLayerList::Redraw()
{
	if(!materials)
		return;

	if(!m_swapChain)
		m_swapChain = materials->CreateSwapChain((HWND)GetHWND());

	if(!IsShown() && !IsShownOnScreen())
		return;

	int w, h;
	GetClientSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w, h);

	if( materials->BeginFrame() )
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0,0,0, 1));

		if(!m_layerCollections.numElem())
		{
			materials->EndFrame(m_swapChain);
			return;
		}

		float scrollbarpercent = GetScrollPos(wxVERTICAL);

		IRectangle screenRect(0,0, w,h);
		screenRect.Fix();

		RedrawItems(screenRect, scrollbarpercent, m_nPreviewSize);

		materials->EndFrame(m_swapChain);
	}
}

Rectangle_t CBuildingLayerList::ItemGetImageCoordinates( buildLayerColl_t*& item )
{
	return Rectangle_t(0,0,1,1);
}

ITexture* CBuildingLayerList::ItemGetImage( buildLayerColl_t*& item )
{
	return g_pShaderAPI->GetErrorTexture();
}

void CBuildingLayerList::ItemPostRender( int id, buildLayerColl_t*& item, const IRectangle& rect )
{
	Rectangle_t name_rect(rect.GetLeftBottom(), rect.GetRightBottom() + Vector2D(0,25));
	name_rect.Fix();

	Vector2D lt = name_rect.GetLeftTop();
	Vector2D rb = name_rect.GetRightBottom();

	Vertex2D_t name_line[] = {MAKETEXQUAD(lt.x, lt.y, rb.x, rb.y, 0)};

	ColorRGBA nameBackCol = ColorRGBA(0.25,0.25,1,1);

	if(m_selection == id)
		nameBackCol = ColorRGBA(1,0.25,0.25,1);
	else if(m_mouseOver == id)
		nameBackCol = ColorRGBA(0.25,0.1,0.25,1);

	// draw name panel
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, nameBackCol);

	eqFontStyleParam_t fontParam;
	fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
	fontParam.textColor = ColorRGBA(1,1,1,1);

	// render text
	m_debugFont->RenderText(item->name.c_str(), name_rect.vleftTop, fontParam);
}

buildLayerColl_t* CBuildingLayerList::CreateCollection()
{
	buildLayerColl_t* newColl = new buildLayerColl_t();
	m_layerCollections.append(newColl);

	UpdateAndFilterList();

	return newColl;
}

void CBuildingLayerList::DeleteCollection(buildLayerColl_t* coll)
{
	if(m_layerCollections.remove(coll))
		delete coll;

	UpdateAndFilterList();
}

//-----------------------------------------------------------------------------
// the editor panel
//-----------------------------------------------------------------------------

CUI_BuildingConstruct::CUI_BuildingConstruct( wxWindow* parent )
	 : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER | wxSTAY_ON_TOP), CBaseTilebasedEditor()
{
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_layerCollList = new CBuildingLayerList( this );
	
	bSizer9->Add( m_layerCollList, 1, wxEXPAND|wxALL, 5 );
	
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
	m_layerCollList->ChangeFilter(m_filtertext->GetValue());
}

void CUI_BuildingConstruct::OnCreateClick( wxCommandEvent& event )
{
	wxTextEntryDialog* dlg = new wxTextEntryDialog(this, "Please enter name of layer set", "Enter the name");

	if(dlg->ShowModal() == wxID_OK)
	{
		buildLayerColl_t* newLayerColl = m_layerCollList->CreateCollection();
		newLayerColl->name = dlg->GetValue();
		
		m_layerEditDlg->SetLayerCollection(newLayerColl);
		m_layerEditDlg->Show();
	}

	delete dlg;
}

void CUI_BuildingConstruct::OnEditClick( wxCommandEvent& event )
{
	buildLayerColl_t* layerColl = m_layerCollList->GetSelectedLayerColl();

	if(!layerColl)
		return;

	m_layerEditDlg->SetLayerCollection( layerColl );

	m_layerEditDlg->Show();
}

void CUI_BuildingConstruct::OnDeleteClick( wxCommandEvent& event )
{
	
}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::InitTool()
{
	//m_layerCollList->LoadLayerCollections();
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