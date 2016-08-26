//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Tile-based editor for Drivers
//////////////////////////////////////////////////////////////////////////////////

#include "BaseTilebasedEditor.h"
#include "EditorMain.h"

#include "world.h"

CBaseTilebasedEditor::CBaseTilebasedEditor()
{
	m_selectedRegion = NULL;
	m_globalTile_pointSet = false;
	m_selectedHField = 0;
	m_mouseOverTileHeight = 0.0f;
}

void CBaseTilebasedEditor::ProcessMouseEvents( wxMouseEvent& event )
{
	if(g_pGameWorld->m_level.m_cellsSize == 0)
		return;

	Vector3D ray_start, ray_dir;

	g_pMainFrame->GetMouseScreenVectors(event.GetX(),event.GetY(), ray_start, ray_dir);

	Vector3D point_pos;
	m_selectedRegion = g_pMainFrame->GetRegionAtScreenPos(event.GetX(),event.GetY(), m_mouseOverTileHeight, point_pos);

	int x, y;
	if( m_selectedRegion && m_selectedRegion->GetHField(m_selectedHField)->PointAtPos(point_pos, x, y) )
	{
		if( !m_globalTile_pointSet )
		{
			if( event.ButtonDown() )
			{
				g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(x,y), m_selectedRegion, m_globalTile_lineStart);
				m_globalTile_lineEnd = m_globalTile_lineStart;

				m_globalTile_pointSet = true;
			}
		}
		else
		{
			g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(x,y), m_selectedRegion, m_globalTile_lineEnd);
		}

		if( event.ButtonUp() )
		{
			m_globalTile_pointSet = false;
		}
			

		hfieldtile_t* tile = m_selectedRegion->GetHField(m_selectedHField)->GetTile(x, y);

		if(tile)
		{
			m_mouseOverTile = IVector2D(x, y);

			if(m_selectedRegion)
				tile = m_selectedRegion->GetHField(m_selectedHField)->GetTile(m_mouseOverTile.x, m_mouseOverTile.y);

			if(tile)
				m_mouseOverTileHeight = tile->height*HFIELD_HEIGHT_STEP;
			else
				m_mouseOverTileHeight = 0.0f;
			
			// process custom event
			MouseEventOnTile(event, tile, x, y, point_pos);
		}
	}
}

void CBaseTilebasedEditor::OnRender()
{
	if( m_globalTile_pointSet )
	{
		Vector3D startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_globalTile_lineStart);
		Vector3D endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(m_globalTile_lineEnd);

		Vector3D box_size(HFIELD_POINT_SIZE*0.5f, 0.25, HFIELD_POINT_SIZE*0.5f);
		debugoverlay->Box3D(startPos-box_size, startPos+box_size, ColorRGBA(0.25f,1,0,0.5f));

		debugoverlay->Line3D(startPos, endPos, ColorRGBA(1,1,0.25f,1), ColorRGBA(1,1,0.25f,1));
	}

	if(m_selectedRegion)
	{
		hfieldtile_t* tile = m_selectedRegion->GetHField(m_selectedHField)->GetTile(m_mouseOverTile.x, m_mouseOverTile.y);

		if(tile)
		{
			Vector3D box_pos(m_mouseOverTile.x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, m_mouseOverTile.y*HFIELD_POINT_SIZE);
			Vector3D box_size(HFIELD_POINT_SIZE, 0.25, HFIELD_POINT_SIZE);

			box_pos += m_selectedRegion->GetHField(m_selectedHField)->m_position - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

			debugoverlay->Box3D(box_pos, box_pos+box_size, ColorRGBA(1,1,0,0.5f));
		}
	}
}

void CBaseTilebasedEditor::OnLevelUnload()
{
	m_selectedRegion = NULL;
	m_mouseOverTileHeight = 0.0f;
}