//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Edtior render window
//////////////////////////////////////////////////////////////////////////////////

#include "MaterialListFrame.h"

#include "IDebugOverlay.h"
#include "math/rectangle.h"
#include "math/boundingbox.h"
#include "math/math_util.h"
#include "SelectionEditor.h"

#include "EditableEntity.h"
#include "EditableSurface.h"
#include "EqBrush.h"

extern editor_user_prefs_t* g_editorCfg;

BEGIN_EVENT_TABLE(CTextureListPanel, wxPanel)
    EVT_ERASE_BACKGROUND(CTextureListPanel::OnEraseBackground)
    EVT_IDLE(CTextureListPanel::OnIdle)
	EVT_SCROLLWIN(CTextureListPanel::OnScrollbarChange)
	EVT_MOTION(CTextureListPanel::OnMouseMotion)
	EVT_MOUSEWHEEL(CTextureListPanel::OnMouseScroll)
	EVT_LEFT_DOWN(CTextureListPanel::OnMouseClick)
	EVT_SIZE(CTextureListPanel::OnSizeEvent)
END_EVENT_TABLE()

bool g_bTexturesInit = false;

CTextureListPanel::CTextureListPanel(wxWindow* parent) : wxPanel( parent, 0,0,640,480 )
{
	m_pFont = NULL;

	SetScrollbar(wxVERTICAL, 0, 8, 100);

	selection_id = -1;
	pointer_position = vec2_zero;

	m_nPreviewSize = 128;
	m_bAspectFix = true;
}

void CTextureListPanel::OnMouseMotion(wxMouseEvent& event)
{
	pointer_position.x = event.GetX();
	pointer_position.y = event.GetY();

	Redraw();
}

void CTextureListPanel::OnSizeEvent(wxSizeEvent &event)
{
	if(!IsShown() || !g_bTexturesInit)
		return;

	if(materials)
	{
		int w, h;
		g_editormainframe->GetMaxRenderWindowSize(w,h);

		materials->SetDeviceBackbufferSize(w,h);

		RefreshScrollbar();

		Redraw();
	}
}

void CTextureListPanel::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos =  GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation()/100, true);
}

void CTextureListPanel::OnScrollbarChange(wxScrollWinEvent& event)
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

void CTextureListPanel::OnMouseClick(wxMouseEvent& event)
{
	// set selection to mouse over
	selection_id = mouseover_id;

	if(selection_id == -1)
		return;

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pObject = (CBaseEditableObject*)g_pLevel->GetEditable(i);

		bool changed = false;

		for(int j = 0; j < pObject->GetSurfaceTextureCount(); j++)
		{
			if(!(pObject->GetSurfaceTexture(j)->nFlags & STFL_SELECTED))
				continue;

			 pObject->GetSurfaceTexture(j)->pMaterial = GetSelectedMaterial();
			 changed = true;
		}

		if(changed)
			pObject->UpdateSurfaceTextures();
	}
	

	g_editormainframe->UpdateAllWindows();
}

void CTextureListPanel::OnIdle(wxIdleEvent &event)
{

}

void CTextureListPanel::OnEraseBackground(wxEraseEvent& event)
{
	//Redraw();
}

void CTextureListPanel::Redraw()
{
	if(!materials)
		return;

	if(!IsShown() && !IsShownOnScreen())
		return;

	if(!m_pFont)
		m_pFont = InternalLoadFont("debug");

	g_bTexturesInit = true;

	int w, h;
	GetClientSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w, h);

	materials->GetConfiguration().wireframeMode = false;

	materials->SetAmbientColor( color4_white );

	if( materials->BeginFrame() )
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0,0,0, 1));

		if(!m_materialslist.numElem())
		{
			materials->EndFrame((HWND)GetHWND());
			return;
		}

		materials->Setup2D(w,h);

		int nLine = 0;
		int nItem = 0;

		Rectangle_t screenRect(0,0, w,h);
		screenRect.Fix();

		float scrollbarpercent = GetScrollPos(wxVERTICAL);///100.0f;

		int numItems = 0;

		float fSize = (float)m_nPreviewSize;

		for(int i = 0; i < m_filteredlist.numElem(); i++)
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
			int estimated_lines = m_filteredlist.numElem() / numItems;

			nItem = 0;

			for(int i = 0; i < m_filteredlist.numElem(); i++)
			{
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

				if( screenRect.IsIntersectsRectangle(check_rect) )
				{
					ITexture* pTex = g_pShaderAPI->GetErrorTexture();

					if(m_filteredlist[i]->GetShader())
					{
						// preload
						materials->PutMaterialToLoadingQueue( m_filteredlist[i] );

						if(m_filteredlist[i]->GetShader())
						{
							pTex = m_filteredlist[i]->GetShader()->GetBaseTexture();

							if(!pTex)
								pTex = g_pShaderAPI->GetErrorTexture();
						}
						else
							pTex = g_pShaderAPI->GetErrorTexture();
					}

					float texture_aspect = pTex->GetWidth() / pTex->GetHeight();

					float x_scale = 1.0f;
					float y_scale = 1.0f;

					if(m_bAspectFix && (pTex->GetWidth() > pTex->GetHeight()))
					{
						y_scale /= texture_aspect;
					}

					Vertex2D_t verts[] = {MAKETEXQUAD(x_offset, y_offset, x_offset + fSize*x_scale,y_offset + fSize*y_scale, 0)};
					Vertex2D_t name_line[] = {MAKETEXQUAD(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 30, 0)};
				
					BlendStateParam_t params;
					params.alphaTest = false;
					params.alphaTestRef = 1.0f;
					params.blendEnable = false;
					params.srcFactor = BLENDFACTOR_ONE;
					params.dstFactor = BLENDFACTOR_ZERO;
					params.mask = COLORMASK_ALL;
					params.blendFunc = BLENDFUNC_ADD;

					materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25,0.25,1,1), &params);

					if( check_rect.IsInRectangle( pointer_position ) )
					{
						mouseover_id = i;

						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, ColorRGBA(1,0.5f,0.5f,1), &params);

						Vertex2D verts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};
						materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(1,0,0,1));

						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25,0.1,0.25,1), &params);
					}
					else
						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, color4_white, &params);

					if(selection_id == i)
					{
						//materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, ColorRGBA(0.5f,1.0f,0.5f,1), &params);

						Vertex2D verts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};
						materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(0,1,0,1));

						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(1,0.25,0.25,1), &params);
					}

					EqString material_name = m_filteredlist[i]->GetName()+1;
					char* name_str = (char*)material_name.c_str();

					while(*name_str)
					{
						if(*name_str == '\\' || *name_str == '/')
							*name_str = '\n';

						name_str++;
					}

					Rectangle_t name_rect(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 400);

					m_pFont->DrawSetColor(color4_white);
					m_pFont->DrawTextInRect(material_name.c_str(), name_rect, 8, 8, false);

					if(m_filteredlist[i]->IsError())
						m_pFont->DrawText("BAD MATERIAL", x_offset, y_offset, 8,8,false);

					if(!m_filteredlist[i]->GetShader() || (m_filteredlist[i]->GetShader() && m_filteredlist[i]->GetShader()->IsError()))
						m_pFont->DrawText("BAD SHADER\nEDIT IT FIRST", x_offset, y_offset+10, 8,8,false);
				}

				nItem++;
			}
		}

		materials->EndFrame((HWND)GetHWND());
	}
}

void CTextureListPanel::ReloadMaterialList()
{
	selection_id = -1;

	bool no_materails = (m_materialslist.numElem() == 0);

	for(int i = 0; i < m_materialslist.numElem(); i++)
	{
		// if this material is removed before reload, remove it
		if(!materials->IsMaterialExist( m_materialslist[i]->GetName() ))
			materials->FreeMaterial( m_materialslist[i] );
	}

	// reload materials
	if(!no_materails)
		materials->ReloadAllMaterials();

	m_loadfilter.clear();

	KeyValues kv;
	if( kv.LoadFromFile("EqEditProjectSettings.CFG") )
	{
		kvkeybase_t* pSection = kv.GetRootSection();
		for(int i = 0; i < pSection->keys.numElem(); i++)
		{
			if(!stricmp(pSection->keys[i]->name, "SkipMaterialDir" ))
				m_loadfilter.append( pSection->keys[i]->values[0] );
		}
	}

	// clean materials and filter list
	m_materialslist.clear();
	m_filteredlist.clear();

	EqString base_path(EqString(g_fileSystem->GetCurrentGameDirectory()) + EqString("/") + materials->GetMaterialPath());
	base_path = base_path.Left(base_path.Length()-1);

	CheckDirForMaterials( base_path.GetData() );

	m_filteredlist.append(m_materialslist);

	RefreshScrollbar();
}

IMaterial* CTextureListPanel::GetSelectedMaterial()
{
	if(selection_id == -1)
		return materials->GetMaterial("error", true);

	return m_filteredlist[selection_id];
}

bool CTextureListPanel::CheckDirForMaterials(const char* filename_to_add)
{
	EqString dir_name = filename_to_add;
	EqString dirname = dir_name + EqString("/*.*");

	WIN32_FIND_DATAA wfd;
	HANDLE hFile;

	for(int i = 0; i < m_loadfilter.numElem(); i++)
	{
		if(dir_name.Find(m_loadfilter[i].GetData(), true) != -1)
			return false;
	}

	EqString tex_dir(EqString(g_fileSystem->GetCurrentGameDirectory()) + materials->GetMaterialPath());

	Msg("Searching directory for materials: '%s'\n", filename_to_add);

	hFile = FindFirstFileA(dirname.GetData(), &wfd);
	if(hFile != NULL)
	{
		while(1) 
		{
			if(!FindNextFileA(hFile, &wfd))
				break;

			if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(!stricmp("..", wfd.cFileName) || !stricmp(".",wfd.cFileName))
					continue;

				CheckDirForMaterials(varargs("%s/%s",dir_name.GetData(), wfd.cFileName));
			}
			else
			{
				EqString filename = dir_name + "/" + wfd.cFileName;

				EqString ext = filename.Path_Extract_Ext();

				if(ext == "mat")
				{
					filename = filename.Path_Strip_Ext();
					filename = filename.Right(filename.Length() - tex_dir.Length());

					IMaterial* pMaterial = materials->GetMaterial( filename.GetData(), true);
					if(pMaterial)
					{
						IMatVar* pVar = pMaterial->FindMaterialVar("showineditor");
						if(pVar && pVar->GetInt() == 0)
							continue;
					}

					int id = m_materialslist.append(pMaterial);

					if(!stricmp(g_editorCfg->default_material.GetData(), filename.GetData()))
					{
						selection_id = id;
					}
				}
			}
		}

		FindClose(hFile);
	}

	return true;
}

void CTextureListPanel::SetPreviewParams(int preview_size, bool bAspectFix)
{
	if(m_nPreviewSize != preview_size)
		RefreshScrollbar();

	m_nPreviewSize = preview_size;
	m_bAspectFix = bAspectFix;
}

void CTextureListPanel::RefreshScrollbar()
{
	int w, h;
	GetSize(&w, &h);

	wxRect rect = GetScreenRect();
	w = rect.GetWidth();
	h = rect.GetHeight();

	int numItems = 0;
	int nItem = 0;

	float fSize = (float)m_nPreviewSize;

	for(int i = 0; i < m_filteredlist.numElem(); i++)
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
		int estimated_lines = m_filteredlist.numElem() / numItems;

		SetScrollbar(wxVERTICAL, 0, 8, estimated_lines + 10);
	}
}

void CTextureListPanel::ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsedMaterials, bool bSortByDate)
{
	if(!IsShown() || !g_bTexturesInit)
		return;

	//if(!(m_filter != filter || m_filterTags != tags))
	//	return;

	// remember filter string
	m_filter = filter;
	m_filterTags = tags;

	selection_id = -1;

	m_filteredlist.clear();

	DkList<IMaterial*> show_materials;

	if(bOnlyUsedMaterials)
	{
		for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
		{
			for(int j = 0; j < g_pLevel->GetEditable(i)->GetSurfaceTextureCount(); j++)
			{
				if(g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->pMaterial != NULL && !g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->pMaterial->IsError())
					show_materials.addUnique(g_pLevel->GetEditable(i)->GetSurfaceTexture(j)->pMaterial);
			}
		}
			
	}
	else
	{
		show_materials.resize(m_materialslist.numElem());
		show_materials.append(m_materialslist);
	}

	if(filter.Length() == 0)
	{
		m_filteredlist.append(show_materials);

		RefreshScrollbar();

		return;
	}

	if(filter.length() > 0)
	{
		for(int i = 0; i < show_materials.numElem(); i++)
		{
			int foundIndex = xstrfind((char*)show_materials[i]->GetName(), (char*)filter.c_str().AsChar());

			if(foundIndex > 0)
				m_filteredlist.append(show_materials[i]);
		}
	}

	RefreshScrollbar();

	Redraw();
}


//---------------------------------------------------------------
// CTextureListPanel holder
//---------------------------------------------------------------

BEGIN_EVENT_TABLE(CTextureListWindow, wxFrame)
	//EVT_SIZE(CTextureListWindow::OnSize)
	EVT_TEXT(-1, CTextureListWindow::OnFilterTextChanged)
	EVT_CLOSE(CTextureListWindow::OnClose)

	EVT_CHECKBOX(-1, CTextureListWindow::OnChangePreviewParams)
	EVT_CHOICE(-1, CTextureListWindow::OnChangePreviewParams)

END_EVENT_TABLE()

CTextureListWindow::CTextureListWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame( parent, -1, title,pos, size, wxDEFAULT_FRAME_STYLE | wxMAXIMIZE)
{
	/*
	wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);

	m_pSplitter = new wxSplitterWindow(this, -1, wxPoint(0, 0), wxSize(640, 480), wxSP_3D);
	m_pSplitter->SetSashGravity(1.0);
	m_pSplitter->SetMinimumPaneSize(20); // Smalest size the

	m_texPanel = new CTextureListPanel(m_pSplitter);
	wxPanel* pPanel2 = new wxPanel(m_pSplitter, 0,0,640,50, wxTAB_TRAVERSAL);

	m_pSplitter->SplitHorizontally(m_texPanel, pPanel2, -100);

	box->Add(m_pSplitter, 1, wxEXPAND,0);

	box->SetSizeHints( this );

	m_pSplitter->SplitHorizontally(m_texPanel, pPanel2);

	m_filtertext = new wxTextCtrl(pPanel2, 0, wxEmptyString, wxPoint(5,5),wxSize(320,20));
	m_onlyusedmaterials = new wxCheckBox(pPanel2, 1, DKLOC("TOKEN_ONLYUSEDMATERIALS", "Only used materials"), wxPoint(5,30),wxSize(320,20));

	SetSizer(box);
	*/

	Maximize(true);

	wxBoxSizer* bSizer34;
	bSizer34 = new wxBoxSizer( wxVERTICAL );
	
	m_texPanel = new CTextureListPanel(this);

	bSizer34->Add( m_texPanel, 1, wxEXPAND | wxALL, 0 );

	wxPanel* pSettingsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,250 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer38;
	bSizer38 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer9;
	sbSizer9 = new wxStaticBoxSizer( new wxStaticBox( pSettingsPanel, wxID_ANY, wxT("Search and Filter") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer9->Add( new wxStaticText( pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_filtertext = new wxTextCtrl( pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	fgSizer9->Add( m_filtertext, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer9->Add( new wxStaticText( pSettingsPanel, wxID_ANY, wxT("Tags"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pTags = new wxTextCtrl( pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	fgSizer9->Add( m_pTags, 0, wxALL, 5 );
	
	
	sbSizer9->Add( fgSizer9, 0, wxEXPAND, 5 );
	
	m_onlyusedmaterials = new wxCheckBox( pSettingsPanel, wxID_ANY, wxT("SHOW ONLY USED MATERIALS"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer9->Add( m_onlyusedmaterials, 0, wxALL, 5 );
	
	m_pSortByDate = new wxCheckBox( pSettingsPanel, wxID_ANY, wxT("SORT BY DATE"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer9->Add( m_pSortByDate, 0, wxALL, 5 );
	
	
	bSizer38->Add( sbSizer9, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer10;
	sbSizer10 = new wxStaticBoxSizer( new wxStaticBox( pSettingsPanel, wxID_ANY, wxT("Display") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer10->SetFlexibleDirection( wxBOTH );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer10->Add( new wxStaticText( pSettingsPanel, wxID_ANY, wxT("Preview size"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_pPreviewSizeChoices[] = { wxT("128"), wxT("256"), wxT("512") };
	int m_pPreviewSizeNChoices = sizeof( m_pPreviewSizeChoices ) / sizeof( wxString );
	m_pPreviewSize = new wxChoice( pSettingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPreviewSizeNChoices, m_pPreviewSizeChoices, 0 );
	m_pPreviewSize->SetSelection( 0 );
	fgSizer10->Add( m_pPreviewSize, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_pAspectCorrection = new wxCheckBox( pSettingsPanel, wxID_ANY, wxT("ASPECT CORRECTION"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pAspectCorrection->SetValue(true); 
	fgSizer10->Add( m_pAspectCorrection, 0, wxALL, 5 );
	
	sbSizer10->Add( fgSizer10, 0, wxEXPAND, 5 );
	
	bSizer38->Add( sbSizer10, 0, wxEXPAND, 5 );
	
	pSettingsPanel->SetSizer( bSizer38 );
	pSettingsPanel->Layout();
	bSizer34->Add( pSettingsPanel, 0, wxALL|wxEXPAND, 0 );
	
	this->SetSizer( bSizer34 );
	this->Layout();


}

void CTextureListWindow::OnChangePreviewParams(wxCommandEvent& event)
{
	int nSel = 0;

	if(m_pPreviewSize->GetSelection() >= 0)
		nSel = m_pPreviewSize->GetSelection();

	int values[] = 
	{
		128,
		256,
		512,
	};

	m_texPanel->SetPreviewParams(values[nSel], m_pAspectCorrection->GetValue());

	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

void CTextureListWindow::OnFilterTextChanged(wxCommandEvent& event)
{
	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}
/*
void CTextureListWindow::OnSize(wxSizeEvent &event)
{
	Layout();
}*/

void CTextureListWindow::OnClose(wxCloseEvent& event)
{
	Hide();
}

IMaterial* CTextureListWindow::GetSelectedMaterial()
{
	return m_texPanel->GetSelectedMaterial();
}

void CTextureListWindow::ReloadMaterialList()
{
	m_texPanel->ReloadMaterialList();
}

CTextureListPanel*	CTextureListWindow::GetTexturePanel()
{
	return m_texPanel;
}