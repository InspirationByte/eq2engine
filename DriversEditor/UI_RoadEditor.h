//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - road straights and intersections placement
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_ROADEDITOR
#define UI_ROADEDITOR

#include "level.h"
#include "EditorHeader.h"
#include "BaseTilebasedEditor.h"
#include "Font.h"

class CUI_RoadEditor : public wxPanel, public CBaseTilebasedEditor
{	
public:	
	CUI_RoadEditor( wxWindow* parent ); 
	~CUI_RoadEditor();

	int							GetRoadType();
	void						SetRoadType(int type);

	int							GetRotation();
	void						SetRotation(int rot);

	void						OnRotationOrTypeTextChanged(wxCommandEvent& event);

	// IEditorTool stuff

	void						MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  );

	void						OnKey(wxKeyEvent& event, bool bDown);
	void						OnRender();

	void						InitTool();
	void						ReloadTool();
	void						ShutdownTool();

	void						Update_Refresh();

	void						PaintLine(int x0, int y0, int x1, int y1);
	void						PaintPointGlobal(int x, int y, int direction);

protected:

	wxChoice*					m_typeSel;
	wxCheckBox*					m_parking;
	wxChoice*					m_rotationSel;

	int							m_rotation;
	int							m_type;

	TexAtlasEntry_t*			m_trafficDir;
	TexAtlasEntry_t*			m_trafficDirVar;
	TexAtlasEntry_t*			m_trafficParking;
	TexAtlasEntry_t*			m_pavement;
};

#endif // UI_ROADEDITOR