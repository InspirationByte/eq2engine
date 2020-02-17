//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable entity class
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITABLEENTITY_H
#define EDITABLEENTITY_H

#include "EntityDef.h"
#include "BaseEditableObject.h"
#include "bonesetup.h"

class IEntityVariable;

struct edkeyvalue_t
{
	char name[128];
	char value[512];
};

// The editable entity
class CEditableEntity : public CBaseEditableObject
{
public:
	CEditableEntity();
	~CEditableEntity();

	EditableType_e				GetType() {return EDITABLE_ENTITY;}

	void						Render(int nViewRenderFlags);
	void						RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center = false, Vector3D &center = vec3_zero);

	Matrix4x4					GetRenderWorldTransform();

	// rendering bbox
	Vector3D					GetBBoxMins();					// min bbox dimensions
	Vector3D					GetBBoxMaxs();					// max bbox dimensions

	Vector3D					GetBoundingBoxMins();	// returns bbox
	Vector3D					GetBoundingBoxMaxs();

	void						OnRemove(bool bOnLevelCleanup = false);

	// saves this object
	bool						WriteObject(IVirtualStream*	pStream);

	// read this object
	bool						ReadObject(IVirtualStream*	pStream);

	// stores object in keyvalues
	void						SaveToKeyValues(kvkeybase_t* pSection);

	// stores object in keyvalues
	bool						LoadFromKeyValues(kvkeybase_t* pSection);

	// called when whole level builds
	//void						BuildObject(level_build_data_t* pLevelBuildData);

	// copies this object
	CBaseEditableObject*		CloneObject();

	// selection ray
	float						CheckLineIntersection(const Vector3D &start, const Vector3D &end,Vector3D &outPos);

	void						Translate(Vector3D &position);
	void						Scale(Vector3D &scale, bool use_center = false, Vector3D &scale_center = vec3_zero);
	void						Rotate(Vector3D &rotation_angles, bool use_center = false, Vector3D &rotation_center = vec3_zero);


	// returns object origin, entity-only
	void						SetPosition(Vector3D &position);

	// returns object angles, entity-only
	void						SetAngles(Vector3D &angles);

	// returns object scale, entity-only
	void						SetScale(Vector3D &scale);

	void						SetClassName(const char* pszClassname);
	const char*					GetClassname();

	void						SetName(const char* pszName);

	edef_entity_t*				GetDefinition();
	DkList<edkeyvalue_t>*		GetPairs();
	DkList<OutputData_t>*		GetOutputs();

	void						SetKey(const char* pszKey, const char* pszValue, bool updateParam = true);
	void						RemoveKey(const char* pszKey);
	char*						GetKeyValue(const char* pszKey);

	IEntityVariable*			GetTypedVariableByKeyName(char* pszKey);
	IEntityVariable*			GetTypedVariable(int index)		{ return m_EntTypedVars[index]; }
	int							GetTypedVariableCount()			{ return m_EntTypedVars.numElem(); }

	void						UpdateParameters();

	void						AddOutput(char* pszString);


protected:

	edef_entity_t*					m_pDefinition;

	DkList<edkeyvalue_t>			m_kvPairs;
	DkList<IEntityVariable*>		m_EntTypedVars;

	EqString						m_szClassName;

	DkList<OutputData_t>			m_Outputs;

	IEqModel*						m_pModel;
	IMaterial*						m_pSprite;
};

#endif // EDITABLEENTITY_H