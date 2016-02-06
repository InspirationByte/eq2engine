//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - model placement
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_LEVELMODELS_H
#define UI_LEVELMODELS_H

#include "level.h"
#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"
#include "GenericImageListRenderer.h"


#include "editaxis.h"

enum EModelEditMode
{
	MEDIT_PLACEMENT = 0,
	MEDIT_TRANSLATE,
	MEDIT_ROTATE,
};

// selection info for region relocation
struct refselectioninfo_t
{
	regionObject_t*	selRef;
	CLevelRegion*	selRegion;
};

class CReplaceModelDialog;

// texture list panel
class CModelListRenderPanel : public wxPanel, CGenericImageListRenderer<CLevObjectDef*>
{
public:
    CModelListRenderPanel(wxWindow* parent);

	void					OnSizeEvent(wxSizeEvent &event);
	void					OnIdle(wxIdleEvent &event);
	void					OnEraseBackground(wxEraseEvent& event);
	void					OnScrollbarChange(wxScrollWinEvent& event);
		
	void					OnMouseMotion(wxMouseEvent& event);
	void					OnMouseScroll(wxMouseEvent& event);
	void					OnMouseClick(wxMouseEvent& event);

	void					OnContextEvent(wxCommandEvent& event);

	void					RebuildPreviewShots();
	void					ReleaseModelPreviews();

	CLevObjectDef*			GetSelectedModelContainer();
	CLevelModel*			GetSelectedModel();
	void					SelectModel(CLevelModel* pMaterial);

	void					Redraw();

	void					ChangeFilter(const wxString& filter, const wxString& tags, bool bOnlyUsed, bool bSortByDate);
	void					SetPreviewParams(int preview_size, bool bAspectFix);

	void					RefreshScrollbar();

	void					AddModel(CLevObjectDef* container);
	void					RemoveModel(CLevObjectDef* container);
	void					RefreshLevelModels();

	DECLARE_EVENT_TABLE()
protected:

	Rectangle_t				ItemGetImageCoordinates( CLevObjectDef*& item );
	ITexture*				ItemGetImage( CLevObjectDef*& item );
	void					ItemPostRender( int id, CLevObjectDef*& item, const IRectangle& rect );

	wxMenu*					m_contextMenu;

	wxString				m_filter;
	wxString				m_filterTags;

	bool					m_bAspectFix;
	int						m_nPreviewSize;

	IEqSwapChain*			m_swapChain;
};

class CUI_LevelModels : public wxPanel, public CBaseTilebasedEditor
{	
	friend class CReplaceModelDialog;

public:	
	CUI_LevelModels( wxWindow* parent ); 
	~CUI_LevelModels();

	void						OnButtons(wxCommandEvent& event);
	void						OnFilterTextChanged(wxCommandEvent& event);
	void						OnChangePreviewParams(wxCommandEvent& event);

	int							GetRotation();
	void						SetRotation(int rot);

	void						OnRotationTextChanged(wxCommandEvent& event);

	void						RefreshModelReplacement();

	// IEditorTool stuff

	void						MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos );

	void						MousePlacementEvents( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos );

	void						MouseTranslateEvents( wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir );
	void						MouseRotateEvents( wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir );

	void						ProcessMouseEvents( wxMouseEvent& event );
	void						OnKey(wxKeyEvent& event, bool bDown);
	void						OnRender();

	void						InitTool();
	void						ShutdownTool();

	void						OnLevelUnload();
	void						OnLevelLoad();

	void						Update_Refresh();

	void						RecalcSelectionCenter();
	void						ToggleSelection( refselectioninfo_t& ref );
	void						ClearSelection();
	void						DeleteSelection();

protected:

	CEditAxisXYZ				m_editAxis;

	CModelListRenderPanel*		m_modelPicker;
	CReplaceModelDialog*		m_modelReplacement;

	DkList<refselectioninfo_t>	m_selRefs;

	wxPanel*					m_pSettingsPanel;
	wxCheckBox*					m_tiledPlacement;
	wxTextCtrl*					m_filtertext;

	wxCheckBox*					m_showLevelModels;
	wxCheckBox*					m_showObjects;

	wxCheckBox*					m_onlyusedmaterials;
	wxChoice*					m_pPreviewSize;

	int							m_rotation;

	int							m_last_tx;
	int							m_last_ty;
	Vector3D					m_lastpos;

	Vector3D					m_dragPos;

	Vector3D					m_dragOffs;

	bool						m_isSelecting;
	int							m_draggedAxes;

	EModelEditMode				m_editMode;
};

#endif // UI_LEVELMODELS_H