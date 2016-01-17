//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightmap editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#include "UI_HeightEdit.h"

#include "EditorMain.h"

#include "IDebugOverlay.h"
#include "math/rectangle.h"
#include "math/boundingbox.h"
#include "math/math_util.h"
#include "TextureView.h"

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

BEGIN_EVENT_TABLE(CMaterialAtlasList, wxPanel)
    EVT_ERASE_BACKGROUND(CMaterialAtlasList::OnEraseBackground)
    EVT_IDLE(CMaterialAtlasList::OnIdle)
	EVT_SCROLLWIN(CMaterialAtlasList::OnScrollbarChange)
	EVT_MOTION(CMaterialAtlasList::OnMouseMotion)
	EVT_MOUSEWHEEL(CMaterialAtlasList::OnMouseScroll)
	EVT_LEFT_DOWN(CMaterialAtlasList::OnMouseClick)
	EVT_SIZE(CMaterialAtlasList::OnSizeEvent)
END_EVENT_TABLE()

bool g_bTexturesInit = false;

CMaterialAtlasList::CMaterialAtlasList(CUI_HeightEdit* parent) : wxPanel( parent, 0,0,640,480 )
{
	m_pFont = NULL;
	m_swapChain = NULL;

	SetScrollbar(wxVERTICAL, 0, 8, 100);

	selection_id = -1;
	pointer_position = vec2_zero;

	m_nPreviewSize = 128;
	m_bAspectFix = true;
	m_heightEdit = parent;
}

void CMaterialAtlasList::OnMouseMotion(wxMouseEvent& event)
{
	pointer_position.x = event.GetX();
	pointer_position.y = event.GetY();

	Redraw();
}

void CMaterialAtlasList::OnSizeEvent(wxSizeEvent &event)
{
	if(!IsShown() || !g_bTexturesInit)
		return;

	if(materials)
	{
		int w, h;
		//g_pMainFrame->GetMaxRenderWindowSize(w,h);
		g_pMainFrame->GetSize(&w, &h);

		//materials->SetDeviceBackbufferSize(w,h);

		RefreshScrollbar();

		Redraw();
	}
}

void CMaterialAtlasList::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos =  GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation()/100, true);
}

void CMaterialAtlasList::OnScrollbarChange(wxScrollWinEvent& event)
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

void CMaterialAtlasList::OnMouseClick(wxMouseEvent& event)
{
	// set selection to mouse over
	selection_id = mouseover_id;

	if(selection_id == -1)
		return;
}

void CMaterialAtlasList::OnIdle(wxIdleEvent &event)
{

}

void CMaterialAtlasList::OnEraseBackground(wxEraseEvent& event)
{
	//Redraw();
}

void CMaterialAtlasList::Redraw()
{
	if(!materials)
		return;

	if(!m_swapChain)
		m_swapChain = materials->CreateSwapChain((HWND)GetHWND());

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
			materials->EndFrame(m_swapChain);
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
			//int estimated_lines = m_filteredlist.numElem() / numItems;

			nItem = 0;

			for(int i = 0; i < m_filteredlist.numElem(); i++)
			{
				matAtlasElem_t& elem = m_filteredlist[i];

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

					float texture_aspect = 1.0f;

					if(!elem.material->IsError())
					{
						// preload
						materials->PutMaterialToLoadingQueue( elem.material );

						if(elem.material->GetState() == MATERIAL_LOAD_OK && elem.material->GetBaseTexture())
							pTex = elem.material->GetBaseTexture();
						else
							pTex = g_pShaderAPI->GetErrorTexture();

						if(pTex) // TODO: atlas aspect, size, etc
							texture_aspect = pTex->GetWidth() / pTex->GetHeight();
					}

					float x_scale = 1.0f;
					float y_scale = 1.0f;

					if(m_bAspectFix && (pTex->GetWidth() > pTex->GetHeight()))
					{
						y_scale /= texture_aspect;
					}

					Vertex2D_t verts[] = {MAKETEXQUAD(x_offset, y_offset, x_offset + fSize*x_scale,y_offset + fSize*y_scale, 0)};
					Vertex2D_t name_line[] = {MAKETEXQUAD(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 25, 0)};
				
					EqString material_name = elem.material->GetName();

					if(elem.entry)
					{
						verts[0].m_vTexCoord = elem.entry->rect.GetLeftTop(); //Vector2D(0, 0)),
						verts[1].m_vTexCoord = elem.entry->rect.GetLeftBottom(); //Vector2D(0, 1)),
						verts[2].m_vTexCoord = elem.entry->rect.GetRightTop(); //Vector2D(1, 0)),
						verts[3].m_vTexCoord = elem.entry->rect.GetRightBottom(); //Vector2D(1, 1))
					}


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

						Vertex2D rectVerts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};

						materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(1,0,0,1));
						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(0.25,0.1,0.25,1), &params);
					}
					else
						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts,4, pTex, color4_white, &params);

					if(selection_id == i)
					{
						Vertex2D rectVerts[] = {MAKETEXRECT(x_offset, y_offset, x_offset + fSize,y_offset + fSize, 0)};

						materials->DrawPrimitives2DFFP(PRIM_LINE_STRIP, rectVerts, elementsOf(rectVerts), NULL, ColorRGBA(0,1,0,1));
						materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, ColorRGBA(1,0.25,0.25,1), &params);
					}

					char* name_str = (char*)material_name.c_str();

					while(*name_str)
					{
						if(*name_str == '\\' || *name_str == '/')
							*name_str = '\n';

						name_str++;
					}

					Rectangle_t name_rect(x_offset, y_offset+fSize, x_offset + fSize,y_offset + fSize + 400);

					eqFontStyleParam_t fontParam;
					fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
					fontParam.textColor = ColorRGBA(1,1,1,1);

					m_pFont->RenderText(material_name.c_str(), name_rect.vleftTop, fontParam);

					if(elem.material->IsError())
						m_pFont->RenderText("BAD MATERIAL", Vector2D(x_offset, y_offset), fontParam);

					if(!elem.material->GetBaseTexture())
						m_pFont->RenderText("no texture", Vector2D(x_offset, y_offset+10), fontParam);

					if(elem.entry)
					{
						fontParam.textColor.w *= 0.5f;
						m_pFont->RenderText(elem.entry->name, Vector2D(x_offset, y_offset), fontParam);
					}
				}

				nItem++;
			}
		}

		materials->EndFrame(m_swapChain);
	}
}

void CMaterialAtlasList::ReloadMaterialList()
{
	// reset
	selection_id = -1;

	bool no_materails = (m_materialslist.numElem() == 0);

	for(int i = 0; i < m_materialslist.numElem(); i++)
	{
		// if this material is removed before reload, remove it
		if(!materials->IsMaterialExist( m_materialslist[i].material->GetName() ))
		{
			m_materialslist[i].Free();
			m_materialslist.fastRemoveIndex(i);
			i--;
			continue;
		}

		delete m_materialslist[i].atlas;
		m_materialslist[i].atlas = NULL;
	}

	// reload materials
	if(!no_materails)
		materials->ReloadAllMaterials();

	m_loadfilter.clear();

	KeyValues kv;
	if( kv.LoadFromFile("EqEditProjectSettings.cfg") )
	{
		kvkeybase_t* pSection = kv.GetRootSection();
		for(int i = 0; i < pSection->keys.numElem(); i++)
		{
			if(!stricmp(pSection->keys[i]->name, "SkipMaterialDir" ))
			{
				EqString pathStr(pSection->keys[i]->values[0]);
				pathStr.Path_FixSlashes();

				Msg("Add ignore filter: '%s'\n", pathStr.c_str());

				m_loadfilter.append( pathStr.c_str() );
			}
		}
	}

	// clean materials and filter list
	m_materialslist.clear();
	m_filteredlist.clear();

	EqString base_path(EqString(GetFileSystem()->GetCurrentGameDirectory()) + EqString("/") + materials->GetMaterialPath());
	base_path = base_path.Left(base_path.GetLength()-1);

	CheckDirForMaterials( base_path.GetData() );

	UpdateAndFilterList();

	RefreshScrollbar();
}

void CMaterialAtlasList::UpdateAndFilterList()
{
	selection_id = -1;

	m_filteredlist.clear();

	for(int i = 0; i < m_materialslist.numElem(); i++)
	{
		// has atlas?
		if( m_materialslist[i].atlas )
		{
			CTextureAtlas* atl = m_materialslist[i].atlas;

			// enumerate all atlas entries and add them
			for(int j = 0; j < atl->GetEntryCount(); j++)
			{
				TexAtlasEntry_t* entry = atl->GetEntry(j);

				// try filter atlas entries along with material path
				if( m_filter.Length() > 0 )
				{
					EqString matName(m_materialslist[i].material->GetName());
					EqString entryName(entry->name);

					int foundIndex = matName.Find(m_filter.c_str());
					int foundEntry = entryName.Find(m_filter.c_str());

					if(foundIndex == -1 && foundEntry == -1)
						continue;
				}

				// add material with atlas entry
				m_filteredlist.append( matAtlasElem_t(entry, j, m_materialslist[i].material) );
			}
				
		}
		else // just add material
		{
			// filter material path
			if( m_filter.Length() > 0 )
			{
				EqString matName(m_materialslist[i].material->GetName());

				int foundIndex = matName.Find(m_filter.c_str());

				if(foundIndex == -1)
					continue;
			}

			// add just material
			m_filteredlist.append( matAtlasElem_t(NULL, 0, m_materialslist[i].material) );
		}
	}

	RefreshScrollbar();
	Redraw();
}

int CMaterialAtlasList::GetSelectedAtlas() const
{
	if(selection_id == 0)
		return NULL;

	return m_filteredlist[selection_id].entryIdx;
}

IMaterial* CMaterialAtlasList::GetSelectedMaterial() const
{
	if(selection_id == -1)
		return NULL;

	return m_filteredlist[selection_id].material;
}

void CMaterialAtlasList::SelectMaterial(IMaterial* pMaterial, int atlasIdx)
{
	if(pMaterial == NULL)
	{
		selection_id = -1;
		return;
	}

	matAtlasElem_t tmp;
	tmp.material = pMaterial;
	tmp.entryIdx = atlasIdx;

	selection_id = m_filteredlist.findIndex( tmp, matAtlasElem_t::CompareByMaterialWithAtlasIdx );
}

bool CMaterialAtlasList::CheckDirForMaterials(const char* filename_to_add)
{
	EqString dir_name = filename_to_add;
	EqString dirname = dir_name + CORRECT_PATH_SEPARATOR + EqString("*.*");

	WIN32_FIND_DATAA wfd;
	HANDLE hFile;

	for(int i = 0; i < m_loadfilter.numElem(); i++)
	{
		if(dir_name.Find(m_loadfilter[i].GetData(), false) != -1)
			return false;
	}

	EqString tex_dir(EqString(GetFileSystem()->GetCurrentGameDirectory()) + materials->GetMaterialPath());

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

				CheckDirForMaterials(varargs("%s%c%s",dir_name.GetData(), CORRECT_PATH_SEPARATOR ,wfd.cFileName));
			}
			else
			{
				EqString filename = dir_name + CORRECT_PATH_SEPARATOR + wfd.cFileName;

				EqString ext = filename.Path_Extract_Ext();

				if(ext == "mat")
				{
					filename = filename.Path_Strip_Ext();
					filename = filename.Right(filename.GetLength() - tex_dir.GetLength());

					IMaterial* pMaterial = materials->FindMaterial( filename.GetData(), true);
					if(pMaterial)
					{
						IMatVar* pVar = pMaterial->FindMaterialVar("showineditor");
						if(pVar && pVar->GetInt() == 0)
							continue;
					}

					EqString atlFileName((materials->GetMaterialPath() + filename + ".atlas"));

					CTextureAtlas* atlas = new CTextureAtlas(); // try load new atlas

					// try load atlas
					if( !atlas->Load( atlFileName.c_str(), atlFileName.c_str() ) )
					{
						delete atlas;
						atlas = nullptr;
					}

					// finally
					m_materialslist.append( matAtlas_t(atlas, pMaterial) );

					pMaterial->Ref_Grab();
				}
			}
		}

		FindClose(hFile);
	}

	selection_id = -1;

	return true;
}

void CMaterialAtlasList::SetPreviewParams(int preview_size, bool bAspectFix)
{
	if(m_nPreviewSize != preview_size)
		RefreshScrollbar();

	m_nPreviewSize = preview_size;
	m_bAspectFix = bAspectFix;
}

void CMaterialAtlasList::RefreshScrollbar()
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

void CMaterialAtlasList::ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsedMaterials, bool bSortByDate)
{
	if(!IsShown() || !g_bTexturesInit)
		return;

	if(!(m_filter != filter || m_filterTags != tags))
		return;

	// remember filter string
	m_filter = filter;
	m_filterTags = tags;

	UpdateAndFilterList();
}


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
	
	m_radius = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, -32767, 32767, 0 );
	fgSizer7->Add( m_radius, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("LAYER"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_layer = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 0, ENGINE_REGION_MAX_HFIELDS-1, 0 );
	fgSizer7->Add( m_layer, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("Atlas Tex."), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );

	bSizer3->Add( fgSizer7, 0, 0, 5 );
	
	m_drawHelpers = new wxCheckBox( this, wxID_ANY, wxT("Draw helpers"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_drawHelpers, 1, wxALL, 5 );
	
	
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
}

void CUI_HeightEdit::OnLayerSpinChanged(wxCommandEvent& event)
{
	m_selectedHField = m_layer->GetValue();
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

int	CUI_HeightEdit::GetAddHeight()
{
	return m_height->GetValue();
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

EEditMode CUI_HeightEdit::GetEditMode()
{
	return (EEditMode)m_heightPaintMode->GetSelection();
}

int CUI_HeightEdit::GetEditorPaintFlags()
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
	g_pEditorActionObserver->BeginModify( field );
	
	bool result = func(rx, ry, px, py, edit, field, tile, flags, percent);

	if(result &&
		(rx <= 0 || ry <= 0 ||
		rx >= field->m_sizew || ry >= field->m_sizeh))
		return true;

	return result;
}

void CUI_HeightEdit::PaintHeightfieldGlobal(int gx, int gy, TILEPAINTFUNC func, float percent)
{
	CLevelRegion* pReg = NULL;
	IVector2D local;
	g_pGameWorld->m_level.GlobalToLocalPoint(IVector2D(gx,gy), local, &pReg);

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

					if(WRAP_PaintFieldModify(rx, ry, ix, iy, this, field, tile, func, GetEditorPaintFlags(), percent))
						field->SetChanged();
					//else
					//	g_pEditorActionObserver->CancelModify();
				}
			}
		}
	}
}

void CUI_HeightEdit::PaintHeightfieldRadius(int px, int py, TILEPAINTFUNC func)
{
	//int p_x = m_selectedRegion->GetHField(m_selectedHField)->m_posidx_x;
	//int p_y = m_selectedRegion->GetHField(m_selectedHField)->m_posidx_y;

	// update neighbours
	//int fields_offs_x[8] = NEIGHBOR_OFFS_XDX(p_x, 1);
	//int fields_offs_y[8] = NEIGHBOR_OFFS_YDY(p_y, 1);

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
				CHeightTileFieldRenderable* field = m_selectedRegion->GetHField(m_selectedHField);

				if(!field)
					continue;

				hfieldtile_t* tile = field->GetTileAndNeighbourField(px+rx, py+ry, (CHeightTileField**)&field);

				if(!tile)
					continue;
				
				int prx = px+rx;
				int pry = py+ry;

				// calculate tile address
				int ix = ROLLING_VALUE(prx, field->m_sizew);
				int iy = ROLLING_VALUE(pry, field->m_sizeh);

				if(WRAP_PaintFieldModify(rx, ry, ix, iy, this, field, tile, func, GetEditorPaintFlags(), 1.0f))
					field->SetChanged();
			}
		}
	}
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

/*
bool FlagPaintFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags)
{
	if(	edit->GetRotation() == tile->rotatetex &&
		tile->flags == edit->GetHeightfieldFlags())
		return false;

	tile->flags = edit->GetHeightfieldFlags();

	g_pMainFrame->NotifyUpdate();

	return true;
}
*/

bool NullTexPaintFunc(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent)
{
	if(flags & HEDIT_PAINT_MATERIAL)
	{
		g_pMainFrame->NotifyUpdate();
		return field->SetPointMaterial(px, py, NULL);
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
	else if(mode == HEDIT_SMOOTH)
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

void CUI_HeightEdit::ProcessMouseEvents( wxMouseEvent& event )
{
	CBaseTilebasedEditor::ProcessMouseEvents(event);

	if(!event.MiddleIsDown() && event.ControlDown())
	{
		int mouse_wheel = event.GetWheelRotation();
		SetRadius(GetRadius()+sign(mouse_wheel));
	}
}

void CUI_HeightEdit::PaintHeightfieldLine(int x0, int y0, int x1, int y1, TILEPAINTFUNC func)
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

		PaintHeightfieldGlobal(floor(x),floor(y), func, percentage);

        x += xInc;
        y += yInc;
    }

    return;
}

/*
void CUI_HeightEdit::PaintHeightfieldLine(int x0, int y0, int x1, int y1, TILEPAINTFUNC func)
{
	int cx0 = x0;
	int cy0 = y0;

	// direction
	int dx = abs(x1-x0);
	int dy = abs(y1-y0);

	int sx = x0<x1 ? 1 : -1;
	int sy = y0<y1 ? 1 : -1; 

	int err = (dx>dy ? dx : -dy) / 2;
	int e2;

	for(;;)
	{
		float interp = / dx;

		// paint using global points
		PaintHeightfieldGlobal(cx0, cy0, func);

		if (cx0==x1 && cy0==y1)
			break;

		
		
		e2 = err;

		if (e2 >-dx)
		{
			err -= dy;
			cx0 += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			cy0 += sy; 
		}
	}
}
*/

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
				PaintHeightfieldRadius(tx, ty, HeightPaintUpFunc);
			}
			else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
			{
				m_globalTile_pointSet = false;
				PaintHeightfieldRadius(tx, ty, HeightPaintDnFunc);
			}
			else if(event.ButtonUp(wxMOUSE_BTN_MIDDLE))
			{
				PaintHeightfieldLine(	m_globalTile_lineStart.x,m_globalTile_lineStart.y,
										m_globalTile_lineEnd.x, m_globalTile_lineEnd.y,
										HeightPaintUpFunc);
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
			PaintHeightfieldRadius(tx, ty, TexPaintFunc);
		}
		else if(event.ButtonUp(wxMOUSE_BTN_MIDDLE))
		{
			// line
			PaintHeightfieldLine(	m_globalTile_lineStart.x,m_globalTile_lineStart.y,
									m_globalTile_lineEnd.x, m_globalTile_lineEnd.y, 
									TexPaintFunc);
		}
		else if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
		{
			m_globalTile_pointSet = false;
			PaintHeightfieldRadius(tx, ty, NullTexPaintFunc);
		}
	}

	if(event.ButtonUp())
	{
		g_pEditorActionObserver->EndModify();
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

	if(m_selectedRegion)
	{
		CHeightTileFieldRenderable* thisField = m_selectedRegion->GetHField(m_selectedHField);

		/*
		IVector2D hfieldPos = m_selectedRegion->GetHField(m_selectedHField)->m_regionPos;

		int dx[8] = NEIGHBOR_OFFS_XDX(hfieldPos.x, 1);
		int dy[8] = NEIGHBOR_OFFS_YDY(hfieldPos.y, 1);

		// draw surrounding regions helpers
		for(int i = 0; i < 8; i++)
		{
			CLevelRegion* nregion = g_pGameWorld->m_level.GetRegionAt(IVector2D(dx[i], dy[i]));

			if(nregion && nregion->GetHField())
			{
				CHeightTileFieldRenderable* nField = nregion->GetHField();

				nField->DebugRender(m_drawHelpers->GetValue(), m_mouseOverTileHeight);
			}
		}*/

		thisField->DebugRender(m_drawHelpers->GetValue(), m_mouseOverTileHeight);
	}

	CBaseTilebasedEditor::OnRender();

	if(m_selectedRegion)
	{
		hfieldtile_t* tile = m_selectedRegion->GetHField(m_selectedHField)->GetTile(m_mouseOverTile.x, m_mouseOverTile.y);

		if(tile)
		{
			Vector3D box_pos(m_mouseOverTile.x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, m_mouseOverTile.y*HFIELD_POINT_SIZE);

			box_pos += m_selectedRegion->GetHField(m_selectedHField)->m_position - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

			debugoverlay->Text3D(box_pos, -1, ColorRGBA(1,1,1,1), "layer: %d", m_selectedHField);
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
	m_texPanel->Refresh();
}
