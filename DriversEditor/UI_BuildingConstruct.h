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

#include "EditorLevel.h"

#include "editaxis.h"

enum EBuildingEditMode
{
	ED_BUILD_READY = 0,

	ED_BUILD_MOVEMENT,

	ED_BUILD_BEGUN,				// 1st point
	ED_BUILD_WAITFORSELECT,		// next points

	ED_BUILD_SELECTEDPOINT,		// selected points

	ED_BUILD_DONE,
};

//-----------------------------------------------------------------------------

class CBuildingLayerEditDialog : public wxDialog, CPointerDropTarget
{
	enum
	{
		LAYEREDIT_NEW = 1000,
		LAYEREDIT_DELETE,
		LAYEREDIT_CHOOSEMODEL,
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

	void OnBtnsClick( wxCommandEvent& event );

	void ChangeSize( wxCommandEvent& event );
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
	wxSpinCtrl* m_size;
	wxChoice* m_typeSel;
	wxButton* m_btnChoose;

	// what we modifying
	buildLayerColl_t*		m_layerColl;
	buildLayer_t*			m_selLayer;
	Vector2D				m_mousePos;
	int						m_mouseoverItem;
	int						m_selectedItem;
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

	void					LoadLayerCollections( const char* levelName );
	void					SaveLayerCollections( const char* levelName );

	void					RemoveAllLayerCollections();

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

struct buildingSelInfo_t
{
	buildingSource_t*		selBuild;
	CEditorLevelRegion*		selRegion;
};

class CUI_BuildingConstruct : public wxPanel, public CBaseTilebasedEditor
{
public:

	CUI_BuildingConstruct( wxWindow* parent ); 
	~CUI_BuildingConstruct();

	
	void		ProcessMouseEvents( wxMouseEvent& event );
	void		MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos );

	void		CancelBuilding();
	void		CompleteBuilding();

	void		ClearSelection();
	void		DeleteSelection();

	void		EditSelectedBuilding();

	void		Update_Refresh();

	void		OnKey(wxKeyEvent& event, bool bDown);

	Vector3D	ComputePlacementPointBasedOnMouse();

	void		RecalcSelectionCenter();
	void		ToggleSelection( buildingSelInfo_t& ref );

	void		OnRender();

	void		InitTool();

	void		OnLevelLoad();
	void		OnLevelSave();
	void		OnLevelUnload();
	
protected:

	// Virtual event handlers, overide them in your derived class
	void OnFilterChange( wxCommandEvent& event );
	void OnCreateClick( wxCommandEvent& event );
	void OnEditClick( wxCommandEvent& event );
	void OnDeleteClick( wxCommandEvent& event );

	CEditAxisXYZ				m_editAxis;

	int							m_mouseLastY;
		
	CBuildingLayerEditDialog*	m_layerEditDlg;
	CBuildingLayerList*			m_layerCollList;

	wxPanel*					m_pSettingsPanel;
	wxTextCtrl*					m_filtertext;
	wxChoice*					m_pPreviewSize;
	wxButton*					m_button5;
	wxButton*					m_button3;
	wxButton*					m_button8;

	wxCheckBox*					m_tiledPlacement;

	Vector3D					m_mousePoint;
	bool						m_placeError;

	EBuildingEditMode			m_mode;
	buildingSource_t*			m_editingBuilding;
	buildingSource_t			m_editingCopy;		// for cancel reason
	bool						m_isEditingNewBuilding;

	int							m_curLayerId;
	float						m_curSegmentScale;

	DkList<buildingSelInfo_t>	m_selBuildings;

	Vector3D					m_dragPos;
	Vector3D					m_dragOffs;
	bool						m_isSelecting;
	int							m_draggedAxes;
};

#endif // UI_BUILDINGCOSTRUCT_H