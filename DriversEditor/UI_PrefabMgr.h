//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate Editor - prefab manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_PREFABMGR
#define UI_PREFABMGR

#include "level.h"
#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"

enum EPrefabManagerMode
{
	PREFABMODE_READY = 0,
	PREFABMODE_PLACEMENT,		// placing the prefab
	PREFABMODE_TILESELECTION,	// selecting the tiles which gonna be saved to prefab lev file
	PREFABMODE_CREATE_READY,
};

class CEditorLevel;

class CUI_PrefabManager : public wxPanel, public CBaseTilebasedEditor
{	
public:	
	CUI_PrefabManager( wxWindow* parent ); 
	~CUI_PrefabManager();

	// IEditorTool stuff

	void						MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  );

	void						OnKey(wxKeyEvent& event, bool bDown);
	void						OnRender();

	void						InitTool();
	void						ReloadTool();

	void						OnLevelUnload();
	void						OnLevelLoad();

	void						Update_Refresh();

	int							GetRotation();
	void						SetRotation(int rot);

protected:

	void						MakePrefabFromSelection();
	void						RefreshPrefabList();
	void						CancelPrefab();
		
	// Virtual event handlers, overide them in your derived class
	virtual void OnBeginPrefabPlacement( wxCommandEvent& event );
	virtual void OnFilterChange( wxCommandEvent& event );
	virtual void OnNewPrefabClick( wxCommandEvent& event) ;
	virtual void OnEditPrefabClick( wxCommandEvent& event );
	virtual void OnDeletePrefabClick( wxCommandEvent& event );

	IRectangle					m_tileSelection;
	EPrefabManagerMode			m_mode;

	IVector2D					m_startPoint;

	wxListBox*					m_prefabList;
	wxTextCtrl*					m_filtertext;
	wxButton*					m_newbtn;
	wxButton*					m_editbtn;
	wxButton*					m_delbtn;
	wxTextEntryDialog*			m_prefabNameDialog;

	CEditorLevel*				m_selPrefab;

	int							m_rotation;
};

#endif // UI_PREFABMGR