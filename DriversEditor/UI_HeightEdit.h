//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Heightmap editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_HEIGHTEDIT_H
#define UI_HEIGHTEDIT_H

#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"

class CUI_HeightEdit;

struct matAtlas_t
{
	matAtlas_t() :
		atlas(nullptr),material(nullptr)
	{
	}

	matAtlas_t(CTextureAtlas* atl, IMaterial* mat) :
		atlas(atl),material(mat)
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
		if(a.entry != NULL)
			return a.material == b.material && a.entryIdx == b.entryIdx;
		else
			return a.material == b.material;
	}
};

// texture list panel
class CMaterialAtlasList : public wxPanel
{
public:
    CMaterialAtlasList(CUI_HeightEdit* parent);

	void					OnSizeEvent(wxSizeEvent &event);
	void					OnIdle(wxIdleEvent &event);
	void					OnEraseBackground(wxEraseEvent& event);
	void					OnScrollbarChange(wxScrollWinEvent& event);
		
	void					OnMouseMotion(wxMouseEvent& event);
	void					OnMouseScroll(wxMouseEvent& event);
	void					OnMouseClick(wxMouseEvent& event);

	void					ReloadMaterialList();

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

	bool					CheckDirForMaterials(const char* filename_to_add);

	DkList<matAtlas_t>		m_materialslist;
	DkList<matAtlasElem_t>	m_filteredlist;

	DkList<EqString>		m_loadfilter;

	IEqFont*				m_pFont;

	Vector2D				pointer_position;
	int						selection_id;
	int						mouseover_id;

	wxString				m_filter;
	wxString				m_filterTags;

	bool					m_bAspectFix;
	int						m_nPreviewSize;

	IEqSwapChain*			m_swapChain;
	CUI_HeightEdit*			m_heightEdit;
};

class CUI_HeightEdit;

enum EWhatPaintFlags
{
	HEDIT_PAINT_MATERIAL	= ( 1 << 0 ),
	HEDIT_PAINT_ROTATION	= ( 1 << 1 ),
	HEDIT_PAINT_FLAGS		= ( 1 << 2 ),
};

// rx, ry are real in processing
typedef bool (*TILEPAINTFUNC)(int rx, int ry, int px, int py, CUI_HeightEdit* edit, CHeightTileField* field, hfieldtile_t* tile, int flags, float percent);

enum EEditMode
{
	HEDIT_ADD,
	HEDIT_SMOOTH,
	HEDIT_SET,
};

//---------------------------------------------------------------
// CTextureListPanel holder
//---------------------------------------------------------------
class CUI_HeightEdit : public wxPanel, public CBaseTilebasedEditor
{
	friend class CMaterialAtlasList;

public:
	CUI_HeightEdit(wxWindow* parent);

	//void						OnSize(wxSizeEvent &event);
	void						OnClose(wxCloseEvent& event);

	void						OnFilterTextChanged(wxCommandEvent& event);
	void						OnChangePreviewParams(wxCommandEvent& event);

	void						OnLayerSpinChanged(wxCommandEvent& event);

	IMaterial*					GetSelectedMaterial();

	void						ReloadMaterialList();

	CMaterialAtlasList*			GetTexturePanel();

	int							GetRotation();
	void						SetRotation(int rot);

	int							GetRadius();
	void						SetRadius(int radius);

	int							GetHeightfieldFlags();
	void						SetHeightfieldFlags(int flags);

	int							GetAddHeight();
	void						SetHeight(int height);

	EEditMode					GetEditMode();
	int							GetEditorPaintFlags();

	int							GetSelectedAtlasIndex() const;

	// IEditorTool stuff

	void						MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  );

	void						ProcessMouseEvents( wxMouseEvent& event );
	void						OnKey(wxKeyEvent& event, bool bDown);
	void						OnRender();

	void						InitTool();

	void						OnLevelUnload();

	void						Update_Refresh();

	void						PaintHeightfieldRadius(int px, int py, TILEPAINTFUNC func);

	void						PaintHeightfieldGlobal(int px, int py, TILEPAINTFUNC func, float percent = 1.0f);

	void						PaintHeightfieldLine(int x0, int y0, int x1, int y1, TILEPAINTFUNC func);

	DECLARE_EVENT_TABLE()
protected:

	enum
	{
		SETROT_0 = 1000,
		SETROT_270,
		SETROT_90,
		SETROT_180
	};

	CMaterialAtlasList*			m_texPanel;

	wxCheckBox*					m_paintMaterial;
	wxCheckBox*					m_paintRotation;
	wxCheckBox*					m_paintFlags;

	wxCheckBox*					m_detached;
	wxCheckBox*					m_addWall;
	wxCheckBox*					m_wallCollide;
	wxCheckBox*					m_noCollide;

	wxCheckBox*					m_drawHelpers;
	wxCheckBox*					m_quadratic;
	wxRadioBox*					m_heightPaintMode;

	wxSpinCtrl*					m_height;
	wxSpinCtrl*					m_radius;
	wxSpinCtrl*					m_layer;
	wxPanel*					m_pSettingsPanel;
	wxTextCtrl*					m_filtertext;
	wxTextCtrl*					m_pTags;

	wxCheckBox*					m_onlyusedmaterials;
	wxCheckBox*					m_pSortByDate;
	wxChoice*					m_pPreviewSize;
	wxCheckBox*					m_pAspectCorrection;

	int							m_rotation;

	//int							m_radiusVal;
	//int							m_heightVal;
};

#endif // UI_HEIGHTEDIT_H