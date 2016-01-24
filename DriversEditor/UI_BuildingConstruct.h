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

enum ELayerType
{
	LAYER_TEXTURE = 0,
	LAYER_MODEL,
	LAYER_CORNER_MODEL
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
	CLevelModel*			model;
	IMaterial*				material;
	TexAtlasEntry_t*		atlEntry;
};

struct buildLayerCollection_t
{
	buildLayerCollection_t()
	{
	}

	EqString				name;
	DkList<buildLayer_t>	layers;
};

//------------------------------------------------------------

class CBuildingLayerEditDialog : public wxDialog 
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

	void UpdateSelection();

protected:

	void OnIdle(wxIdleEvent &event){Redraw();}
	void OnEraseBackground(wxEraseEvent& event) {}
	void OnMouseMotion(wxMouseEvent& event);

	void Redraw();

	void OnBtnsClick( wxCommandEvent& event );

	void ChangeHeight( wxCommandEvent& event );
	void ChangeRepeat( wxSpinEvent& event );
	void ChangeInterval( wxSpinEvent& event );
	void ChangeType( wxCommandEvent& event );

	IEqSwapChain*	m_pSwapChain;
	IEqFont*		m_pFont;

	wxStaticBoxSizer* m_propertyBox;

	wxPanel* m_renderPanel;
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
	buildLayerCollection_t	m_layerColl;
	buildLayer_t*			m_selLayer;
	Vector2D				m_mousePos;
	int						m_mouseoverItem;
	int						m_selectedItem;
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

	wxPanel*			m_modelPicker;
	wxPanel*			m_pSettingsPanel;
	wxTextCtrl*			m_filtertext;
	wxChoice*			m_pPreviewSize;
	wxButton*			m_button5;
	wxButton*			m_button3;
	wxButton*			m_button8;
};

#endif // UI_BUILDINGCOSTRUCT_H