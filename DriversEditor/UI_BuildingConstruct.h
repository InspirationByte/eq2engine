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

class CBuildingLayerEditDialog : public wxDialog 
{
public:
	CBuildingLayerEditDialog( wxWindow* parent ); 
	~CBuildingLayerEditDialog();

protected:
	wxPanel* m_renderPanel;
	wxPanel* m_panel18;

	wxButton* m_button16;
	wxButton* m_button14;
	wxButton* m_button15;
	wxButton* m_button18;
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