//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable group class
//////////////////////////////////////////////////////////////////////////////////

#include "GroupEditable.h"
#include "math/BoundingBox.h"
#include "EditorLevel.h"
#include "DebugInterface.h"
#include "utils/strtools.h"

CEditableGroup::CEditableGroup()
{
	//m_groupname = "";
}

CEditableGroup::~CEditableGroup()
{

}

void CEditableGroup::Render(int nViewRenderFlags)
{/*
	for(int i = 0; i < m_groupped.numElem(); i++)
	{
		m_groupped[i]->Render(i);
	}
	*/
}

void CEditableGroup::RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center, Vector3D &center)
{
	/*
	for(int i = 0; i < m_groupped.numElem(); i++)
	{
		m_groupped[i]->RenderGhost(addpos,addscale,addrot,use_center,center);
	}
	*/
}

void CEditableGroup::OnRemove(bool bOnLevelCleanup)
{
	if(bOnLevelCleanup)
		return;

	for(int i = 0; i < m_groupped.numElem(); i++)
		m_groupped[i]->RemoveFlags(EDFL_GROUPPED);

	m_groupped.clear();
}

// saves this object
bool CEditableGroup::WriteObject(IVirtualStream* pStream)
{
	return true;
}

// read this object
bool CEditableGroup::ReadObject(IVirtualStream*	pStream)
{
	return true;
}

// stores object in keyvalues
void CEditableGroup::SaveToKeyValues(kvkeybase_t* pSection)
{
	//pSection->SetKey("name", m_groupname.getData());

	kvkeybase_t* objIds = pSection->AddKeyBase("objects");

	for(int i = 0; i < m_groupped.numElem(); i++)
		objIds->AddKeyBase("id", varargs("%d", m_groupped[i]->GetObjectID()));
}

// stores object in keyvalues
bool CEditableGroup::LoadFromKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* objIds = pSection->FindKeyBase("objects", KV_FLAG_SECTION);

	//kvkeyvaluepair_t* pPair = pSection->FindKey("name");

	//if(pPair)
	//	m_groupname = pPair->value;

	for(int i = 0; i < objIds->keys.numElem(); i++)
	{
		int obj_id = KV_GetValueInt(objIds->keys[i]);

		CBaseEditableObject* pObj = g_pLevel->GetEditableByUniqueID(obj_id);
		if(pObj)
		{
			m_groupped.append(pObj);
			pObj->AddFlags(EDFL_GROUPPED);
		}
	}

	return true;
}

// copies this object
CBaseEditableObject* CEditableGroup::CloneObject()
{
	return NULL;
}

// selection ray
float CEditableGroup::CheckLineIntersection(const Vector3D &start, const Vector3D &end,Vector3D &outPos)
{
	float fNearest = 1.0f;
	Vector3D intersecPos = vec3_zero;

	for(int i = 0; i < m_groupped.numElem(); i++)
	{
		float fNear = m_groupped[i]->CheckLineIntersection(start,end,intersecPos);

		if(fNear < fNearest)
		{
			fNearest = fNear;
			outPos = intersecPos;
		}
	}

	return fNearest;
}

DkList<CBaseEditableObject*>* CEditableGroup::GetGroupList()
{
	return &m_groupped;
}

void CEditableGroup::InitGroup(DkList<CBaseEditableObject*> &pObjects)
{
	m_groupped.clear();
	m_groupped.append(pObjects);

	for(int i = 0; i < m_groupped.numElem(); i++)
	{
		m_groupped[i]->AddFlags(EDFL_GROUPPED);
	}
}

void CEditableGroup::RemoveObject(CBaseEditableObject* pObject)
{
	m_groupped.remove(pObject);
	pObject->RemoveFlags(EDFL_GROUPPED);
}

// rendering bbox
Vector3D CEditableGroup::GetBBoxMins()
{
	BoundingBox box;
	box.Reset();

	for(int i = 0; i < m_groupped.numElem(); i++)
	{
		box.AddVertex(m_groupped[i]->GetBBoxMins());
		box.AddVertex(m_groupped[i]->GetBBoxMaxs());
	}

	return box.minPoint;
}

Vector3D CEditableGroup::GetBBoxMaxs()
{
	BoundingBox box;
	box.Reset();

	for(int i = 0; i < m_groupped.numElem(); i++)
	{
		box.AddVertex(m_groupped[i]->GetBBoxMins());
		box.AddVertex(m_groupped[i]->GetBBoxMaxs());
	}

	return box.maxPoint;
}

Vector3D CEditableGroup::GetBoundingBoxMins()
{
	return GetBBoxMins();
}

Vector3D CEditableGroup::GetBoundingBoxMaxs()
{
	return GetBBoxMaxs();
}

/*
void CEditableGroup::SetName(const char* pszName)
{
	m_groupname = pszName;
}

char* CEditableGroup::GetName()
{
	return m_groupname.getData();
}
*/