//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics model processor
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

struct KVSection;

struct PhyNamedObject;
struct RagdollJoint;
using IdxTriangle = IVector3D;
using IdxTriList = Array<IdxTriangle>;

namespace SharedModel
{
	struct dsmmodel_t;
	struct dsmvertex_t;
}

class CEGFPhysicsGenerator
{
public:
	CEGFPhysicsGenerator();
	~CEGFPhysicsGenerator();

	void		Cleanup();

	bool		GenerateGeometry(SharedModel::dsmmodel_t* srcModel, const KVSection* physInfo, bool forceGroupSubdivision);
	void		SaveToFile(const char* filename);
	bool		HasObjects() const {return m_objects.numElem() > 0;}

protected:
	void		SetupRagdollJoints(Array<RagdollJoint>& boneArray);

	int			FindJointIdx(const char* name);
	int			MakeBoneValidParent(int boneId);

	int			AddShape(Array<SharedModel::dsmvertex_t> &vertices, Array<int> &indices, int shapeType = PHYSSHAPE_TYPE_CONVEX, bool assumedAsConvex = false);

	void		SubdivideModelParts( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices, Array<IdxTriList*>& groups );

	bool		CreateRagdollObjects( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices, Array<IdxTriList*>& indexGroups );
	bool		CreateCompoundOrSeparateObjects( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices, Array<IdxTriList*>& indexGroups, bool bCompound );
	bool		CreateSingleObject( Array<SharedModel::dsmvertex_t>& vertices, Array<int>& indices );

	// data
	SharedModel::dsmmodel_t*	m_srcModel;
	const KVSection*			m_physicsParams;

	Array<Vector3D>				m_vertices{ PP_SL };		// generated verts
	Array<int>					m_indices{ PP_SL };			// generated indices
	Array<physgeominfo_t>		m_shapes{ PP_SL };			// shapes
	Array<PhyNamedObject>		m_objects{ PP_SL };			// objects that use shapes
	Array<physjoint_t>			m_joints{ PP_SL };			// joints which uses objects

	BoundingBox					m_bbox;

	physmodelprops_t			m_props;					// model usage
	bool						m_forceGroupSubdivision;
};
