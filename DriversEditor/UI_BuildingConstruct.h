//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Building editor for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_BUILDINGCOSTRUCT_H
#define UI_BUILDINGCOSTRUCT_H

#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"
#include "DragDropObjects.h"
#include "GenericImageListRenderer.h"

enum ELayerType
{
	LAYER_TEXTURE = 0,
	LAYER_MODEL,
	LAYER_CORNER_MODEL
};

class CLayerModel : public CEditorPreviewable
{
public:
	PPMEM_MANAGED_OBJECT()

	CLayerModel();
	~CLayerModel();

	void			RefreshPreview();

	CLevelModel*	m_model;
	EqString		m_name;
};

struct buildLayer_t
{
	buildLayer_t() 
		: height(1.0f), type(LAYER_TEXTURE), repeatTimes(0), repeatInterval(0), model(nullptr), material(nullptr), atlEntry(nullptr)
	{
	}

	float					height;			// height in units
	int						type;			// ELayerType
	int						repeatTimes;	// height repeat times
	int						repeatInterval;	// repeat after repeatInterval times
	CLayerModel*			model;
	IMaterial*				material;
	TexAtlasEntry_t*		atlEntry;
};

struct buildLayerColl_t
{
	buildLayerColl_t()
	{
	}

	EqString				name;
	DkList<buildLayer_t>	layers;
};

//-----------------------------------------------------------------------------

class CBuildingLayerEditDialog : public wxDialog, CPointerDropTarget
{
	enum
	{
		LAYEREDIT_NEW = 1000,
		LAYEREDIT_DELETE,
		LAYEREDIT_CHOOSEMODEL,
		LAYEREDIT_TOGGLEPREVIEW
	};

public:
	CBuildingLayerEditDialog( wxWindow* parent ); 
	~CBuildingLayerEditDialog();

	void				UpdateSelection();
	void				SetLayerCollection(buildLayerColl_t* coll);
	buildLayerColl_t*	GetLayerCollection() const;

protected:

	void OnIdle(wxIdleEvent &event) {Redraw();}
	void OnEraseBackground(wxEraseEvent& event) {}
	void OnMouseMotion(wxMouseEvent& event);
	void OnMouseScroll(wxMouseEvent& event);
	void OnMouseClick(wxMouseEvent& event);

	void Redraw();
	void RenderList();
	void RenderPreview();

	void OnBtnsClick( wxCommandEvent& event );

	void ChangeHeight( wxCommandEvent& event );
	void ChangeRepeat( wxSpinEvent& event );
	void ChangeInterval( wxSpinEvent& event );
	void ChangeType( wxCommandEvent& event );

	void OnScrollbarChange(wxScrollWinEvent& event);

	bool OnDropPoiner(wxCoord x, wxCoord y, void* ptr, EDragDropPointerType type);

	IEqSwapChain*	m_pSwapChain;
	IEqFont*		m_pFont;

	wxStaticBoxSizer* m_propertyBox;

	wxPanel*			m_renderPanel;

	wxPanel* m_panel18;
	wxButton* m_newBtn;
	wxButton* m_delBtn;
	wxTextCtrl* m_height;
	wxSpinCtrl* m_repeat;
	wxSpinCtrl* m_interval;
	wxChoice* m_typeSel;
	wxButton* m_btnChoose;
	wxButton* m_previewBtn;

	// what we modifying
	buildLayerColl_t*		m_layerColl;
	buildLayer_t*			m_selLayer;
	Vector2D				m_mousePos;
	int						m_mouseoverItem;
	int						m_selectedItem;
	bool					m_preview;
};

//-----------------------------------------------------------------------------

// texture list panel
class CBuildingLayerList : public wxPanel, CGenericImageListRenderer<buildLayerColl_t*>
{
	friend class CUI_BuildingConstruct;

public:
    CBuildingLayerList(CUI_BuildingConstruct* parent);

	void					ReloadList();

	buildLayerColl_t*		GetSelectedLayerColl() const;

	void					Redraw();

	void					ChangeFilter(const wxString& filter);
	void					UpdateAndFilterList();
	void					SetPreviewParams(int preview_size, bool bAspectFix);

	void					RefreshScrollbar();

	buildLayerColl_t*		CreateCollection();
	void					DeleteCollection(buildLayerColl_t* coll);

	DECLARE_EVENT_TABLE()
protected:

	void						OnSizeEvent(wxSizeEvent &event);
	void						OnIdle(wxIdleEvent &event);
	void						OnEraseBackground(wxEraseEvent& event);
	void						OnScrollbarChange(wxScrollWinEvent& event);
		
	void						OnMouseMotion(wxMouseEvent& event);
	void						OnMouseScroll(wxMouseEvent& event);
	void						OnMouseClick(wxMouseEvent& event);

	Rectangle_t					ItemGetImageCoordinates( buildLayerColl_t*& item );
	ITexture*					ItemGetImage( buildLayerColl_t*& item );
	void						ItemPostRender( int id, buildLayerColl_t*& item, const IRectangle& rect );

	DkList<buildLayerColl_t*>	m_layerCollections;

	wxString					m_filter;

	int							m_nPreviewSize;

	IEqSwapChain*				m_swapChain;
	CUI_BuildingConstruct*		m_bldConstruct;
};

//-----------------------------------------------------------------------------

class CUI_BuildingConstruct : public wxPanel, public CBaseTilebasedEditor
{
public:

	CUI_BuildingConstruct( wxWindow* parent ); 
	~CUI_BuildingConstruct();

	void		MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos );

	void		InitTool();
	void		ShutdownTool();

	void		Update_Refresh();

	void		OnKey(wxKeyEvent& event, bool bDown);
	void		OnRender();
	
protected:

	// Virtual event handlers, overide them in your derived class
	void OnFilterChange( wxCommandEvent& event );
	void OnCreateClick( wxCommandEvent& event );
	void OnEditClick( wxCommandEvent& event );
	void OnDeleteClick( wxCommandEvent& event );
		
	CBuildingLayerEditDialog* m_layerEditDlg;
	CBuildingLayerList*	m_layerCollList;

	wxPanel*			m_pSettingsPanel;
	wxTextCtrl*			m_filtertext;
	wxChoice*			m_pPreviewSize;
	wxButton*			m_button5;
	wxButton*			m_button3;
	wxButton*			m_button8;
};

#endif // UI_BUILDINGCOSTRUCT_H