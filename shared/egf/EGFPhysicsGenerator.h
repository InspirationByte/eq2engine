//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics model processor
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

struct KVSection;

struct PhyNamedObject;
struct RagdollJoint;

// same as AdjacentTriangles::ITriangle & AdjacentTriangles::Island
using IdxTriangle = IVector3D;
using IdxIsland = Array<IdxTriangle>;

namespace SharedModel
{
	struct DSModel;
	struct DSVertex;
}

class CEGFPhysicsGenerator
{
public:
	CEGFPhysicsGenerator();
	~CEGFPhysicsGenerator();

	void		Cleanup();

	bool		GenerateGeometry(SharedModel::DSModel* srcModel, const KVSection* physInfo, bool forceGroupSubdivision);
	void		SaveToFile(const char* filename);
	bool		HasObjects() const {return m_objects.numElem() > 0;}

protected:
	void		SetupRagdollJoints(Array<RagdollJoint>& boneArray);

	int			FindJointIdx(const char* name);
	int			MakeBoneValidParent(int boneId);

	int			AddShape(ArrayCRef<SharedModel::DSVertex> vertices, ArrayCRef<int> indices, EPhysShapeType shapeType = PHYSSHAPE_TYPE_CONVEX, bool assumedAsConvex = false);

	void		SubdivideModelParts( Array<SharedModel::DSVertex>& vertices, Array<int>& indices, Array<IdxIsland>& indexGroups);

	void		CreateRagdollObjects(const KVSection* bonesSect, ArrayRef<SharedModel::DSVertex> vertices, ArrayCRef<int> indices, ArrayCRef<IdxIsland> indexGroups );
	void		CreateCompoundObject(ArrayCRef<SharedModel::DSVertex> vertices, ArrayCRef<int> indices, ArrayCRef<IdxIsland> indexGroups);
	void		CreateMultipleObjects(ArrayCRef<SharedModel::DSVertex> vertices, ArrayCRef<int> indices, ArrayCRef<IdxIsland> indexGroups);
	void		CreateSingleObject(ArrayCRef<SharedModel::DSVertex> vertices, ArrayCRef<int> indices );

	// data
	SharedModel::DSModel*	m_srcModel;
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
