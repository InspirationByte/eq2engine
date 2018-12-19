//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable group class
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITABLEGROUP_H
#define EDITABLEGROUP_H

#include "BaseEditableObject.h"

//struct level_build_data_t;

// The editable entity
class CEditableGroup: public CBaseEditableObject
{
public:
									CEditableGroup();
									~CEditableGroup();

	EditableType_e					GetType() {return EDITABLE_GROUP;}

	void							Render(int nViewRenderFlags);
	void							RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center = false, Vector3D &center = vec3_zero);

	Matrix4x4						GetRenderWorldTransform() {return identity4();}

	// rendering bbox
	Vector3D						GetBBoxMins();
	Vector3D						GetBBoxMaxs();

	Vector3D						GetBoundingBoxMins();
	Vector3D						GetBoundingBoxMaxs();

	void							Translate(Vector3D &position) {}
	void							Scale(Vector3D &scale, bool use_center = false, Vector3D &scale_center = vec3_zero) {}
	void							Rotate(Vector3D &rotation_angles, bool use_center = false, Vector3D &rotation_center = vec3_zero) {}

	void							OnRemove(bool bOnLevelCleanup = false);

	// saves this object
	bool							WriteObject(IVirtualStream*	pStream);

	// read this object
	bool							ReadObject(IVirtualStream*	pStream);

	// stores object in keyvalues
	void							SaveToKeyValues(kvkeybase_t* pSection);

	// stores object in keyvalues
	bool							LoadFromKeyValues(kvkeybase_t* pSection);

	// called when whole level builds
	//void							BuildObject(level_build_data_t* pLevelBuildData) {}

	// copies this object
	CBaseEditableObject*			CloneObject();

	// selection ray
	float							CheckLineIntersection(const Vector3D &start, const Vector3D &end,Vector3D &outPos);

	DkList<CBaseEditableObject*>*	GetGroupList();

	void							InitGroup(DkList<CBaseEditableObject*> &pObjects);
	void							RemoveObject(CBaseEditableObject* pObject);

	//void							SetName(const char* pszName);
	//const char*						GetName();

protected:

	DkList<CBaseEditableObject*>	m_groupped;
	//DkStr							m_groupname;
};

#endif // EDITABLEGROUP_H