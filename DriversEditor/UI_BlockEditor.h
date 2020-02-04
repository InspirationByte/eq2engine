#pragma once
//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - occluder placement
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_BLOCKEDITOR_H
#define UI_BLOCKEDITOR_H

#include "level.h"
#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"

#include "editaxis.h"

enum EBlockEditorTools
{
	BlockEdit_Selection = 0,
	BlockEdit_Brush,
	BlockEdit_Polygon,
	BlockEdit_VertexManip,
	BlockEdit_Clipper,
};

class CUI_BlockEditor : public wxPanel, public CBaseTilebasedEditor
{
	friend class CMaterialAtlasList;
	DECLARE_EVENT_TABLE()
public:
	CUI_BlockEditor(wxWindow* parent);
	~CUI_BlockEditor();

	void				ConstructToolbars(wxBoxSizer* addTo);
	void				OnFilterTextChanged(wxCommandEvent& event);
	void				OnChangePreviewParams(wxCommandEvent& event);

	void				ProcessAllMenuCommands(wxCommandEvent& event);

	// IEditorTool stuff

	void				MouseEventOnTile(wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos);
	void				ProcessMouseEvents(wxMouseEvent& event);

	void				OnKey(wxKeyEvent& event, bool bDown);
	void				OnRender();

	void				InitTool();
	void				OnLevelUnload();

	void				Update_Refresh();

protected:

	wxPanel* m_settingsPanel;
	CMaterialAtlasList* m_texPanel;

	wxPanel*					m_pSettingsPanel;
	wxTextCtrl*					m_filtertext;
	wxTextCtrl*					m_pTags;

	wxCheckBox*					m_onlyusedmaterials;
	wxCheckBox*					m_pSortByDate;
	wxChoice*					m_pPreviewSize;
	wxCheckBox*					m_pAspectCorrection;

	//----------------------------------------------

	Vector3D					m_cursorPos;
	EBlockEditorTools			m_selectedTool;
};

#endif // UI_BLOCKEDITOR_H