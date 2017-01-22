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

class CUI_PrefabManager : public wxPanel, public CBaseTilebasedEditor
{	
public:	
	CUI_PrefabManager( wxWindow* parent ); 
	~CUI_PrefabManager();

	// IEditorTool stuff

	void						MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  );

	void						ProcessMouseEvents( wxMouseEvent& event );
	void						OnKey(wxKeyEvent& event, bool bDown);
	void						OnRender();

	void						InitTool();
	void						ReloadTool();

	void						Update_Refresh();

protected:

};

#endif // UI_PREFABMGR