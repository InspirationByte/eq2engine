//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Basic editor objects
//
// TODO: Undo tables, undoable objects//////////////////////////////////////////////////////////////////////////////////

#include "BaseEditableObject.h"
#include "EditorLevel.h"
#include "grid.h"

CBaseEditableObject::CBaseEditableObject()
{
	SetPosition(vec3_zero);
	SetAngles(vec3_zero);
	SetScale(Vector3D(1));

	// by default, object not in group
	m_groupid = -1;
	m_groupColor = ColorRGB(0.5f+RandomFloat(0.0f,0.5f),0.5f+RandomFloat(0.0f,0.5f),0.5f+RandomFloat(0.0f,0.5f));

	m_nFlags = 0;

	m_layer_id = 0; // main layer

	m_id = s_objectid_increment;
	s_objectid_increment++;

	m_szName = "";
}

CBaseEditableObject::CBaseEditableObject(Vector3D &position, Vector3D &angles, Vector3D &scale)
{
	SetPosition(position);
	SetAngles(angles);
	SetScale(scale);
}

// called when object is removes from level
void CBaseEditableObject::OnRemove(bool bOnLevelCleanup)
{

}

void CBaseEditableObject::AddFlags(int flags)
{
	m_nFlags |= flags;

	if(flags & EDFL_SELECTED)
	{
		// select all surfaces
		for(int i = 0; i < GetSurfaceTextureCount(); i++)
			GetSurfaceTexture(i)->nFlags |= STFL_SELECTED;
	}
}

void CBaseEditableObject::RemoveFlags(int flags)
{
	m_nFlags &= ~flags;

	if(flags & EDFL_SELECTED)
	{
		// select all surfaces
		for(int i = 0; i < GetSurfaceTextureCount(); i++)
			GetSurfaceTexture(i)->nFlags &= ~STFL_SELECTED;
	}
}

int	CBaseEditableObject::GetFlags()
{
	return m_nFlags;
}

void CBaseEditableObject::SetGroupColor(ColorRGB &col)
{
	m_groupColor = col;
}

ColorRGB CBaseEditableObject::GetGroupColor()
{
	return m_groupColor;
}

Vector3D CBaseEditableObject::GetPosition()
{
	return m_position;
}

Vector3D CBaseEditableObject::GetAngles()
{
	return m_angles;
}

Vector3D CBaseEditableObject::GetScale()
{
	return m_scale;
}

void CBaseEditableObject::SetPosition(Vector3D &position)
{
	m_position = position;
}

void CBaseEditableObject::SetAngles(Vector3D &angles)
{
	m_angles = angles;
}

void CBaseEditableObject::SetScale(Vector3D &scale)
{
	m_scale = scale;
}

void CBaseEditableObject::SnapObjectByGrid(int grid_size)
{
	m_position = SnapVector(grid_size, m_position);
}

void CBaseEditableObject::SetGroupID(int id)
{
	m_groupid = id;
}

int	CBaseEditableObject::GetGroupID()
{
	return m_groupid;
}

void CBaseEditableObject::SetName(const char* pszName)
{
	m_szName = pszName;
}

const char* CBaseEditableObject::GetName() const
{
	return m_szName.GetData();
}

void CBaseEditableObject::SetLayer(int id)
{
	g_pLevel->GetLayer(m_layer_id)->object_ids.remove(m_id);

	m_layer_id = id;

	g_pLevel->GetLayer(m_layer_id)->object_ids.addUnique(m_id);
}