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

#include "world.h"

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
	m_size->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeSize ), NULL, this );
	m_typeSel->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeType ), NULL, this );
	m_btnChoose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );
}

CBuildingLayerEditDialog::CBuildingLayerEditDialog( wxWindow* parent ) 
	: wxDialog( parent, wxID_ANY, wxT("Building floor layer set constructor"), wxDefaultPosition,  wxSize( 756,558 ), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP )
{
	m_pFont = NULL;

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );
	
	m_renderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	// THIS CRASHES ON EXIT
	//m_renderPanel->SetDropTarget(this);

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
	
	m_size = new wxSpinCtrl( m_panel18, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), wxSP_ARROW_KEYS, 1, 128, 0 );
	m_propertyBox->Add( m_size, 0, wxRIGHT|wxLEFT, 5 );
	
	wxString m_typeSelChoices[] = { wxT("Texture"), wxT("Model") };
	int m_typeSelNChoices = sizeof( m_typeSelChoices ) / sizeof( wxString );
	m_typeSel = new wxChoice( m_panel18, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_typeSelNChoices, m_typeSelChoices, 0 );
	m_typeSel->SetSelection( 0 );
	m_propertyBox->Add( m_typeSel, 0, wxALL|wxEXPAND, 5 );
	
	m_btnChoose = new wxButton( m_panel18, LAYEREDIT_CHOOSEMODEL, wxT("Choose..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_propertyBox->Add( m_btnChoose, 0, wxALL, 5 );
	
	
	bSizer21->Add( m_propertyBox, 1, wxEXPAND, 5 );

	m_propertyBox->ShowItems(false);
	
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
	m_size->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeSize ), NULL, this );
	m_typeSel->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CBuildingLayerEditDialog::ChangeType ), NULL, this );
	m_btnChoose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CBuildingLayerEditDialog::OnBtnsClick ), NULL, this );

	m_renderPanel->Connect( wxEVT_IDLE, wxIdleEventHandler( CBuildingLayerEditDialog::OnIdle ), NULL, this );
	m_renderPanel->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CBuildingLayerEditDialog::OnEraseBackground ), NULL, this );

	m_renderPanel->Connect(wxEVT_MOTION, wxMouseEventHandler( CBuildingLayerEditDialog::OnMouseMotion ), NULL, this);
	m_renderPanel->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(CBuildingLayerEditDialog::OnMouseScroll), NULL, this);
	m_renderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CBuildingLayerEditDialog::OnMouseClick), NULL, this);
	m_renderPanel->Connect(wxEVT_SCROLLBAR, wxScrollWinEventHandler(CBuildingLayerEditDialog::OnScrollbarChange), NULL, this);

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
	// set selection to mouse over
	m_selectedItem = m_mouseoverItem;

	UpdateSelection();
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

		m_selLayer->type = BUILDLAYER_TEXTURE;
		m_selLayer->material = elem->material;
	}
	else if(type == DRAGDROP_PTR_OBJECTCONTAINER)
	{
	
	}

	return true;
}

void CBuildingLayerEditDialog::Redraw()
{
	RenderList();
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

				if(elem.type == BUILDLAYER_TEXTURE)
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

				/*
				if(elem.type == BUILDLAYER_TEXTURE && elem.atlEntry)
				{
					verts[0].m_vTexCoord = elem.atlEntry->rect.GetLeftTop();
					verts[1].m_vTexCoord = elem.atlEntry->rect.GetLeftBottom();
					verts[2].m_vTexCoord = elem.atlEntry->rect.GetRightTop();
					verts[3].m_vTexCoord = elem.atlEntry->rect.GetRightBottom();
				}
				*/

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

				if(elem.type == BUILDLAYER_TEXTURE && elem.material)
				{
					EqString material_name = elem.material->GetName();
					material_name.Replace( CORRECT_PATH_SEPARATOR, '\n' );

					m_pFont->RenderText(material_name.c_str(), name_rect.vleftTop, fontParam);
				}
				else if(elem.type == BUILDLAYER_MODEL && elem.model)
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
			g_pMainFrame->NotifyUpdate();

			UpdateSelection();

			break;
		}
		case LAYEREDIT_DELETE:
		{
			if(m_selectedItem >= 0)
			{
				if(m_selLayer->type >= BUILDLAYER_MODEL)
					delete m_selLayer->model;

				m_layerColl->layers.removeIndex(m_selectedItem);
				m_selectedItem--;

				g_pMainFrame->NotifyUpdate();
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

				g_pMainFrame->NotifyUpdate();
			}

			Show();

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

	m_size->SetValue( varargs("%g", m_selLayer->size) );
	m_typeSel->SetSelection( m_selLayer->type );
	m_btnChoose->Enable( m_selLayer->type >= BUILDLAYER_MODEL );
}

void CBuildingLayerEditDialog::SetLayerCollection(buildLayerColl_t* coll)
{
	m_layerColl = coll;
}

buildLayerColl_t* CBuildingLayerEditDialog::GetLayerCollection() const
{
	return m_layerColl;
}

void CBuildingLayerEditDialog::ChangeSize( wxCommandEvent& event )
{
	if(m_selLayer) m_selLayer->size = m_size->GetValue();
}

void CBuildingLayerEditDialog::ChangeType( wxCommandEvent& event )
{
	if(m_selLayer)
	{
		if(m_selLayer->type == m_typeSel->GetSelection())
			return;

		if(m_selLayer->type >= BUILDLAYER_MODEL)
		{
			delete m_selLayer->model;
			m_selLayer->model = NULL;
		}

		m_selLayer->type = m_typeSel->GetSelection();

		m_btnChoose->Enable( m_selLayer->type >= BUILDLAYER_MODEL );
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
	if(m_selection == -1)
		return NULL;

	return m_filteredList[m_selection];
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

	g_pMainFrame->NotifyUpdate();

	return newColl;
}

void CBuildingLayerList::DeleteCollection(buildLayerColl_t* coll)
{
	if(m_layerCollections.remove(coll))
		delete coll;

	UpdateAndFilterList();

	g_pMainFrame->NotifyUpdate();
}

void CBuildingLayerList::LoadLayerCollections( const char* levelName )
{
	EqString folderPath = varargs("levels/%s_editor",levelName);

	KeyValues kvDefs;
	if(!kvDefs.LoadFromFile(_Es(folderPath + "/buildingTemplates.def").c_str()))
		return;

	// load model files
	IFile* pFile = g_fileSystem->Open(_Es(folderPath + "/buildingTemplateModels.dat").c_str(), "rb", SP_MOD);

	if(!pFile)
	{
		ErrorMsg("Failed to open buildingTemplateModels.dat !!!");
		return;
	}

	int numLayerCollections = 0;
	pFile->Read(&numLayerCollections, 1, sizeof(int));

	for(int i = 0; i < kvDefs.GetRootSection()->keys.numElem(); i++)
	{
		if(stricmp(kvDefs.GetRootSection()->keys[i]->name, "template"))
			continue;

		kvkeybase_t* templateSec = kvDefs.GetRootSection()->keys[i];

		buildLayerColl_t* coll = new buildLayerColl_t();
		m_layerCollections.append(coll);

		coll->name = KV_GetValueString(templateSec, 0, "unnamed");

		coll->Load(pFile, templateSec);
	}

	UpdateAndFilterList();

	g_fileSystem->Close( pFile );
}

void CBuildingLayerList::SaveLayerCollections( const char* levelName )
{
	EqString folderPath = varargs("levels/%s_editor",levelName);

	// make folder <levelName>_editor and put this stuff there
	g_fileSystem->MakeDir(folderPath.c_str(), SP_MOD);

	// keyvalues file buildings.def
	KeyValues kvDefs;

	// save model files
	IFile* pFile = g_fileSystem->Open(_Es(folderPath + "/buildingTemplateModels.dat").c_str(), "wb", SP_MOD);

	int numLayerCollections = m_layerCollections.numElem();
	pFile->Write(&numLayerCollections, 1, sizeof(int));

	for(int i = 0; i < m_layerCollections.numElem(); i++)
	{
		buildLayerColl_t* coll = m_layerCollections[i];

		kvkeybase_t* layerCollData = kvDefs.GetRootSection()->AddKeyBase("template", coll->name.c_str());

		coll->Save( pFile, layerCollData );
	}

	kvDefs.SaveToFile(_Es(folderPath + "/buildingTemplates.def").c_str());

	g_fileSystem->Close( pFile );
}

void CBuildingLayerList::RemoveAllLayerCollections()
{
	for(int i = 0; i < m_layerCollections.numElem(); i++)
	{
		delete m_layerCollections[i];
	}

	m_layerCollections.clear();

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

	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Properties") ), wxVERTICAL );

	m_tiledPlacement = new wxCheckBox( m_pSettingsPanel, wxID_ANY, wxT("Tiled placement (T)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tiledPlacement->SetValue(false); 
	sbSizer6->Add( m_tiledPlacement, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizer6, 0, wxEXPAND, 5 );
	
	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 0, 3, 0, 0 );

	bSizer10->Add( gSizer2, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer20 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Sets") ), wxVERTICAL );
	
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
	m_placeError = false;

	m_curLayerId = 0;
	m_curSegmentScale = 1.0f;
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
	buildLayerColl_t* layerColl = m_layerCollList->GetSelectedLayerColl();
	
	if(m_newBuilding.layerColl == layerColl)
		m_newBuilding.layerColl = NULL;

	m_layerCollList->DeleteCollection(layerColl);

}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::Update_Refresh()
{

}

void CUI_BuildingConstruct::OnKey(wxKeyEvent& event, bool bDown)
{
	// hotkeys
	if(!bDown)
	{
		if(event.m_keyCode == WXK_ESCAPE)
		{
			CancelBuilding();
			ClearSelection();
		}
		else if(event.m_keyCode == WXK_DELETE)
		{
			DeleteSelection();
		}
		else if(event.m_keyCode == WXK_SPACE)
		{
			m_newBuilding.order = m_newBuilding.order > 0 ? -1 : 1;
		}
		else if(event.m_keyCode == WXK_RETURN)
		{
			CompleteBuilding();
		}
		else if(event.GetRawKeyCode() == 'T')
		{
			m_tiledPlacement->SetValue(!m_tiledPlacement->GetValue());
		}
	}
}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::InitTool()
{

}

void CUI_BuildingConstruct::OnLevelLoad()
{
	CBaseTilebasedEditor::OnLevelLoad();
	m_layerCollList->LoadLayerCollections( g_pGameWorld->GetLevelName() );

	// assign layers on buildings
	g_pGameWorld->m_level.PostLoadEditorBuildings( m_layerCollList->m_layerCollections );

	m_mode = ED_BUILD_READY;
}

void CUI_BuildingConstruct::OnLevelSave()
{
	CBaseTilebasedEditor::OnLevelSave();
	m_layerCollList->SaveLayerCollections( g_pGameWorld->GetLevelName() );

	// save each region of level
	
}

void CUI_BuildingConstruct::OnLevelUnload()
{
	ClearSelection();
	CancelBuilding();

	CBaseTilebasedEditor::OnLevelUnload();

	m_layerCollList->RemoveAllLayerCollections();
}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::ProcessMouseEvents( wxMouseEvent& event )
{
	if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT) && event.Dragging())
	{
		float delta = m_mouseLastY - event.GetY();
		
		// make scale
		m_curSegmentScale += delta*0.001f;
		m_curSegmentScale = clamp(m_curSegmentScale,0.5f, 2.0f);
	}
	else
		CBaseTilebasedEditor::ProcessMouseEvents(event);

	m_mouseLastY = event.GetY();
}


/*
	1. move to EditorLevel.h and EditorLevel.cpp:

		buildingData_t
		
			buildLayerColl_t
				buildLayer_t

			buildSegmentPoint_t

	2. Save all building sources per region in regionBuildings.dat

	3. buildingSources must be converted to CLevelModel using only Editor.
		CLevelModel must have reference to buildingSource so it can be edited
		Building model must be regenerated on the fly when it's dirty and completed editing

	4. Generated building models are stored in region models data block

	5. Make per-region model list so we can load models in specified regions
		Generated models are not visible it the list, so they can't be reused.

*/


void CUI_BuildingConstruct::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	if(event.GetWheelRotation() != 0)
	{
		int wheelSign = sign(event.GetWheelRotation());
		m_curLayerId += wheelSign;
	}

	if(m_newBuilding.layerColl != NULL)
	{
		if(m_curLayerId >= m_newBuilding.layerColl->layers.numElem())
			m_curLayerId = m_newBuilding.layerColl->layers.numElem() - 1;
	}

	if(m_curLayerId < 0)
		m_curLayerId = 0;

	IVector2D globalTile;
	g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(tx,ty), m_selectedRegion, globalTile);

	Vector3D tilePos = g_pGameWorld->m_level.GlobalTilePointToPosition(globalTile);

	if(m_tiledPlacement->GetValue())
		m_mousePoint = tilePos;
	else
		m_mousePoint = ppos;

	if(!event.ControlDown() && !event.AltDown())
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT) && !event.Dragging())
		{
			if( !m_layerCollList->GetSelectedLayerColl() && m_mode == ED_BUILD_READY )
			{
				wxMessageBox("Please select template to begin", "Can't do it!", wxOK | wxCENTRE, g_pMainFrame);
				return;
			}

			if(m_mode == ED_BUILD_READY)
				m_mode = ED_BUILD_BEGUN;	// make to the point 1

			if(m_mode == ED_BUILD_BEGUN)
			{
				buildSegmentPoint_t segment;
				segment.layerId = m_curLayerId;
				segment.position = m_mousePoint;

				// make a first point
				m_newBuilding.points.addLast( segment );
				m_newBuilding.layerColl = m_layerCollList->GetSelectedLayerColl();
				m_newBuilding.order = 1;

				m_curSegmentScale = 1.0f;

				m_mode = ED_BUILD_SELECTEDPOINT;
				return;
			}
			else if(m_mode == ED_BUILD_SELECTEDPOINT)
			{
				if(m_placeError)
					return;

				if(!m_newBuilding.points.goToFirst())
					return;

				Vector3D firstPoint = m_newBuilding.points.getCurrent().position;

				if(!m_newBuilding.points.goToLast())
					return;

				// Modify last point to use selected segment and layer
				buildSegmentPoint_t& lastPoint = m_newBuilding.points.getCurrentNode()->object;
				lastPoint.layerId = m_curLayerId;
				lastPoint.scale = m_curSegmentScale;

				// height of the segments must be equal to first point
				m_mousePoint.y = firstPoint.y;

				buildSegmentPoint_t newSegment;
				newSegment.layerId = 0;
				newSegment.scale = 1.0f;
				newSegment.position = ComputePlacementPointBasedOnMouse();

				// Add point to the building
				m_newBuilding.points.addLast( newSegment );

				// ED_BUILD_DONE if connected to begin point
				//if(distance(ppos, m_newBuilding.points[0]) < 2.0f)
				//	m_mode = ED_BUILD_DONE;

				return;
			}
		}
	}
}

void CUI_BuildingConstruct::CancelBuilding()
{
	if(m_mode != ED_BUILD_READY)
	{
		m_newBuilding.points.clear();
		m_newBuilding.layerColl = NULL;

		m_mode = ED_BUILD_READY;
	}
}

void CUI_BuildingConstruct::CompleteBuilding()
{
	// generate building model for region:
	if(!m_selectedRegion || m_newBuilding.points.getCount() == 0 || m_newBuilding.layerColl == NULL)
		return;

	buildingSource_t* newBuilding = new buildingSource_t(m_newBuilding);

	if( !GenerateBuildingModel(newBuilding) )
	{
		delete newBuilding;
		return;
	}

	CEditorLevelRegion* region = (CEditorLevelRegion*)g_pGameWorld->m_level.GetRegionAtPosition(newBuilding->modelPosition);

	if( !region )
		region = (CEditorLevelRegion*)m_selectedRegion;

	// add building to the region
	region->m_buildings.append( newBuilding );

	m_newBuilding.points.clear();
	m_newBuilding.layerColl = NULL;
	m_curLayerId = 0;
	m_curSegmentScale = 1.0f;

	m_mode = ED_BUILD_READY;
}

void CUI_BuildingConstruct::ClearSelection()
{

}

void CUI_BuildingConstruct::DeleteSelection()
{
	if(m_mode == ED_BUILD_SELECTEDPOINT)
	{
		if(m_newBuilding.points.getCount() > 1)
		{
			m_newBuilding.points.goToLast();
			m_newBuilding.points.removeCurrent(); //removeIndex(m_newBuilding.points.numElem()-1);
		}
		else
			CancelBuilding();
	}
}

extern void ListQuadTex(const Vector3D &v1, const Vector3D &v2, const Vector3D& v3, const Vector3D& v4, int rotate, const ColorRGBA &color, DkList<Vertex3D_t> &verts);

void CUI_BuildingConstruct::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(!m_selectedRegion)
		return;

	CHeightTileFieldRenderable* field = m_selectedRegion->m_heightfield[0];
	field->DebugRender(false,m_mouseOverTileHeight);

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	// only draw if building is not empty
	if(m_newBuilding.points.getCount() > 0 && m_newBuilding.layerColl != NULL)
	{
		buildSegmentPoint_t extraSegment;
		extraSegment.position = ComputePlacementPointBasedOnMouse();
		extraSegment.layerId = m_curLayerId;
		extraSegment.scale = m_curSegmentScale;

		//
		// Finally render the dynamic preview of our building
		//
		RenderBuilding(&m_newBuilding, &extraSegment);

		// render the actual and computed points
		debugoverlay->Box3D(m_mousePoint - 0.5f, m_mousePoint + 0.5f, ColorRGBA(1,0,0,1));
		debugoverlay->Box3D(extraSegment.position-0.5f, extraSegment.position+0.5f, ColorRGBA(1,1,0,1));

		// render points of building
		for(DkLLNode<buildSegmentPoint_t>* lln = m_newBuilding.points.goToFirst(); lln != NULL; lln = m_newBuilding.points.goToNext())
		{
			debugoverlay->Box3D(lln->object.position - 0.5f, lln->object.position + 0.5f, ColorRGBA(0,1,0,1));
		}
	}
	else
		m_placeError = false;

	CBaseTilebasedEditor::OnRender();
}

Vector3D CUI_BuildingConstruct::ComputePlacementPointBasedOnMouse()
{
	if(!m_newBuilding.points.goToLast())
		return vec3_zero;

	// now the computations
	const buildSegmentPoint_t& end = m_newBuilding.points.getCurrent();

	// get scaled segment length of last node
	buildLayer_t& layer = m_newBuilding.layerColl->layers[m_curLayerId];
	float segLen = GetSegmentLength(layer) * m_curSegmentScale;

	buildSegmentPoint_t testSegPoint;
	testSegPoint.position = m_mousePoint;

	// calculate segment interations from last node to mouse position
	int numIterations = GetLayerSegmentIterations(end, testSegPoint, segLen);
	m_placeError = (numIterations == 0);

	// compute the valid placement point of segment
	Vector3D segmentDir = normalize(m_mousePoint - end.position);
	return end.position + segmentDir * floor(numIterations) * segLen;
}