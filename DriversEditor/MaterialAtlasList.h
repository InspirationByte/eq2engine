//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Material list for editor
//////////////////////////////////////////////////////////////////////////////////

#ifndef MATERIALATLASLIST_H
#define MATERIALATLASLIST_H

#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"
#include "GenericImageListRenderer.h"

struct matAtlas_t
{
	matAtlas_t() :
		atlas(nullptr), material(nullptr)
	{
	}

	matAtlas_t(CTextureAtlas* atl, IMaterial* mat) :
		atlas(atl), material(mat)
	{
	}

	void Free()
	{
		materials->FreeMaterial(material);
		delete atlas;
	}

	CTextureAtlas*	atlas;
	IMaterial*		material;
};

struct matAtlasElem_t
{
	matAtlasElem_t() :
		entry(nullptr), entryIdx(0), material(nullptr)
	{
	}

	matAtlasElem_t(TexAtlasEntry_t* ent, int idx, IMaterial* mat) :
		entry(ent), entryIdx(idx), material(mat)
	{
	}

	TexAtlasEntry_t*	entry;
	int					entryIdx;
	IMaterial*			material;

	static bool CompareByMaterial(const matAtlasElem_t& a, const matAtlasElem_t& b)
	{
		return a.material == b.material;
	}

	static bool CompareByMaterialWithAtlasIdx(const matAtlasElem_t& a, const matAtlasElem_t& b)
	{
		if (a.entry != NULL)
			return a.material == b.material && a.entryIdx == b.entryIdx;
		else
			return a.material == b.material;
	}
};

// texture list panel
class CMaterialAtlasList : public wxPanel, CGenericImageListRenderer<matAtlasElem_t>
{
public:
	CMaterialAtlasList(wxWindow* parent);

	void					ReloadMaterialList(bool allowAtlases = true);

	int						GetSelectedAtlas() const;
	IMaterial*				GetSelectedMaterial() const;

	void					SelectMaterial(IMaterial* pMaterial, int atlasIdx);

	void					Redraw();

	void					ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsedMaterials, bool bSortByDate);
	void					UpdateAndFilterList();
	void					SetPreviewParams(int preview_size, bool bAspectFix);

	void					RefreshScrollbar();

	DECLARE_EVENT_TABLE()
protected:


	void					OnSizeEvent(wxSizeEvent &event);
	void					OnIdle(wxIdleEvent &event);
	void					OnEraseBackground(wxEraseEvent& event);
	void					OnScrollbarChange(wxScrollWinEvent& event);

	void					OnMouseMotion(wxMouseEvent& event);
	void					OnMouseScroll(wxMouseEvent& event);
	void					OnMouseClick(wxMouseEvent& event);

	Rectangle_t				ItemGetImageCoordinates(matAtlasElem_t& item);
	ITexture*				ItemGetImage(matAtlasElem_t& item);
	void					ItemPostRender(int id, matAtlasElem_t& item, const IRectangle& rect);

	bool					CheckDirForMaterials(const char* filename_to_add);

	DkList<matAtlas_t>		m_materialslist;

	DkList<EqString>		m_loadfilter;

	wxString				m_filter;
	wxString				m_filterTags;

	int						m_nPreviewSize;

	IEqSwapChain*			m_swapChain;

	bool					m_allowAtlases;
};

#endif // MATERIALATLASLIST_H