//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Edtior render window
//////////////////////////////////////////////////////////////////////////////////

#ifndef MATERIALLISTFRAME_H
#define MATERIALLISTFRAME_H

#include "EditorHeader.h"
#include "wx_inc.h"
#include "Font.h"
#include "EditorCamera.h"

// texture list panel
class CTextureListPanel : public wxPanel
{
public:
    CTextureListPanel(wxWindow* parent);

	void				OnSizeEvent(wxSizeEvent &event);
	void				OnIdle(wxIdleEvent &event);
	void				OnEraseBackground(wxEraseEvent& event);
	void				OnScrollbarChange(wxScrollWinEvent& event);
		
	void				OnMouseMotion(wxMouseEvent& event);
	void				OnMouseScroll(wxMouseEvent& event);
	void				OnMouseClick(wxMouseEvent& event);

	void				ReloadMaterialList();

	IMaterial*			GetSelectedMaterial();

	void				Redraw();

	void				ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsedMaterials, bool bSortByDate);
	void				SetPreviewParams(int preview_size, bool bAspectFix);

	void				RefreshScrollbar();

	DECLARE_EVENT_TABLE()
protected:

	bool				CheckDirForMaterials(const char* filename_to_add);

	DkList<IMaterial*>	m_materialslist;
	DkList<IMaterial*>	m_filteredlist;
	DkList<EqString>	m_loadfilter;

	IEqFont*			m_pFont;

	Vector2D			pointer_position;
	int					selection_id;
	int					mouseover_id;

	wxString			m_filter;
	wxString			m_filterTags;

	bool				m_bAspectFix;
	int					m_nPreviewSize;

	IEqSwapChain*		m_swapChain;
};

//---------------------------------------------------------------
// CTextureListPanel holder
//---------------------------------------------------------------
class CTextureListWindow : public wxFrame
{
public:
	CTextureListWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size);

	//void				OnSize(wxSizeEvent &event);
	void				OnClose(wxCloseEvent& event);

	void				OnFilterTextChanged(wxCommandEvent& event);
	void				OnChangePreviewParams(wxCommandEvent& event);

	IMaterial*			GetSelectedMaterial();

	void				ReloadMaterialList();

	CTextureListPanel*	GetTexturePanel();

	DECLARE_EVENT_TABLE()
protected:

	CTextureListPanel*	m_texPanel;

	//wxSplitterWindow*	m_pSplitter;
	wxTextCtrl*			m_filtertext;
	wxCheckBox*			m_onlyusedmaterials;

	wxTextCtrl*			m_pTags;
	wxCheckBox*			m_pSortByDate;
	wxChoice*			m_pPreviewSize;
	wxCheckBox*			m_pAspectCorrection;
};

#endif // MATERIALLISTFRAME_H