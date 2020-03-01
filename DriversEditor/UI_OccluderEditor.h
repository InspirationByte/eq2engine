//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - occluder placement
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_OCCLUDEREDITOR_H
#define UI_OCCLUDEREDITOR_H

#include "level.h"
#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"

#include "editaxis.h"

enum EOccluderPlaceMode
{
	ED_OCCL_READY = 0,
	ED_OCCL_POINT1,
	ED_OCCL_POINT2,
	ED_OCCL_HEIGHT,
	ED_OCCL_DONE,
};

struct selectedOccluder_t
{
	int occIdx;
	CEditorLevelRegion* region;
};

class CUI_OccluderEditor : public wxPanel, public CBaseTilebasedEditor
{	
public:	
	CUI_OccluderEditor( wxWindow* parent ); 
	~CUI_OccluderEditor();

	// IEditorTool stuff

	void				MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  );

	void				ProcessMouseEvents( wxMouseEvent& event );
	void				MouseTranslateEvents(wxMouseEvent& event, const Vector3D& ray_start, const Vector3D& ray_dir);

	void				OnKey(wxKeyEvent& event, bool bDown);
	void				OnRender();

	void				InitTool();
	void				OnLevelUnload();

	void				Update_Refresh();

protected:

	void				SelectOccluder(const Vector3D& rayStart, const Vector3D& rayDir);
	void				DeleteSelection();
	void				ClearSelection();

	DkList<selectedOccluder_t>	m_selection;

	CEditGizmo					m_editPoints[3];
	levOccluderLine_t			m_newOccl;

	int							m_currentGizmo;
	int							m_mode;

	CEditorLevelRegion*			m_placeRegion;
};

#endif // UI_OCCLUDEREDITOR_H