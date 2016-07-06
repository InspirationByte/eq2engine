//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Tile-based editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASETILEBASEDEDITOR_H
#define BASETILEBASEDEDITOR_H

#include "EditorHeader.h"
#include "IEditorTool.h"
#include "region.h"
#include "heightfield.h"

class CBaseTilebasedEditor : public IEditorTool
{
public:

	CBaseTilebasedEditor();

	virtual void				ProcessMouseEvents( wxMouseEvent& event );
	virtual void				OnRender();

	virtual void				ShutdownTool() {}

	virtual void				OnLevelUnload();

	virtual void				MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos ) = 0;

protected:
	CLevelRegion*				m_selectedRegion;
	IVector2D					m_mouseOverTile;
	float						m_mouseOverTileHeight;

	IVector2D					m_globalTile_lineStart;
	IVector2D					m_globalTile_lineEnd;
	bool						m_globalTile_pointSet;

	int							m_selectedHField;
};

#endif // BASETILEBASEDEDITOR_H