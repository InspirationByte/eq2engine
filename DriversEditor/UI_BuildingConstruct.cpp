//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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
	m_model->Render(0);

	//Render(0.0f, bbox, true, 0);

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// copy rendertarget
	UTIL_CopyRendertargetToTexture(pTempRendertarget, m_preview);
}

//---------------------------------------------------------------------------------------

CFloorSetEditDialog::CFloorSetEditDialog(wxWindow* parent) 
	: wxDialog(parent, wxID_ANY, wxT("Building leyer floor set editor"), wxDefaultPosition, wxSize(1002, 280), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer(wxHORIZONTAL);

	m_renderPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	m_renderPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));

	bSizer18->Add(m_renderPanel, 1, wxEXPAND | wxALL, 5);

	m_panel18 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, -1), wxTAB_TRAVERSAL);
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* sbSizer12;
	sbSizer12 = new wxStaticBoxSizer(new wxStaticBox(m_panel18, wxID_ANY, wxT("Floors")), wxVERTICAL);

	m_newBtn = new wxButton(m_panel18, FLOOREDIT_NEW, wxT("New"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer12->Add(m_newBtn, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxEXPAND, 5);


	bSizer25->Add(sbSizer12, 0, wxEXPAND, 5);

	m_propertyBox = new wxStaticBoxSizer(new wxStaticBox(m_panel18, wxID_ANY, wxT("Selected floor")), wxVERTICAL);

	m_btnChoose = new wxButton(m_panel18, FLOOREDIT_CHOOSEMODEL, wxT("Choose model..."), wxDefaultPosition, wxDefaultSize, 0);
	m_propertyBox->Add(m_btnChoose, 0, wxALL | wxEXPAND, 5);

	m_upBtn = new wxButton(m_panel18, FLOOREDIT_UP, wxT("Move up"), wxDefaultPosition, wxDefaultSize, 0);
	m_propertyBox->Add(m_upBtn, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxEXPAND, 5);

	m_dnBtn = new wxButton(m_panel18, FLOOREDIT_DOWN, wxT("Move down"), wxDefaultPosition, wxDefaultSize, 0);
	m_propertyBox->Add(m_dnBtn, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxEXPAND, 5);

	m_delBtn = new wxButton(m_panel18, FLOOREDIT_DELETE, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0);
	m_propertyBox->Add(m_delBtn, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);


	bSizer25->Add(m_propertyBox, 1, wxEXPAND, 5);


	m_panel18->SetSizer(bSizer25);
	m_panel18->Layout();
	bSizer25->Fit(m_panel18);
	bSizer18->Add(m_panel18, 0, wxALL | wxEXPAND, 5);


	this->SetSizer(bSizer18);
	this->Layout();

	this->Centre(wxBOTH);

	// create swap chain
	m_pSwapChain = materials->CreateSwapChain(m_renderPanel->GetHandle());
	m_selLayer = nullptr;
	m_selModel = nullptr;
	m_pFont = nullptr;
	
	// Connect Events
	m_renderPanel->Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(CFloorSetEditDialog::OnEraseBackground), NULL, this);
	m_newBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);
	m_btnChoose->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);
	m_delBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);

	m_upBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);
	m_dnBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);

	m_renderPanel->Connect(wxEVT_MOTION, wxMouseEventHandler(CFloorSetEditDialog::OnMouseMotion), NULL, this);
	m_renderPanel->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(CFloorSetEditDialog::OnMouseScroll), NULL, this);
	m_renderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CFloorSetEditDialog::OnMouseClick), NULL, this);
	m_renderPanel->Connect(wxEVT_SCROLLBAR, wxScrollWinEventHandler(CFloorSetEditDialog::OnScrollbarChange), NULL, this);
	m_renderPanel->Connect(wxEVT_IDLE, wxIdleEventHandler(CFloorSetEditDialog::OnIdle), NULL, this);
}

CFloorSetEditDialog::~CFloorSetEditDialog()
{
	// Disconnect Events
	m_renderPanel->Disconnect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(CFloorSetEditDialog::OnEraseBackground), NULL, this);
	m_renderPanel->Disconnect(wxEVT_SIZE, wxSizeEventHandler(CFloorSetEditDialog::OnSize), NULL, this);
	m_newBtn->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);
	m_btnChoose->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);
	m_delBtn->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);

	m_upBtn->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);
	m_dnBtn->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CFloorSetEditDialog::OnBtnsClick), NULL, this);

	m_renderPanel->Disconnect(wxEVT_MOTION, wxMouseEventHandler(CFloorSetEditDialog::OnMouseMotion), NULL, this);
	m_renderPanel->Disconnect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(CFloorSetEditDialog::OnMouseScroll), NULL, this);
	m_renderPanel->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CFloorSetEditDialog::OnMouseClick), NULL, this);
	m_renderPanel->Disconnect(wxEVT_SCROLLBAR, wxScrollWinEventHandler(CFloorSetEditDialog::OnScrollbarChange), NULL, this);
	m_renderPanel->Disconnect(wxEVT_IDLE, wxIdleEventHandler(CFloorSetEditDialog::OnIdle), NULL, this);

}

void CFloorSetEditDialog::OnScrollbarChange(wxScrollWinEvent& event)
{
	if (event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_TOP)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) - 1, true);
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM)
	{
		SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL) + 1, true);
	}
	else
		SetScrollPos(wxVERTICAL, event.GetPosition(), true);

	Redraw();
}

void CFloorSetEditDialog::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos = GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation() / 100, true);
}

void CFloorSetEditDialog::OnMouseClick(wxMouseEvent& event)
{
	// set selection to mouse over
	m_selectedItem = m_mouseoverItem;

	UpdateSelection();
}

void CFloorSetEditDialog::OnMouseMotion(wxMouseEvent& event)
{
	m_mousePos.x = event.GetX();
	m_mousePos.y = event.GetY();

	Redraw();
}

void CFloorSetEditDialog::Redraw()
{
	RenderList();
}

void CFloorSetEditDialog::RenderList()
{
	if (!m_selLayer)
		return;

	if (!materials)
		return;

	if (!m_pFont)
		m_pFont = g_fontCache->GetFont("debug", 0);

	int w, h;
	m_renderPanel->GetSize(&w, &h);
	g_pShaderAPI->SetViewport(0, 0, w, h);

	materials->GetConfiguration().wireframeMode = false;
	materials->SetAmbientColor(color4_white);

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
	fontParam.textColor = ColorRGBA(1, 1, 1, 1);
	fontParam.layoutBuilder = &rectLayout;

	m_mouseoverItem = -1;

	if (materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true, true, false, ColorRGBA(0, 0, 0, 0));
		materials->Setup2D(w, h);

		int nLine = 0;
		int nItem = 0;

		Rectangle_t screenRect(0, 0, w, h);
		screenRect.Fix();

		float scrollbarpercent = GetScrollPos(wxVERTICAL);

		int numItems = 0;

		const float fSize = 128.0f;

		for (int i = 0; i < m_selLayer->models.numElem(); i++)
		{
			float x_offset = 16 + nItem * (fSize + 16);

			if (x_offset + fSize > w)
			{
				numItems = i;
				break;
			}

			numItems++;
			nItem++;
		}

		if (numItems > 0)
		{
			nItem = 0;

			for (int i = 0; i < m_selLayer->models.numElem(); i++)
			{
				CLayerModel* model = m_selLayer->models[i];

				if (nItem >= numItems)
				{
					nItem = 0;
					nLine++;
				}

				float x_offset = 16 + nItem * (fSize + 16);
				float y_offset = 8 + nLine * (fSize + 48);

				y_offset -= scrollbarpercent * (fSize + 48);

				Rectangle_t check_rect(x_offset, y_offset, x_offset + fSize, y_offset + fSize);
				check_rect.Fix();

				if (check_rect.vleftTop.y > screenRect.vrightBottom.y)
					break;

				if (!screenRect.IsIntersectsRectangle(check_rect))
				{
					nItem++;
					continue;
				}

				float texture_aspect = 1.0f;
				float x_scale = 1.0f;
				float y_scale = 1.0f;

				ITexture* pTex = model->GetPreview();

				texture_aspect = pTex->GetWidth() / pTex->GetHeight();

				if (pTex->GetWidth() > pTex->GetHeight())
					y_scale /= texture_aspect;

				Rectangle_t name_rect(x_offset, y_offset + fSize, x_offset + fSize, y_offset + fSize + 400);

				Vertex2D_t verts[] = { MAKETEXQUAD(x_offset, y_offset, x_offset + fSize * x_scale,y_offset + fSize * y_scale, 0) };
				Vertex2D_t name_line[] = { MAKETEXQUAD(x_offset, y_offset + fSize, x_offset + fSize,y_offset + fSize + 25, 0) };

				// draw name panel
				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25, 0.25, 1, 1), &blendParams);

				// mouseover rectangle
				if (check_rect.IsInRectangle(m_mousePos))
				{
					m_mouseoverItem = i;

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts, 4, pTex, ColorRGBA(1, 0.5f, 0.5f, 1), &blendParams);

					Vertex2D rectVerts[] = { MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0) };

					materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(1, 0, 0, 1));
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25, 0.1, 0.25, 1), &blendParams);
				}
				else
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts, 4, pTex, color4_white, &blendParams);

				// draw selection rectangle
				if (m_selectedItem == i)
				{
					Vertex2D rectVerts[] = { MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0) };

					materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(0, 1, 0, 1));
					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(1, 0.25, 0.25, 1), &blendParams);
				}

				// render layer name text
				rectLayout.SetRectangle(name_rect);

				// TODO: draw model index
				m_pFont->RenderText(model->m_name.c_str(), name_rect.vleftTop, fontParam);

				//else if(elem.type == LAYER_MODEL)
				//	m_pFont->RenderText("pls sel model", name_rect.vleftTop, fontParam);

				nItem++;
			}
		}

		materials->EndFrame(m_pSwapChain);
	}
}

void CFloorSetEditDialog::OnBtnsClick(wxCommandEvent& event)
{
	switch(event.GetId())
	{
		case FLOOREDIT_NEW:
		{
			CallAddModel();
			break;
		}
		case FLOOREDIT_CHOOSEMODEL:
		{
			if (m_selectedItem < 0)
			{
				wxMessageBox("First you need to select model!");
				break;
			}

			// replacement function
			CLayerModel* model = GetModelUsingDialog();

			if(!model)
				break;

			CLayerModel* oldModel = m_selLayer->models[m_selectedItem];
			delete oldModel;

			m_selLayer->models[m_selectedItem] = model;
			m_selModel = model;

			g_pMainFrame->NotifyUpdate();
			break;
		}
		case FLOOREDIT_UP:
		{
			break;
		}
		case FLOOREDIT_DOWN:
		{
			break;
		}
		case FLOOREDIT_DELETE:
		{
			if (m_selectedItem < 0)
				break;

			CLayerModel* model = m_selLayer->models[m_selectedItem];
			delete model;

			m_selLayer->models.removeIndex(m_selectedItem);
			m_selectedItem--;

			UpdateSelection();

			break;
		}
	}
}

void CFloorSetEditDialog::UpdateSelection()
{
	if (m_selectedItem == -1)
	{
		m_selModel = NULL;
		m_propertyBox->ShowItems(false);
		Layout();

		return;
	}

	m_selModel = m_selLayer->models[m_selectedItem];

	m_propertyBox->ShowItems(true);
	Layout();
}

void CFloorSetEditDialog::SetLayerModelList(buildLayer_t* coll)
{
	m_selModel = nullptr;
	m_selectedItem = -1;
	m_selLayer = coll;
}

buildLayer_t* CFloorSetEditDialog::GetLayerModelList() const
{
	return m_selLayer;
}

void CFloorSetEditDialog::CallAddModel()
{
	// Open model choose dialog first
	CLayerModel* model = GetModelUsingDialog();

	if (!model)
		return;

	m_selectedItem = m_selLayer->models.append(model);
	UpdateSelection();

	g_pMainFrame->NotifyUpdate();
}

CLayerModel* CFloorSetEditDialog::GetModelUsingDialog()
{
	wxFileDialog* file = new wxFileDialog(NULL, "Open OBJ/ESM", "./", "*.*", "Model files (*.obj, *.esm)|*.obj;*.esm", wxFD_FILE_MUST_EXIST | wxFD_OPEN);

	// hide this dialog
	Hide();

	CLayerModel* lmodel = nullptr;

	if (file->ShowModal() == wxID_OK)
	{
		wxArrayString paths;
		EqString path(file->GetPath().wchar_str());

		dsmmodel_t model;

		if (!LoadSharedModel(&model, path.c_str()))
		{
			Show();
			return nullptr;
		}

		CLevelModel* pLevModel = new CLevelModel();
		pLevModel->CreateFrom(&model);
		FreeDSM(&model);

		// make container
		lmodel = new CLayerModel();
		lmodel->SetDirtyPreview();

		lmodel->m_model = pLevModel;
		lmodel->m_name = path.Path_Extract_Name().Path_Strip_Ext().c_str();
		lmodel->RefreshPreview();
	}

	// and show again
	Show();

	return lmodel;
}

//-----------------------------------------------------------------------------
// Layer collection list
//-----------------------------------------------------------------------------
/*
BEGIN_EVENT_TABLE(CBuildingLayerList, wxPanel)
    EVT_ERASE_BACKGROUND(CBuildingLayerList::OnEraseBackground)
    EVT_IDLE(CBuildingLayerList::OnIdle)
	EVT_SCROLLWIN(CBuildingLayerList::OnScrollbarChange)
	EVT_MOTION(CBuildingLayerList::OnMouseMotion)
	EVT_MOUSEWHEEL(CBuildingLayerList::OnMouseScroll)
	EVT_LEFT_DOWN(CBuildingLayerList::OnMouseClick)
	EVT_SIZE(CBuildingLayerList::OnSizeEvent)
END_EVENT_TABLE()

CBuildingLayerList::CBuildingLayerList(CUI_BuildingConstruct* parent) : wxPanel( parent, -1, wxPoint(0, 0), wxSize(640, 480))
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

void CBuildingLayerList::SetSelectedLayerColl(buildLayerColl_t* layerColl)
{
	for(int i = 0; i < m_filteredList.numElem(); i++)
	{
		if(m_filteredList[i] == layerColl)
			m_selection = i;
	}
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

*/

//-----------------------------------------------------------------------------
// the editor panel
//-----------------------------------------------------------------------------

CUI_BuildingConstruct::CUI_BuildingConstruct( wxWindow* parent )
	 : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER | wxSTAY_ON_TOP), CBaseTilebasedEditor()
{
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer(wxHORIZONTAL);

	wxPanel* m_panel21 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	wxBoxSizer* bSizer27;
	bSizer27 = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer* sbSizer20;
	sbSizer20 = new wxStaticBoxSizer(new wxStaticBox(m_panel21, wxID_ANY, wxT("Templates")), wxHORIZONTAL);

	m_templateList = new wxListBox(m_panel21, wxID_ANY, wxDefaultPosition, wxSize(200, -1), 0, NULL, 0);
	sbSizer20->Add(m_templateList, 1, wxRIGHT | wxLEFT | wxEXPAND, 5);

	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer(wxVERTICAL);

	m_button5 = new wxButton(m_panel21, BUILD_TEMPLATE_NEW, wxT("New..."), wxDefaultPosition, wxDefaultSize, 0);
	bSizer30->Add(m_button5, 0, wxALL, 5);

	m_button29 = new wxButton(m_panel21, BUILD_TEMPLATE_RENAME, wxT("Rename"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer30->Add(m_button29, 0, wxALL, 5);

	m_button8 = new wxButton(m_panel21, BUILD_TEMPLATE_DELETE, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer30->Add(m_button8, 0, wxALL, 5);


	sbSizer20->Add(bSizer30, 0, wxEXPAND, 5);


	bSizer27->Add(sbSizer20, 0, wxEXPAND, 5);

	wxStaticBoxSizer* m_layerSetBox;
	m_layerSetBox = new wxStaticBoxSizer(new wxStaticBox(m_panel21, wxID_ANY, wxT("Variants")), wxHORIZONTAL);

	m_layerList = new wxListBox(m_panel21, wxID_ANY, wxDefaultPosition, wxSize(200, -1), 0, NULL, 0);
	m_layerSetBox->Add(m_layerList, 0, wxRIGHT | wxLEFT | wxEXPAND, 5);

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer(wxVERTICAL);

	m_button26 = new wxButton(m_panel21, BUILD_LAYER_NEW, wxT("New..."), wxDefaultPosition, wxDefaultSize, 0);
	bSizer31->Add(m_button26, 0, wxALL, 5);

	m_button28 = new wxButton(m_panel21, BUILD_LAYER_EDIT, wxT("Edit..."), wxDefaultPosition, wxDefaultSize, 0);
	bSizer31->Add(m_button28, 0, wxALL, 5);

	m_button27 = new wxButton(m_panel21, BUILD_LAYER_DELETE, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer31->Add(m_button27, 0, wxALL, 5);


	m_layerSetBox->Add(bSizer31, 1, wxEXPAND, 5);


	bSizer27->Add(m_layerSetBox, 0, wxEXPAND | wxLEFT, 5);


	m_panel21->SetSizer(bSizer27);
	m_panel21->Layout();
	bSizer27->Fit(m_panel21);
	bSizer9->Add(m_panel21, 1, wxEXPAND | wxALL, 5);

	m_pSettingsPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 150), wxTAB_TRAVERSAL);
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer(new wxStaticBox(m_pSettingsPanel, wxID_ANY, wxT("Search and Filter")), wxVERTICAL);

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer1->SetFlexibleDirection(wxBOTH);
	fgSizer1->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	fgSizer1->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	m_filtertext = new wxTextCtrl(m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0);
	m_filtertext->SetMaxLength(0);
	fgSizer1->Add(m_filtertext, 0, wxBOTTOM | wxRIGHT | wxLEFT, 5);


	sbSizer2->Add(fgSizer1, 0, wxEXPAND, 5);


	bSizer10->Add(sbSizer2, 0, wxEXPAND, 5);

	m_tiledPlacement = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("Tiled placement (T)"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer10->Add(m_tiledPlacement, 0, wxALL, 5);

	m_offsetCloning = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("Offset cloning (O)"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer10->Add(m_offsetCloning, 0, wxALL, 5);


	m_pSettingsPanel->SetSizer(bSizer10);
	m_pSettingsPanel->Layout();
	bSizer9->Add(m_pSettingsPanel, 0, wxALL | wxEXPAND, 5);


	this->SetSizer(bSizer9);
	this->Layout();

	// Connect Events
	m_templateList->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CUI_BuildingConstruct::OnSelectTemplate), NULL, this);
	m_button5->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button29->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button8->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button26->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button28->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button27->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_filtertext->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CUI_BuildingConstruct::OnFilterChange), NULL, this);

	m_layerList->Connect(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnItemDclick), NULL, this);

	m_layerEditDlg = new CFloorSetEditDialog(g_pMainFrame);
	m_placeError = false;

	m_curModelId = 0;
	m_curSegmentScale = 1.0f;
	m_isEditingNewBuilding = false;
	m_editingBuilding = NULL;
}

CUI_BuildingConstruct::~CUI_BuildingConstruct()
{
	// Disconnect Events
	m_templateList->Disconnect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(CUI_BuildingConstruct::OnSelectTemplate), NULL, this);
	m_button5->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button29->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button8->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button26->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button28->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_button27->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnBtnsClick), NULL, this);
	m_filtertext->Disconnect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CUI_BuildingConstruct::OnFilterChange), NULL, this);

	m_layerList->Disconnect(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler(CUI_BuildingConstruct::OnItemDclick), NULL, this);
}

// Virtual event handlers, overide them in your derived class
void CUI_BuildingConstruct::OnFilterChange( wxCommandEvent& event )
{
	UpdateAndFilterList();
}

void CUI_BuildingConstruct::LoadTemplates()
{
	const char* levelName = g_pGameWorld->GetLevelName();

	EqString folderPath = varargs(LEVELS_PATH "%s_editor", levelName);

	KeyValues kvDefs;
	if (!kvDefs.LoadFromFile(_Es(folderPath + "/buildingTemplates.def").c_str()))
		return;

	// load model files
	IFile* pFile = g_fileSystem->Open(_Es(folderPath + "/buildingTemplateModels.dat").c_str(), "rb", SP_MOD);

	if (!pFile)
	{
		ErrorMsg("Failed to open buildingTemplateModels.dat !!!");
		return;
	}

	int numLayerCollections = 0;
	pFile->Read(&numLayerCollections, 1, sizeof(int));

	for (int i = 0; i < kvDefs.GetRootSection()->keys.numElem(); i++)
	{
		if (stricmp(kvDefs.GetRootSection()->keys[i]->name, "template"))
			continue;

		kvkeybase_t* templateSec = kvDefs.GetRootSection()->keys[i];

		buildLayerColl_t* coll = new buildLayerColl_t();
		m_buildingTemplates.append(coll);

		coll->name = KV_GetValueString(templateSec, 0, "unnamed");

		coll->Load(pFile, templateSec);
	}

	UpdateAndFilterList();

	g_fileSystem->Close(pFile);
}

void CUI_BuildingConstruct::SaveTemplates()
{
	const char* levelName = g_pGameWorld->GetLevelName();

	EqString folderPath(varargs(LEVELS_PATH "%s_editor", levelName));

	// make folder <levelName>_editor and put this stuff there
	g_fileSystem->MakeDir(folderPath.c_str(), SP_MOD);

	// keyvalues file buildings.def
	KeyValues kvDefs;

	// save model files
	IFile* pFile = g_fileSystem->Open(_Es(folderPath + "/buildingTemplateModels.dat").c_str(), "wb", SP_MOD);

	int numLayerCollections = m_buildingTemplates.numElem();
	pFile->Write(&numLayerCollections, 1, sizeof(int));

	for (int i = 0; i < m_buildingTemplates.numElem(); i++)
	{
		buildLayerColl_t* coll = m_buildingTemplates[i];

		kvkeybase_t* layerCollData = kvDefs.GetRootSection()->AddKeyBase("template", coll->name.c_str());

		coll->Save(pFile, layerCollData);
	}

	kvDefs.SaveToFile(_Es(folderPath + "/buildingTemplates.def").c_str());

	g_fileSystem->Close(pFile);
}

void CUI_BuildingConstruct::RemoveAllTemplates()
{
	for (int i = 0; i < m_buildingTemplates.numElem(); i++)
		delete m_buildingTemplates[i];

	m_buildingTemplates.clear();

	UpdateAndFilterList();
}

void CUI_BuildingConstruct::UpdateAndFilterList()
{
	// m_filtertext->GetValue()
	m_templateList->Clear();

	for (int i = 0; i < m_buildingTemplates.numElem(); i++)
	{
		m_templateList->Append(m_buildingTemplates[i]->name.c_str(), m_buildingTemplates[i]);
	}
}

void CUI_BuildingConstruct::UpdateLayerList()
{
	buildLayerColl_t* templ = GetSelectedTemplate();

	m_layerList->Clear();
	if (!templ)
		return;

	for (int i = 0; i < templ->layers.numElem(); i++)
	{
		buildLayer_t& layer = templ->layers[i];
		m_layerList->Append(varargs("Layer %d (%d models)", i + 1, layer.models.numElem()), &layer);
	}
}

buildLayerColl_t* CUI_BuildingConstruct::GetSelectedTemplate()
{
	int selIndex = m_templateList->GetSelection();

	if (selIndex == -1)
		return nullptr;

	buildLayerColl_t* templ = (buildLayerColl_t*)m_templateList->GetClientData(selIndex);

	return templ;
}

void CUI_BuildingConstruct::SetSelectedTemplate(buildLayerColl_t* templ)
{
	int templIdx = m_buildingTemplates.findIndex(templ);
	m_templateList->SetSelection(templIdx);

	// update layer list
	UpdateLayerList();

	// reset layer selection
	SetSelectedLayerId(0);
}

int	CUI_BuildingConstruct::GetSelectedLayerId()
{
	return m_layerList->GetSelection();
}

void CUI_BuildingConstruct::SetSelectedLayerId(int idx)
{
	buildLayerColl_t* templ = GetSelectedTemplate();

	if (templ && templ->layers.inRange(idx))
	{
		m_layerList->SetSelection(idx);
		m_curModelId = 0;
	}
	else if (m_layerList->GetCount())
	{
		m_layerList->SetSelection(0);
		m_curModelId = 0;
	}
}

void CUI_BuildingConstruct::OnSelectTemplate(wxCommandEvent& event)
{
	UpdateLayerList();
}

void CUI_BuildingConstruct::OnItemDclick(wxCommandEvent& event)
{
	CallEditLayer();
}

void CUI_BuildingConstruct::OnBtnsClick( wxCommandEvent& event )
{
	switch(event.GetId())
	{
		case BUILD_TEMPLATE_NEW:
		case BUILD_TEMPLATE_RENAME:
		{
			wxTextEntryDialog* dlg = new wxTextEntryDialog(this, "Please enter name of template", "Enter the name");

			if (dlg->ShowModal() == wxID_OK)
			{
				buildLayerColl_t* newTempl = new buildLayerColl_t();
				newTempl->name = dlg->GetValue().c_str();

				m_buildingTemplates.append(newTempl);
				UpdateAndFilterList();
			}

			delete dlg;

			break;
		}
		case BUILD_TEMPLATE_DELETE:
		{
			// this will remove all associated buildings
			// or convert them into models

			/*
			buildLayerColl_t* layerColl = m_layerCollList->GetSelectedLayerColl();

			if (m_editingBuilding && m_editingBuilding->layerColl == layerColl)
				m_editingBuilding->layerColl = NULL;
			*/

			break;
		}
		case BUILD_LAYER_NEW:
		{
			buildLayerColl_t* selTempl = GetSelectedTemplate();

			if(!selTempl)
			{
				wxMessageBox("Please select template", "Can't do it!", wxOK | wxCENTRE, g_pMainFrame);
				break;
			}

			int newIdx = selTempl->layers.append(buildLayer_t());
			buildLayer_t& layer = selTempl->layers[newIdx];

			m_layerEditDlg->SetLayerModelList(&layer);

			m_layerEditDlg->SetTitle(varargs("New Layer %d", newIdx+1));
			m_layerEditDlg->CallAddModel();

			m_layerEditDlg->ShowModal();

			UpdateLayerList();

			break;
		}
		case BUILD_LAYER_EDIT:
		{
			CallEditLayer();

			break;
		}
		case BUILD_LAYER_DELETE:
		{

			/*
			buildLayerColl_t* layerColl = m_layerCollList->GetSelectedLayerColl();

			if (m_editingBuilding && m_editingBuilding->layerColl == layerColl)
				m_editingBuilding->layerColl = NULL;
			*/
			break;
		}
	}
	
}

void CUI_BuildingConstruct::CallEditLayer()
{
	buildLayerColl_t* selTempl = GetSelectedTemplate();

	if (!selTempl)
	{
		wxMessageBox("Please select template", "Can't do it!", wxOK | wxCENTRE, g_pMainFrame);
		return;
	}

	int selLayerIdx = GetSelectedLayerId();

	if (selLayerIdx == -1)
	{
		wxMessageBox("Please select layer", "Can't do it!", wxOK | wxCENTRE, g_pMainFrame);
		return;
	}

	buildLayer_t& layer = selTempl->layers[selLayerIdx];

	m_layerEditDlg->SetLayerModelList(&layer);
	m_layerEditDlg->SetTitle(varargs("Editing Layer %d", selLayerIdx + 1));

	m_layerEditDlg->ShowModal();

	UpdateLayerList();
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
			if(m_mode == ED_BUILD_MOVEMENT)
			{
				m_mode = ED_BUILD_READY;
			}
			else if(m_mode == ED_BUILD_READY)
			{
				ClearSelection();
			}
			else
				CancelBuilding();
		}
		else if(event.GetRawKeyCode() == 'E')
		{
			EditSelectedBuilding();
		}
		else if(event.GetRawKeyCode() == 'G')
		{
			m_mode = ED_BUILD_MOVEMENT;
		}
		else if(event.m_keyCode == WXK_DELETE)
		{
			DeleteSelection();
		}
		else if(event.m_keyCode == WXK_SPACE)
		{
			if(m_editingBuilding)
			{
				m_editingBuilding->order++;
				if (m_editingBuilding->order > 3)
					m_editingBuilding->order = 0;

				return;
			}

			DuplicateSelection();
		}
		else if(event.m_keyCode == WXK_RETURN)
		{
			CompleteBuilding();
		}
		else if(event.GetRawKeyCode() == 'T')
		{
			m_tiledPlacement->SetValue(!m_tiledPlacement->GetValue());
		}
		else if(event.GetRawKeyCode() == 'O')
		{
			m_offsetCloning->SetValue(!m_offsetCloning->GetValue());
		}
		
	}

	if(event.m_keyCode == WXK_SHIFT)
	{
		m_shiftModifier = bDown;
	}
}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::InitTool()
{

}

void CUI_BuildingConstruct::OnLevelLoad()
{
	CBaseTilebasedEditor::OnLevelLoad();
	LoadTemplates();

	// assign layers on buildings
	g_pGameWorld->m_level.PostLoadEditorBuildings(m_buildingTemplates);

	m_mode = ED_BUILD_READY;
}

void CUI_BuildingConstruct::OnLevelSave()
{
	CBaseTilebasedEditor::OnLevelSave();
	SaveTemplates();

	// save each region of level
	
}

void CUI_BuildingConstruct::OnLevelUnload()
{
	ClearSelection();
	CancelBuilding();

	CBaseTilebasedEditor::OnLevelUnload();

	RemoveAllTemplates();
}

void CUI_BuildingConstruct::OnSwitchedTo()
{
	// if we switched from this tool, show selected hidden refs
	for(int i = 0; i < m_selBuildings.numElem(); i++)
	{
		m_selBuildings[i].selBuild->hide = true;
	}

	IEditorTool::OnSwitchedTo();
}

void CUI_BuildingConstruct::OnSwitchedFrom()
{
	// if we switched from this tool, show selected hidden refs
	for(int i = 0; i < m_selBuildings.numElem(); i++)
	{
		m_selBuildings[i].selBuild->hide = false;
	}

	IEditorTool::OnSwitchedFrom();
}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::ProcessMouseEvents( wxMouseEvent& event )
{
	Vector3D ray_start, ray_dir;
	g_pMainFrame->GetMouseScreenVectors(event.GetX(), event.GetY(), ray_start, ray_dir);

	if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT) && event.Dragging())
	{
		float delta = m_mouseLastY - event.GetY();
		
		// make scale
		m_curSegmentScale += delta*0.001f;
		m_curSegmentScale = clamp(m_curSegmentScale,0.5f, 2.0f);
	}
	else
	{
		if (m_mode == ED_BUILD_READY)
		{
			if (event.ControlDown())
			{
				m_isSelecting = true;

				if (event.ButtonIsDown(wxMOUSE_BTN_LEFT) && !event.Dragging() && !m_editingBuilding)
				{
					float dist = DrvSynUnits::MaxCoordInUnits;

					buildingSelInfo_t info;

					int refIdx = g_pGameWorld->m_level.Ed_SelectBuildingAndReg(ray_start, ray_dir, &info.selRegion, dist);

					if (refIdx != -1 && info.selRegion)
					{
						info.selBuild = info.selRegion->m_buildings[refIdx];

						ToggleSelection(info);
					}
				}
			}
			else
				m_isSelecting = false;

			CBaseTilebasedEditor::ProcessMouseEvents(event);
		}
		else if (m_mode == ED_BUILD_MOVEMENT)
		{
			if (m_selBuildings.numElem() > 0)
			{
				// try move selection
				if (m_mode == ED_BUILD_MOVEMENT)
					MouseTranslateEvents(event, ray_start, ray_dir);
			}
		}
		else
			CBaseTilebasedEditor::ProcessMouseEvents(event);
	}

	m_mouseLastY = event.GetY();
}

void CUI_BuildingConstruct::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	if(event.GetWheelRotation() != 0)
	{
		int wheelSign = sign(event.GetWheelRotation());
		m_curModelId += wheelSign;
	}

	if(m_editingBuilding && m_editingBuilding->layerColl != NULL)
	{
		int selectedLayer = GetSelectedLayerId();

		if(selectedLayer >= 0)
		{
			buildLayer_t& layer = m_editingBuilding->layerColl->layers[selectedLayer];

			if (m_curModelId >= layer.models.numElem())
				m_curModelId = layer.models.numElem() - 1;
		}
	}

	if(m_curModelId < 0)
		m_curModelId = 0;

	Vector3D ray_start, ray_dir;
	g_pMainFrame->GetMouseScreenVectors(event.GetX(),event.GetY(), ray_start, ray_dir);

	IVector2D globalTile;
	g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(tx,ty), m_selectedRegion, globalTile);

	Vector3D tilePos = g_pGameWorld->m_level.GlobalTilePointToPosition(globalTile);

	if(m_tiledPlacement->GetValue())
		m_mousePoint = tilePos;
	else
		m_mousePoint = ppos;

	if(!event.ControlDown())
	{
		m_isSelecting = false;

		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT) && !event.Dragging())
		{
			if(m_mode == ED_BUILD_READY)
				m_mode = ED_BUILD_BEGUN;	// make to the point 1

			if(m_mode == ED_BUILD_BEGUN)
			{
				if( !GetSelectedTemplate() || GetSelectedLayerId() == -1)
				{
					m_mode = ED_BUILD_READY;
					wxMessageBox("Please select template and layer to begin", "Can't do it!", wxOK | wxCENTRE, g_pMainFrame);
					return;
				}

				buildSegmentPoint_t segment;
				segment.layerId = GetSelectedLayerId();
				segment.modelId = m_curModelId;
				segment.position = m_mousePoint;

				m_editingBuilding = new buildingSource_t();
				m_isEditingNewBuilding = true;

				// make a first point
				m_editingBuilding->points.addLast( segment );
				m_editingBuilding->layerColl = GetSelectedTemplate();
				m_editingBuilding->order = 0;

				m_curSegmentScale = 1.0f;

				m_mode = ED_BUILD_SELECTEDPOINT;
				return;
			}
			else if(m_mode == ED_BUILD_SELECTEDPOINT)
			{
				if(!m_editingBuilding)
				{
					m_mode = ED_BUILD_READY;
					return;
				}

				if(m_placeError)
					return;

				if(!m_editingBuilding->points.goToFirst())
					return;

				Vector3D firstPoint = m_editingBuilding->points.getCurrent().position;

				if(!m_editingBuilding->points.goToLast())
					return;

				// Modify last point to use selected segment and layer
				buildSegmentPoint_t& lastPoint = m_editingBuilding->points.getCurrent();
				lastPoint.layerId = GetSelectedLayerId();
				lastPoint.modelId = m_curModelId;
				lastPoint.scale = m_curSegmentScale;

				// height of the segments must be equal to first point
				m_mousePoint.y = firstPoint.y;

				buildSegmentPoint_t newSegment;
				newSegment.layerId = GetSelectedLayerId();
				newSegment.modelId = m_curModelId;
				newSegment.scale = 1.0f;
				newSegment.position = ComputePlacementPointBasedOnMouse();

				// Add point to the building
				m_editingBuilding->points.addLast( newSegment );

				// ED_BUILD_DONE if connected to begin point
				//if(distance(ppos, m_editingBuilding->points[0]) < 2.0f)
				//	m_mode = ED_BUILD_DONE;
			}
		}
	}
}

extern Vector3D g_camera_target;

void CUI_BuildingConstruct::MouseTranslateEvents( wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir )
{
	if(!m_selBuildings.numElem())
		return;

	// display selection
	float clength = length(m_editAxis.m_position-g_camera_target);

	Vector3D plane_dir = normalize(m_editAxis.m_position-g_camera_target);

	if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
	{
		int initAxes = m_editAxis.TestRay(ray_start, ray_dir, clength, true);

		// update movement
		Vector3D movement = m_editAxis.PerformTranslate(ray_start, ray_dir, plane_dir, initAxes);

		m_editAxis.m_position += movement;

		for (int i = 0; i < m_selBuildings.numElem(); i++)
		{
			buildingSource_t* build = m_selBuildings[i].selBuild;

			build->modelPosition += movement;
			if (build->points.goToFirst())
			{
				do
				{
					buildSegmentPoint_t& seg = build->points.getCurrent();
					seg.position += movement;
				} while (build->points.goToNext());
			}
		}
	}

	if(event.ButtonUp(wxMOUSE_BTN_LEFT))
	{
#pragma todo("Relocate model in a level regions after movement (on left mouse UP)")

		m_editAxis.EndDrag();

		RecalcSelectionCenter();

		g_pMainFrame->NotifyUpdate();
	}
}

void CUI_BuildingConstruct::EditSelectedBuilding()
{
	if(m_selBuildings.numElem() != 1)
	{
		if(m_selBuildings.numElem() > 1)
			wxMessageBox("You have to select ONE building to begin editing", "Can't do it!", wxOK | wxCENTRE, g_pMainFrame);

		return;
	}

	// don't forget to copy
	buildingSource_t* selBuilding = m_selBuildings[0].selBuild;
	m_editingCopy.InitFrom(*selBuilding);

	m_editingBuilding = selBuilding;
	m_isEditingNewBuilding = false;
	m_mode = ED_BUILD_SELECTEDPOINT;

	SetSelectedTemplate( m_editingBuilding->layerColl );
	m_curModelId = 0;
	m_curSegmentScale = 1.0f;

	//ClearSelection();
}

void CUI_BuildingConstruct::CancelBuilding()
{
	if(m_mode != ED_BUILD_READY)
	{
		if( m_isEditingNewBuilding )
		{
			delete m_editingBuilding;
			m_editingBuilding = NULL;
		}
		else if(m_editingBuilding)
		{
			// delete model
			delete m_editingBuilding->model;

			// reset from the copy
			m_editingBuilding->InitFrom( m_editingCopy );
			GenerateBuildingModel(m_editingBuilding);

			m_editingBuilding = NULL;

			m_editingCopy.points.clear();
			m_editingCopy.layerColl = NULL;
		}

		m_mode = ED_BUILD_READY;
	}
}

void CUI_BuildingConstruct::CompleteBuilding()
{
	if(!m_editingBuilding)
		return;

	// generate building model for region:
	if(!m_selectedRegion || m_editingBuilding->points.getCount() == 0 || m_editingBuilding->layerColl == NULL)
		return;

	if( !GenerateBuildingModel(m_editingBuilding) )
	{
		CancelBuilding();
		return;
	}

	if(m_isEditingNewBuilding)
	{
		CEditorLevelRegion* region = (CEditorLevelRegion*)g_pGameWorld->m_level.GetRegionAtPosition(m_editingBuilding->modelPosition);

		if( !region )
			region = (CEditorLevelRegion*)m_selectedRegion;

		// add building to the region
		region->m_buildings.append( m_editingBuilding );
	}

	m_editingBuilding = NULL;

	m_curModelId = 0;
	m_curSegmentScale = 1.0f;

	m_mode = ED_BUILD_READY;

	g_pMainFrame->NotifyUpdate();
}

void CUI_BuildingConstruct::DuplicateSelection()
{
	DkList<buildingSelInfo_t> newSelection;

	for(int i = 0; i < m_selBuildings.numElem(); i++)
	{
		buildingSource_t* building =  m_selBuildings[i].selBuild;
		
		buildingSource_t* cloned = new buildingSource_t();
		cloned->InitFrom(*building);

		if(m_offsetCloning->GetValue())
		{
			// move a bit
			for(DkLLNode<buildSegmentPoint_t>* lln = cloned->points.goToFirst(); lln != NULL; lln = cloned->points.goToNext())
			{
				lln->object.position += FVector3D(1.0f,0.0f,1.0f);
			}
		}

		GenerateBuildingModel(cloned);

		cloned->hide = true; // prepare for selection

		buildingSelInfo_t selection;
		selection.selBuild = cloned;
		selection.selRegion = m_selBuildings[i].selRegion;

		selection.selRegion->m_buildings.append( cloned );

		newSelection.append(selection);
	}

	ClearSelection();

	m_selBuildings.swap(newSelection);
}

void CUI_BuildingConstruct::ClearSelection()
{
	for(int i = 0; i < m_selBuildings.numElem(); i++)
	{
		m_selBuildings[i].selBuild->hide = false;
	}

	m_selBuildings.clear();
}

void CUI_BuildingConstruct::DeleteSelection()
{
	if(m_mode == ED_BUILD_READY)
	{
		if(m_selBuildings.numElem() > 0)
		{
			for(int i = 0; i < m_selBuildings.numElem(); i++)
			{
				buildingSource_t* bs = m_selBuildings[i].selBuild;
				delete bs;
			}

			for(int i = 0; i < m_selBuildings.numElem(); i++)
			{
				m_selBuildings[i].selRegion->m_buildings.remove( m_selBuildings[i].selBuild );
			}

			m_selBuildings.clear();

			g_pMainFrame->NotifyUpdate();
		}
	}
	else if(m_mode == ED_BUILD_SELECTEDPOINT)
	{
		if(m_editingBuilding->points.getCount() > 1)
		{
			// don't delete first point if we're editing
			if(!m_isEditingNewBuilding && m_editingBuilding->points.getCount() == 1)
				return;

			m_editingBuilding->points.goToLast();
			m_editingBuilding->points.removeCurrent(); //removeIndex(m_editingBuilding->points.numElem()-1);
		}
		else
			CancelBuilding();
	}
}

void CUI_BuildingConstruct::RecalcSelectionCenter()
{
	BoundingBox bbox;

	for(int i = 0; i < m_selBuildings.numElem(); i++)
	{
		bbox.AddVertex( m_selBuildings[i].selBuild->modelPosition );
	}

	m_editAxis.m_position = bbox.GetCenter();
}

void CUI_BuildingConstruct::ToggleSelection( buildingSelInfo_t& bld )
{
	for(int i = 0; i < m_selBuildings.numElem(); i++)
	{
		if(	m_selBuildings[i].selRegion == bld.selRegion &&
			m_selBuildings[i].selBuild == bld.selBuild)
		{
			bld.selBuild->hide = false;
			m_selBuildings.fastRemoveIndex(i);

			RecalcSelectionCenter();
			return;
		}
	}

	m_selBuildings.append( bld );
	bld.selBuild->hide = true;

	RecalcSelectionCenter();
}

extern Vector3D g_camera_target;

void CUI_BuildingConstruct::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if (m_selectedRegion)
	{
		CHeightTileFieldRenderable* field = m_selectedRegion->m_heightfield[0];
		field->DebugRender(false, m_mouseOverTileHeight, GridSize());
	}

	if( m_selBuildings.numElem() > 0 )
	{
		for(int i = 0; i < m_selBuildings.numElem(); i++)
		{
			if(m_selBuildings[i].selBuild == m_editingBuilding)
				continue;

			Matrix4x4 wmatrix = identity4() * translate(m_selBuildings[i].selBuild->modelPosition);

			// render
			materials->SetCullMode(CULL_BACK);
			materials->SetMatrix(MATRIXMODE_WORLD, wmatrix);

			ColorRGBA oldcol = materials->GetAmbientColor();
			materials->SetAmbientColor(ColorRGBA(1,0.5,0.5,1));

			m_selBuildings[i].selBuild->model->Render(0);

			materials->SetAmbientColor(oldcol);
			materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		}

		m_editAxis.SetProps(identity3(), m_editAxis.m_position);

		float clength = length(m_editAxis.m_position-g_camera_target);
		m_editAxis.Draw(clength);
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	// only draw if building is not empty
	if(m_editingBuilding && m_editingBuilding->points.getCount() > 0 && m_editingBuilding->layerColl != NULL)
	{
		buildSegmentPoint_t extraSegment;
		extraSegment.position = ComputePlacementPointBasedOnMouse();
		extraSegment.layerId = GetSelectedLayerId();
		extraSegment.modelId = m_curModelId;
		extraSegment.scale = m_curSegmentScale;

		//
		// Finally render the dynamic preview of our building
		//
		RenderBuilding(m_editingBuilding, &extraSegment);

		// render the actual and computed points
		debugoverlay->Box3D(m_mousePoint - 0.5f, m_mousePoint + 0.5f, ColorRGBA(1,0,0,1));
		debugoverlay->Box3D(extraSegment.position-0.5f, extraSegment.position+0.5f, ColorRGBA(1,1,0,1));

		// render points of building
		for(DkLLNode<buildSegmentPoint_t>* lln = m_editingBuilding->points.goToFirst(); lln != NULL; lln = m_editingBuilding->points.goToNext())
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
	if(!m_editingBuilding->points.goToLast())
		return vec3_zero;

	// now the computations
	const buildSegmentPoint_t& end = m_editingBuilding->points.getCurrent();

	int selLayerId = GetSelectedLayerId();

	// get scaled segment length of last node
	buildLayer_t& layer = m_editingBuilding->layerColl->layers[selLayerId];
	float segLen = GetSegmentLength(layer, m_editingBuilding->order, m_curModelId) * m_curSegmentScale;

	buildSegmentPoint_t testSegPoint;
	testSegPoint.position = m_mousePoint;

	// calculate segment interations from last node to mouse position
	int numIterations = GetLayerSegmentIterations(end, testSegPoint, segLen);
	m_placeError = (numIterations == 0);

	// compute the valid placement point of segment
	Vector3D segmentDir = normalize(m_mousePoint - end.position);
	return end.position + segmentDir * floor(numIterations) * segLen;
}