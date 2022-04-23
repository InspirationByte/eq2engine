//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics model processor
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGFPHYSICSGENERATOR_H
#define EGFPHYSICSGENERATOR_H

#include "utils/KeyValues.h"
#include "dsm_loader.h"
#include "egf/model.h"

struct itriangle
{
	int idxs[3];
};

typedef Array<itriangle> indxgroup_t;

struct ragdolljoint_t;

struct physNamedObject_t
{
	char name[32];
	physobject_t object;
};

class CEGFPhysicsGenerator
{
public:
	CEGFPhysicsGenerator();
	virtual ~CEGFPhysicsGenerator();

	void		Cleanup();

	bool		GenerateGeometry(SharedModel::dsmmodel_t* srcModel, kvkeybase_t* physInfo, bool forceGroupSubdivision);

	void		SaveToFile(const char* filename);

	bool		HasObjects() const {return m_objects.numElem() > 0;}

protected:
	void		SetupRagdollJoints(ragdolljoint_t* boneArray);

	int			FindJointIdx(char* name);
	int			MakeBoneValidParent(int boneId);

	int			AddShape(Array<SharedModel::dsmvertex_t> &vertices, Array<int> &indices, int shapeType = PHYSSHAPE_TYPE_CONVEX, bool assumedAsConvex = false);

	void		SubdivideModelParts( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices, Array<indxgroup_t*>& groups );

	bool		CreateRagdollObjects( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices, Array<indxgroup_t*>& indexGroups );
	bool		CreateCompoundOrSeparateObjects( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices, Array<indxgroup_t*>& indexGroups, bool bCompound );
	bool		CreateSingleObject( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices );

	// data
	SharedModel::dsmmodel_t*	m_srcModel;
	kvkeybase_t*				m_physicsParams;

	Array<Vector3D>			m_vertices;			// generated verts
	Array<int>					m_indices;			// generated indices
	Array<physgeominfo_t>		m_shapes;			// shapes
	Array<physNamedObject_t>	m_objects;			// objects that use shapes
	Array<physjoint_t>			m_joints;			// joints which uses objects

	BoundingBox					m_bbox;

	physmodelprops_t			m_props;					// model usage
	bool						m_forceGroupSubdivision;
};

#endif // EGFPHYSICSGENERATOR_H