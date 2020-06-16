//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Material list for editor
//////////////////////////////////////////////////////////////////////////////////

#include "MaterialAtlasList.h"
#include "DragDropObjects.h"
#include "EditorMain.h"
#include "FontCache.h"

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

CMaterialAtlasList::CMaterialAtlasList(wxWindow* parent) : wxPanel(parent, -1, wxPoint(0, 0), wxSize(640, 480))
{
	m_swapChain = NULL;

	SetScrollbar(wxVERTICAL, 0, 8, 100);

	m_nPreviewSize = 128;

	m_mouseOver = -1;
	m_selection = -1;
}

void CMaterialAtlasList::OnMouseMotion(wxMouseEvent& event)
{
	m_mousePos.x = event.GetX();
	m_mousePos.y = event.GetY();

	if (event.Dragging() && GetSelectedMaterial())
	{
		if (m_mouseOver == -1)
			return;

		CPointerDataObject dropData;
		dropData.SetCompositeMaterial(&m_filteredList[m_mouseOver]);
		wxDropSource dragSource(dropData, this);

		wxDragResult result = dragSource.DoDragDrop(wxDrag_CopyOnly);
	}

	Redraw();
}

void CMaterialAtlasList::OnSizeEvent(wxSizeEvent &event)
{
	if (!IsShown() || !g_bTexturesInit)
		return;

	if (materials)
	{
		int w, h;
		//g_pMainFrame->GetMaxRenderWindowSize(w,h);
		g_pMainFrame->GetSize(&w, &h);

		//materials->SetDeviceBackbufferSize(w,h);

		Redraw();
		RefreshScrollbar();
	}
}

void CMaterialAtlasList::OnMouseScroll(wxMouseEvent& event)
{
	int scroll_pos = GetScrollPos(wxVERTICAL);

	SetScrollPos(wxVERTICAL, scroll_pos - event.GetWheelRotation() / 100, true);
	Redraw();
}

void CMaterialAtlasList::OnScrollbarChange(wxScrollWinEvent& event)
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

void CMaterialAtlasList::OnMouseClick(wxMouseEvent& event)
{
	// set selection to mouse over
	m_selection = m_mouseOver;
}

void CMaterialAtlasList::OnIdle(wxIdleEvent &event)
{
}

void CMaterialAtlasList::OnEraseBackground(wxEraseEvent& event)
{
}

Rectangle_t CMaterialAtlasList::ItemGetImageCoordinates(matAtlasElem_t& item)
{
	if (item.entry)
		return item.entry->rect;
	else
		return Rectangle_t(0, 0, 1, 1);
}

ITexture* CMaterialAtlasList::ItemGetImage(matAtlasElem_t& item)
{
	// preload
	materials->PutMaterialToLoadingQueue(item.material);

	if (item.material->GetState() == MATERIAL_LOAD_OK && item.material->GetBaseTexture())
		return item.material->GetBaseTexture();
	else
		return g_pShaderAPI->GetErrorTexture();
}

void CMaterialAtlasList::ItemPostRender(int id, matAtlasElem_t& item, const IRectangle& rect)
{
	Rectangle_t name_rect(rect.GetLeftBottom(), rect.GetRightBottom() + Vector2D(0, 25));
	name_rect.Fix();

	Vector2D lt = name_rect.GetLeftTop();
	Vector2D rb = name_rect.GetRightBottom();

	Vertex2D_t name_line[] = { MAKETEXQUAD(lt.x, lt.y, rb.x, rb.y, 0) };

	ColorRGBA nameBackCol = ColorRGBA(0.25, 0.25, 1, 1);

	if (m_selection == id)
		nameBackCol = ColorRGBA(1, 0.25, 0.25, 1);
	else if (m_mouseOver == id)
		nameBackCol = ColorRGBA(0.25, 0.1, 0.25, 1);

	// draw name panel
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, name_line, 4, NULL, nameBackCol);

	EqString material_name = item.material->GetName();
	material_name.Replace(CORRECT_PATH_SEPARATOR, '\n');

	eqFontStyleParam_t fontParam;
	fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
	fontParam.textColor = ColorRGBA(1, 1, 1, 1);

	// render text
	m_debugFont->RenderText(material_name.c_str(), name_rect.vleftTop, fontParam);

	if (item.material->IsError())
		m_debugFont->RenderText("BAD MATERIAL", rect.GetLeftTop(), fontParam);
	else if (!item.material->GetBaseTexture())
		m_debugFont->RenderText("no texture", rect.GetLeftTop() + Vector2D(0, 10), fontParam);

	// show atlas entry name
	if (item.entry)
	{
		fontParam.textColor.w *= 0.5f;
		m_debugFont->RenderText(item.entry->name, rect.GetLeftTop(), fontParam);
		fontParam.textColor.w = 1.0f;
	}
}

void CMaterialAtlasList::Redraw()
{
	if (!materials)
		return;

	if (!m_swapChain)
		m_swapChain = materials->CreateSwapChain((HWND)GetHWND());

	if (!IsShown() && !IsShownOnScreen())
		return;

	g_bTexturesInit = true;

	int w, h;
	GetClientSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w, h);

	if (materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true, true, false, ColorRGBA(0, 0, 0, 1));

		if (!m_materialslist.numElem())
		{
			materials->EndFrame(m_swapChain);
			return;
		}

		int scrollPos = GetScrollPos(wxVERTICAL);

		IRectangle screenRect(0, 0, w, h);
		screenRect.Fix();

		RedrawItems(screenRect, scrollPos, m_nPreviewSize);

		materials->EndFrame(m_swapChain);
	}
}

void CMaterialAtlasList::ReloadMaterialList(bool allowAtlases)
{
	m_allowAtlases = allowAtlases;

	// reset
	m_mouseOver = -1;
	m_selection = -1;

	bool no_materails = (m_materialslist.numElem() == 0);

	for (int i = 0; i < m_materialslist.numElem(); i++)
	{
		// if this material is removed before reload, remove it
		if (!materials->IsMaterialExist(m_materialslist[i].material->GetName()))
		{
			m_materialslist[i].Free();
			m_materialslist.fastRemoveIndex(i);
			i--;
			continue;
		}
		m_materialslist[i].atlas = NULL;
	}

	// reload materials
	if (!no_materails)
		materials->ReloadAllMaterials();

	m_loadfilter.clear();

	KeyValues kv;
	if (kv.LoadFromFile("EqEditProjectSettings.cfg", SP_MOD))
	{
		Msg("Adding material ignore filters from 'EqEditProjectSettings.cfg'\n");

		kvkeybase_t* pSection = kv.GetRootSection();
		for (int i = 0; i < pSection->keys.numElem(); i++)
		{
			if (!stricmp(pSection->keys[i]->name, "SkipMaterialDir"))
			{
				EqString pathStr(KV_GetValueString(pSection->keys[i]));
				pathStr.Path_FixSlashes();

				m_loadfilter.append(pathStr);
			}
		}
	}

	// clean materials and filter list
	m_materialslist.clear();
	m_filteredList.clear();

	//EqString base_path(EqString(g_fileSystem->GetCurrentGameDirectory()) + EqString("/") + materials->GetMaterialPath());
	//base_path = base_path.Left(base_path.Length() - 1);

	CheckDirForMaterials(materials->GetMaterialPath());

	UpdateAndFilterList();

	RefreshScrollbar();
}

void CMaterialAtlasList::UpdateAndFilterList()
{
	m_mouseOver = -1;
	m_selection = -1;

	m_filteredList.clear();

	for (int i = 0; i < m_materialslist.numElem(); i++)
	{
		// has atlas?
		if (m_materialslist[i].atlas)
		{
			if (m_allowAtlases)
			{
				CTextureAtlas* atl = m_materialslist[i].atlas;

				// enumerate all atlas entries and add them
				for (int j = 0; j < atl->GetEntryCount(); j++)
				{
					TexAtlasEntry_t* entry = atl->GetEntry(j);

					// try filter atlas entries along with material path
					if (m_filter.Length() > 0)
					{
						EqString matName(m_materialslist[i].material->GetName());
						EqString entryName(entry->name);

						int foundIndex = matName.Find(m_filter.c_str());
						int foundEntry = entryName.Find(m_filter.c_str());

						if (foundIndex == -1 && foundEntry == -1)
							continue;
					}

					// add material with atlas entry
					m_filteredList.append(matAtlasElem_t(entry, j, m_materialslist[i].material));
				}
			}
		}
		else // just add material
		{
			// filter material path
			if (m_filter.Length() > 0)
			{
				EqString matName(m_materialslist[i].material->GetName());

				int foundIndex = matName.Find(m_filter.c_str());

				if (foundIndex == -1)
					continue;
			}

			// add just material
			m_filteredList.append(matAtlasElem_t(NULL, 0, m_materialslist[i].material));
		}
	}

	RefreshScrollbar();
	Redraw();
}

int CMaterialAtlasList::GetSelectedAtlas() const
{
	if (m_selection == -1)
		return NULL;

	return m_filteredList[m_selection].entryIdx;
}

IMaterial* CMaterialAtlasList::GetSelectedMaterial() const
{
	if (m_selection == -1)
		return NULL;

	return m_filteredList[m_selection].material;
}

void CMaterialAtlasList::SelectMaterial(IMaterial* pMaterial, int atlasIdx)
{
	if (pMaterial == NULL)
	{
		m_selection = -1;
		return;
	}

	matAtlasElem_t tmp;
	tmp.material = pMaterial;
	tmp.entryIdx = atlasIdx;

	m_selection = m_filteredList.findIndex(tmp, matAtlasElem_t::CompareByMaterialWithAtlasIdx);
}

bool CMaterialAtlasList::CheckDirForMaterials(const char* filename_to_add)
{
	EqString searchPath = CombinePath(2, filename_to_add, "*.*");

	for (int i = 0; i < m_loadfilter.numElem(); i++)
	{
		Msg("searchPath: %s vs %s\n", searchPath.c_str(), m_loadfilter[i].c_str());

		if (searchPath.Find(m_loadfilter[i].c_str(), false) != -1)
			return false;
	}

	CFileSystemFind materialsFind(searchPath.c_str(), SP_MOD);

	Msg("Searching directory for materials: '%s'\n", filename_to_add);

	int materials_str_length = strlen(materials->GetMaterialPath());

	while (materialsFind.Next())
	{
		if (materialsFind.IsDirectory())
		{
			if (!stricmp("..", materialsFind.GetPath()) || !stricmp(".", materialsFind.GetPath()))
				continue;

			// search recursively
			CheckDirForMaterials( CombinePath(2, filename_to_add, materialsFind.GetPath()).c_str() );
		}
		else
		{
			EqString fileName = CombinePath(2, filename_to_add, materialsFind.GetPath());

			if (fileName.Path_Extract_Ext() != "mat")
				continue;

			IMaterial* pMaterial = materials->GetMaterial( fileName.Path_Strip_Ext().c_str() + materials_str_length);
			if (pMaterial)
			{
				IMatVar* pVar = pMaterial->FindMaterialVar("showineditor");
				if (pVar && pVar->GetInt() == 0)
				{
					pMaterial->Ref_Grab(); // do safely
					materials->FreeMaterial(pMaterial);
					continue;
				}
			}

			CTextureAtlas* atlas = pMaterial->GetAtlas(); // try load new atlas

			// finally
			m_materialslist.append(matAtlas_t(atlas, pMaterial));

			pMaterial->Ref_Grab();
		}
	}

	m_selection = -1;

	return true;
}

void CMaterialAtlasList::SetPreviewParams(int preview_size, bool bAspectFix)
{
	m_nPreviewSize = preview_size;

	Redraw();
	RefreshScrollbar();
}

void CMaterialAtlasList::RefreshScrollbar()
{
	// This is now recalculated in RedrawItems
	if (m_itemsPerLine > 0)
	{
		int maxLines = (m_filteredList.numElem() / m_itemsPerLine) + 1;
		SetScrollbar(wxVERTICAL, 0, true, maxLines, true);
	}
}

void CMaterialAtlasList::ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsedMaterials, bool bSortByDate)
{
	if (!IsShown() || !g_bTexturesInit)
		return;

	if (!(m_filter != filter || m_filterTags != tags))
		return;

	// remember filter string
	m_filter = filter;
	m_filterTags = tags;

	UpdateAndFilterList();
}