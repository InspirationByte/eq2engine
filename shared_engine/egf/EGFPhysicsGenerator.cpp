//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics object generator
//
//				TODO: refactoring of code here
//////////////////////////////////////////////////////////////////////////////////

#include "EGFPhysicsGenerator.h"

#include "core/DebugInterface.h"
#include "core/IFileSystem.h"
#include "utils/mtriangle_framework.h"
#include "utils/SmartPtr.h"
#include "utils/VirtualStream.h"
#include "utils/strtools.h"

#include "math/coord.h"

using namespace MTriangle;

// physics
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <bullet/BulletCollision/CollisionShapes/btShapeHull.h>

#include "egf/physmodel.h"

struct ragdolljoint_t
{
	Matrix4x4 localTrans;
	Matrix4x4 absTrans;
};

bool triangle_compare(itriangle &tri1, itriangle &tri2)
{
	return (tri1.idxs[0] == tri2.idxs[0]) && (tri1.idxs[1] == tri2.idxs[1]) && (tri1.idxs[2] == tri2.idxs[2]);
}

int find_triangle(DkList<itriangle> *tris, itriangle &tofind)
{
	for(int i = 0; i < tris->numElem(); i++)
	{
		if(triangle_compare(tris->ptr()[i], tofind))
			return i;
	}

	return -1;
}

bool vertex_compare(dsmvertex_t &v1, dsmvertex_t &v2)
{
	return v1.position == v2.position;
}

int find_vertex(DkList<dsmvertex_t> &verts, dsmvertex_t &tofind)
{
	for(int i = 0; i < verts.numElem(); i++)
	{
		if(vertex_compare(verts[i], tofind))
			return i;
	}

	return -1;
}

void AddTriangleWithAllNeighbours(indxgroup_t *group, mtriangle_t* triangle)
{
	itriangle first = {triangle->indices[0], triangle->indices[1], triangle->indices[2]};

	bool discard = false;

	for(int i = 0; i < group->numElem(); i++)
	{
		if( first.idxs[0] == group->ptr()[i].idxs[0] &&
			first.idxs[1] == group->ptr()[i].idxs[1] &&
			first.idxs[2] == group->ptr()[i].idxs[2])
		{
			discard = true;
			break;
		}
	}

	if(!discard)
	{
		group->append(first);

		for(int i = 0; i < triangle->index_connections.numElem(); i++)
		{
			AddTriangleWithAllNeighbours(group, triangle->index_connections[i]); 
		}
	}
}


//---------------------------------------------------------------------------------------------------

CEGFPhysicsGenerator::CEGFPhysicsGenerator() : m_srcModel(nullptr), m_physicsParams(nullptr), m_forceGroupSubdivision(false)
{
	m_props.model_usage = PHYSMODEL_USAGE_RIGID_COMP;
}

CEGFPhysicsGenerator::~CEGFPhysicsGenerator()
{
	Cleanup();
}

void CEGFPhysicsGenerator::Cleanup()
{
	m_srcModel = nullptr;
	m_physicsParams = nullptr;

	m_vertices.clear();
	m_indices.clear();
	m_shapes.clear();
	m_objects.clear();
	m_joints.clear();

	m_bbox.Reset();
}

void CEGFPhysicsGenerator::SetupRagdollJoints(ragdolljoint_t* boneArray)
{
	// setup each bone's transformation
	for(int i = 0; i < m_srcModel->bones.numElem(); i++)
	{
		ragdolljoint_t* joint = &boneArray[i];
		dsmskelbone_t* bone = m_srcModel->bones[i];

		// setup transformation
		joint->localTrans = identity4();

		joint->localTrans.setRotation(bone->angles);
		joint->localTrans.setTranslation(bone->position);

		if(bone->parent_id != -1)
			joint->absTrans = joint->localTrans * boneArray[bone->parent_id].absTrans;
		else
			joint->absTrans = joint->localTrans;
	}
}

// adds shape to datas
int CEGFPhysicsGenerator::AddShape(DkList<dsmvertex_t> &vertices, DkList<int> &indices, int shapeType, bool assumedAsConvex)
{
	physgeominfo_t geom_info;
	geom_info.type = shapeType;

	geom_info.startIndices = m_indices.numElem();

	// make hull
	if( geom_info.type == PHYSSHAPE_TYPE_CONVEX)
	{
		if(assumedAsConvex)
		{
			int startIndex = m_vertices.numElem();

			for(int i = 0; i < indices.numElem(); i+=3)
			{
				Vector3D pos1 = vertices[indices[i]].position;
				Vector3D pos2 = vertices[indices[i+1]].position;
				Vector3D pos3 = vertices[indices[i+2]].position;

				m_vertices.append(pos1);
				m_vertices.append(pos2);
				m_vertices.append(pos3);

				m_indices.append(i+startIndex);
				m_indices.append(i+1+startIndex);
				m_indices.append(i+2+startIndex);
			}

			geom_info.numIndices = indices.numElem();

			Msg("Adding convex shape (if you sure that it is), %d verts %d indices\n", vertices.numElem(), geom_info.numIndices);
		}
		else
		{
			// first generate triangle mesh
			btTriangleMesh trimesh(true, false);

			for(int i = 0; i < indices.numElem(); i+=3)
			{
				Vector3D pos1 = vertices[indices[i]].position;
				Vector3D pos2 = vertices[indices[i+1]].position;
				Vector3D pos3 = vertices[indices[i+2]].position;

				trimesh.addTriangle(	btVector3(pos1.x,pos1.y,pos1.z),
										btVector3(pos2.x,pos2.y,pos2.z),
										btVector3(pos3.x,pos3.y,pos3.z),
										true);
			}

			// second is to generate convex mesh
			btConvexTriangleMeshShape tmpConvexShape(&trimesh);

			// we have to generate shape without margin
			tmpConvexShape.setMargin(0.055f);

			// make shape hull
			btShapeHull shape_hull(&tmpConvexShape);

			// cook hull
			shape_hull.buildHull( 0.0 /*this even not work*/ );

			// finally, add indices and vertices:

			int startIndex = m_vertices.numElem();

			for(int i = 0; i < shape_hull.numVertices(); i++)
			{
				btVector3 vertex = shape_hull.getVertexPointer()[i];
				m_vertices.append(Vector3D(vertex.x(),vertex.y(),vertex.z()));
			}

			for(int i = 0; i < shape_hull.numIndices(); i++)
			{
				m_indices.append(shape_hull.getIndexPointer()[i] + startIndex);
			}

			geom_info.numIndices = shape_hull.numIndices();

			Msg("Adding convex shape, %d verts %d indices\n", shape_hull.numVertices(), shape_hull.numIndices());
		}
	}
	else
	{
		// just make trimesh

		DkList<Vector3D> shapeVerts;

		int start_vertex = m_vertices.numElem();

		for(int i = 0; i < indices.numElem(); i++)
		{
			Vector3D pos = vertices[indices[i]].position;

			int found_idx = shapeVerts.findIndex(pos);
			if(found_idx == -1 && indices[i] != found_idx)
			{
				int nVerts = shapeVerts.append(pos);
				m_indices.append(nVerts + start_vertex);
			}
			else
				m_indices.append(found_idx + start_vertex);
		}

		geom_info.numIndices = m_indices.numElem() - geom_info.startIndices;

		Msg("Adding shape, %d verts\n", indices.numElem());

		for(int i = 0; i < shapeVerts.numElem(); i++)
		{
			m_vertices.append(shapeVerts[i]);
		}
	}

	return m_shapes.append(geom_info);
}

//
// Joint valid parenting
//

int CEGFPhysicsGenerator::FindJointIdx(char* name)
{
	for(int i = 0; i < m_joints.numElem(); i++)
	{
		if(!stricmp(m_joints[i].name, name))
		{
			return i;
		}
	}

	return -1;
}

int CEGFPhysicsGenerator::MakeBoneValidParent(int boneId)
{
	int parent = m_srcModel->bones[boneId]->parent_id;

	int joint_index = -1;

	if(parent != -1)
	{
		joint_index = FindJointIdx(m_srcModel->bones[parent]->name);

		if(joint_index == -1)
			joint_index = MakeBoneValidParent(parent);
		else
			return joint_index;
	}

	return joint_index;
}

// this procedure useful for ragdolls
// it collects information about neighbour surfaces and joins triangles into subparts
void CEGFPhysicsGenerator::SubdivideModelParts( DkList<dsmvertex_t>& vertices, DkList<int>& indices, DkList<indxgroup_t*>& groups )
{
	for(int i = 0 ; i < m_srcModel->groups.numElem(); i++)
	{
		dsmgroup_t* group = m_srcModel->groups[i];
		for(int j = 0; j < group->verts.numElem(); j++)
		{
			indices.append(vertices.numElem());
			vertices.append(group->verts[j]);
			m_bbox.AddVertex(vertices[i].position);
		}
	}

	// connect same vertices
	for(int i = 0; i < indices.numElem(); i++)
	{
		int index = indices[i];
		dsmvertex_t& vertex = vertices[index];

		int found_index = find_vertex(vertices, vertex);

		if(found_index != i && found_index != -1)
			indices[i] = found_index;
	}

	Msg("Building neighbour triangle table... (%d indices)\n", indices.numElem());

	CAdjacentTriangleGraph triangleGraph;

	// build neighbours
	triangleGraph.Build(indices.ptr(),indices.numElem());

	Msg("Num. triangles parsed: %d\n", triangleGraph.GetTriangles()->numElem());

	indxgroup_t* main_group = new indxgroup_t;
	groups.append(main_group);

	Msg("Building groups...\n");

	// add tri with all of it's neighbour's herarchy
	AddTriangleWithAllNeighbours(main_group, &triangleGraph.GetTriangles()->ptr()[0]);

	Msg("Processed group 1, %d tris\n", main_group->numElem());

	for(int i = 1; i < triangleGraph.GetTriangles()->numElem(); i++)
	{
		mtriangle_t* mTriangle = &triangleGraph.GetTriangles()->ptr()[i];

		itriangle triangle = {mTriangle->indices[0],mTriangle->indices[1],mTriangle->indices[2]};

		bool found = false;

		// find this triangle in all previous groups
		for(int j = 0; j < groups.numElem(); j++)
		{
			if(find_triangle(groups[j], triangle) != -1)
			{
				found = true;
				break;
			}
		}

		// if not found, create new group and add triangle with all of it's neighbours
		if(!found)
		{
			indxgroup_t *new_group = new indxgroup_t;
			groups.append(new_group);

			// add tri with all of it's neighbour's herarchy
			AddTriangleWithAllNeighbours(new_group, mTriangle);

			Msg("Processed group %d, %d tris\n", groups.numElem(), new_group->numElem());
		}
	}

	MsgInfo("Detected groups: %d\n", groups.numElem());
}

bool CEGFPhysicsGenerator::CreateRagdollObjects( DkList<dsmvertex_t>& vertices, DkList<int>& indices, DkList<indxgroup_t*>& indexGroups )
{
	// setup pose bones
	S_NEWA(ragJoints, ragdolljoint_t, m_srcModel->bones.numElem());
	SetupRagdollJoints(ragJoints);

	kvkeybase_t* bonesSect = m_physicsParams->FindKeyBase("Bones", KV_FLAG_SECTION);
	kvkeybase_t* isDynamicProp = m_physicsParams->FindKeyBase("IsDynamic");

	if(KV_GetValueBool(isDynamicProp))
	{
		m_props.model_usage = PHYSMODEL_USAGE_DYNAMIC;
		Msg("  Model is dynamic\n");
	}
	else
	{
		m_props.model_usage = PHYSMODEL_USAGE_RAGDOLL;
		Msg("  Model is ragdoll\n");
	}

	Msg("Assigning bones to groups...\n");

	DkList<int> bone_group_indices;

	for(int i = 0; i < indexGroups.numElem(); i++)
	{
		int firsttri_indx0 = indexGroups[i]->ptr()[0].idxs[0];

		int bone_index = -1;
		
		if(vertices[firsttri_indx0].weights.numElem() > 0)
			vertices[firsttri_indx0].weights[0].bone;

		if(bone_index != -1)
			Msg("Group %d uses bone %s\n", i+1, m_srcModel->bones[bone_index]->name);
		else
			Msg("Group %d doesn't use bones, it will be static\n", i+1);

		bone_group_indices.append(bone_index);

		for(int j = 1; j < indexGroups[i]->numElem(); j++)
		{
			int idx0 = indexGroups[i]->ptr()[j].idxs[0];
			int idx1 = indexGroups[i]->ptr()[j].idxs[1];
			int idx2 = indexGroups[i]->ptr()[j].idxs[2];

			if( vertices[idx0].weights[0].bone != bone_index ||
				vertices[idx1].weights[0].bone != bone_index ||
				vertices[idx2].weights[0].bone != bone_index)
			{
				MsgError("Invalid bone id. Mesh part must use single bone index.\n");
				MsgError("Please separate model parts for bones.\n");
				return false;
			}
						
		}
	}

	kvkeybase_t* pDefaultSurfaceProps = m_physicsParams->FindKeyBase("SurfaceProps");

	for(int i = 0; i < m_srcModel->bones.numElem(); i++)
	{
		DkList<int> bone_geom_indices;

		Vector3D bboxMins(MAX_COORD_UNITS);
		Vector3D bboxMaxs(-MAX_COORD_UNITS);

		// add indices of attached groups and also build bounding box
		for(int j = 0; j < indexGroups.numElem(); j++)
		{
			if(bone_group_indices[j] == i)
			{
				for(int k = 0; k < indexGroups[j]->numElem(); k++)
				{
					bone_geom_indices.append( indexGroups[j]->ptr()[k].idxs[0] );
					bone_geom_indices.append( indexGroups[j]->ptr()[k].idxs[1] );
					bone_geom_indices.append( indexGroups[j]->ptr()[k].idxs[2] );

					m_bbox.AddVertex(vertices[ indexGroups[j]->ptr()[k].idxs[0] ].position);
					m_bbox.AddVertex(vertices[ indexGroups[j]->ptr()[k].idxs[1] ].position);
					m_bbox.AddVertex(vertices[ indexGroups[j]->ptr()[k].idxs[2] ].position);
				}
			}
		}

		// we should have at least more than 3 triangles for convex shapes
		if( bone_geom_indices.numElem() <= 9 )
			continue;

		// compute object center
		Vector3D object_center = (bboxMins+bboxMaxs)*0.5f;
				
		// transform objects to origin
		DkList<int> processed_index;
		for(int j = 0; j < bone_geom_indices.numElem(); j++)
		{
			if( processed_index.findIndex(bone_geom_indices[j]) == -1 )
			{
				vertices[bone_geom_indices[j]].position -= object_center;
				processed_index.append(bone_geom_indices[j]);
			}
		}
				
		// generate physics shape
		int shapeID = AddShape(vertices, bone_geom_indices);

		// build object data
		physobject_t object;

		memset(object.shape_indexes, -1, sizeof(object.shape_indexes));

		kvkeybase_t* thisBoneSec = bonesSect->FindKeyBase(m_srcModel->bones[i]->name, KV_FLAG_SECTION);

		object.body_part = 0;
		object.numShapes = 1;
		object.shape_indexes[0] = shapeID;
		object.offset = object_center;
		object.mass_center = vec3_zero;
		object.mass = DEFAULT_MASS;

		strcpy(object.surfaceprops, KV_GetValueString( pDefaultSurfaceProps, 0, "default" ));

		if( thisBoneSec )
		{
			object.mass = KV_GetValueFloat( thisBoneSec->FindKeyBase("Mass"), 0, DEFAULT_MASS );
			object.body_part = KV_GetValueInt(thisBoneSec->FindKeyBase("bodypart"), 0, 0);

			kvkeybase_t* surfPropsPair = thisBoneSec->FindKeyBase("SurfaceProps");

			if(surfPropsPair)
				strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair));
		}

		// build joint information
		physjoint_t joint;

		memset(joint.name, 0, sizeof(joint.name));
		strcpy_s(joint.name, m_srcModel->bones[i]->name);

		physNamedObject_t obj;
		memset(obj.name, 0, sizeof(obj.name));
		strcpy_s(obj.name, m_srcModel->bones[i]->name);
		obj.object = object;

		// add object after building
		m_objects.append(obj);

		// setup default limits
		joint.minLimit = vec3_zero;
		joint.maxLimit = vec3_zero;

		// set bone position
		Vector3D bone_position = ragJoints[i].absTrans.rows[3].xyz();
		joint.position = bone_position;

		joint.object_indexA = m_objects.numElem() - 1;

		int parent = m_srcModel->bones[i]->parent_id;

		if(parent == -1)
		{
			// join to itself
			joint.object_indexB = joint.object_indexA;
		}
		else
		{
			int phys_parent = MakeBoneValidParent(i);

			if(phys_parent == -1)
				joint.object_indexB = joint.object_indexA;
			else
				joint.object_indexB = phys_parent;
		}

		if(thisBoneSec)
		{
			// get axis and check limits
			for(int i = 0; i < thisBoneSec->keys.numElem(); i++)
			{
				int axis_idx = -1;

				kvkeybase_t* pKey = thisBoneSec->keys[i];

				if( !stricmp(pKey->name, "x_axis") )
					axis_idx = 0;
				else if( !stricmp(pKey->name, "y_axis") )
					axis_idx = 1;
				else if( !stricmp(pKey->name, "z_axis") )
					axis_idx = 2;

				if( axis_idx != -1 )
				{
					// check the value for arguments
					for(int j = 0; j < pKey->values.numElem(); j++)
					{
						if( !stricmp(KV_GetValueString(pKey, j), "limit" ))
						{
							// read limits
							float lo_limit = KV_GetValueFloat(pKey, j+1);
							float hi_limit = KV_GetValueFloat(pKey, j+2);

							joint.minLimit[axis_idx] = DEG2RAD(lo_limit);
							joint.maxLimit[axis_idx] = DEG2RAD(hi_limit);
						}
						else if( !stricmp(KV_GetValueString(pKey, j), "limitoffset" ))
						{
							float offs = KV_GetValueFloat(pKey, j+1);

							joint.minLimit[axis_idx] += DEG2RAD(offs);
							joint.maxLimit[axis_idx] += DEG2RAD(offs);
						}
					}
				}
			}
		}

		// add new joint
		m_joints.append(joint);
	}

	return true;
}

bool CEGFPhysicsGenerator::CreateCompoundOrSeparateObjects( DkList<dsmvertex_t>& vertices, DkList<int>& indices, DkList<indxgroup_t*>& indexGroups, bool bCompound )
{
	m_props.model_usage = PHYSMODEL_USAGE_RIGID_COMP;

	if(indexGroups.numElem() == 1)
		Msg("  Model is single\n");
	else
		Msg("  Model is compound\n");

	// shape types ignored on compound
	bool isConcave = KV_GetValueBool( m_physicsParams->FindKeyBase("concave"), 0, false );
	bool isStatic = KV_GetValueBool( m_physicsParams->FindKeyBase("static"), 0, false );

	int nShapeType = PHYSSHAPE_TYPE_CONVEX;

	if( isConcave )
	{
		nShapeType = PHYSSHAPE_TYPE_CONCAVE;

		if(!isStatic)
			nShapeType = PHYSSHAPE_TYPE_MOVABLECONCAVE;
	}

	// do compoound
	if( bCompound )
	{
		DkList<int> shape_ids;

		for(int i = 0; i < indexGroups.numElem(); i++)
		{
			DkList<int> tmpIndices;

			for(int j = 0; j < indexGroups[i]->numElem(); j++)
			{
				tmpIndices.append( indexGroups[i]->ptr()[j].idxs[0] );
				tmpIndices.append( indexGroups[i]->ptr()[j].idxs[1] );
				tmpIndices.append( indexGroups[i]->ptr()[j].idxs[2] );
			}

			int shapeID = AddShape(vertices, tmpIndices);
			shape_ids.append(shapeID);
		}

		physobject_t object;
		object.body_part = 0;

		kvkeybase_t* surfPropsPair = m_physicsParams->FindKeyBase("SurfaceProps");
				
		memset(object.surfaceprops, 0, 0);
		strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair, 0, "default"));

		object.mass = KV_GetValueFloat(m_physicsParams->FindKeyBase("Mass"), 0, DEFAULT_MASS);

		object.numShapes = shape_ids.numElem();

		if(object.numShapes > MAX_GEOM_PER_OBJECT)
		{
			MsgWarning("Exceeded physics shape count (%d)\n", object.numShapes);
			object.numShapes = MAX_GEOM_PER_OBJECT;
		}

		memset(object.shape_indexes, -1, sizeof(object.shape_indexes));

		for(int i = 0; i < object.numShapes; i++)
			object.shape_indexes[i] = shape_ids[i];

		object.offset = vec3_zero;
		object.mass_center = KV_GetVector3D( m_physicsParams->FindKeyBase("MassCenter"), 0, m_bbox.GetCenter() );

		physNamedObject_t obj;
		memset(obj.name, 0, sizeof(obj.name));
		strcpy_s(obj.name, KV_GetValueString(m_physicsParams, 0, varargs("obj_%d", m_objects.numElem())));

		obj.object = object;

		m_objects.append(obj);
	}
	else
	{
		physobject_t object;
		object.body_part = 0;

		memset(object.surfaceprops, 0, 0);
		strcpy(object.surfaceprops, KV_GetValueString(m_physicsParams->FindKeyBase("SurfaceProps"), 0, "default"));

		object.mass = KV_GetValueFloat(m_physicsParams->FindKeyBase("Mass"), 0, DEFAULT_MASS);

		for(int i = 0; i < indexGroups.numElem(); i++)
		{
			DkList<int> tmpIndices;

			for(int j = 0; j < indexGroups[i]->numElem(); j++)
			{
				tmpIndices.append( indexGroups[i]->ptr()[j].idxs[0] );
				tmpIndices.append( indexGroups[i]->ptr()[j].idxs[1] );
				tmpIndices.append( indexGroups[i]->ptr()[j].idxs[2] );
			}

			int shapeID = AddShape( vertices, tmpIndices, nShapeType );

			object.numShapes = 1;
			memset(object.shape_indexes, -1, sizeof(object.shape_indexes));

			object.shape_indexes[0] = shapeID;
			object.offset = vec3_zero;
			object.mass_center = vec3_zero;

			physNamedObject_t obj;
			obj.object = object;

			memset(obj.name, 0, sizeof(obj.name));

			if(m_physicsParams->values.numElem() > 0)
				strcpy_s(obj.name, varargs("%s_part%d", KV_GetValueString(m_physicsParams), i));
			else
				strcpy_s(obj.name,varargs("obj_%d", m_objects.numElem()));

			m_objects.append(obj);
		}
	}

	return true;
}

bool CEGFPhysicsGenerator::CreateSingleObject( DkList<dsmvertex_t>& vertices, DkList<int>& indices )
{
	// shape types ignored on compound
	bool isConcave = KV_GetValueBool( m_physicsParams->FindKeyBase("concave"), 0, false );
	bool isStatic = KV_GetValueBool( m_physicsParams->FindKeyBase("static"), 0, false );
	bool isAssumedAsConvex = KV_GetValueBool( m_physicsParams->FindKeyBase("dont_simplify"), 0, false );

	int nShapeType = PHYSSHAPE_TYPE_CONVEX;

	if( isConcave )
	{
		nShapeType = PHYSSHAPE_TYPE_CONCAVE;

		if(!isStatic)
			nShapeType = PHYSSHAPE_TYPE_MOVABLECONCAVE;
	}

	// generate big shape in this case
	int shapeID = AddShape(vertices, indices, nShapeType, isAssumedAsConvex);

	physobject_t object;

	object.body_part = 0;

	memset(object.surfaceprops, 0, sizeof(object.surfaceprops));
	strcpy(object.surfaceprops, KV_GetValueString(m_physicsParams->FindKeyBase("SurfaceProps"), 0, "default"));

	object.mass = KV_GetValueFloat(m_physicsParams->FindKeyBase("Mass"), 0, DEFAULT_MASS);
	object.mass_center = KV_GetVector3D(m_physicsParams->FindKeyBase("MassCenter"), 0, m_bbox.GetCenter());

	object.numShapes = 1;
	memset(object.shape_indexes, -1, sizeof(object.shape_indexes));
	object.shape_indexes[0] = shapeID;
	object.offset = vec3_zero;

	physNamedObject_t obj;
	memset(obj.name, 0, sizeof(obj.name));
	strcpy_s(obj.name, KV_GetValueString(m_physicsParams, 0, varargs("obj_%d", m_objects.numElem())));

	obj.object = object;

	m_objects.append(obj);

	return true;
}

bool CEGFPhysicsGenerator::GenerateGeometry(dsmmodel_t* srcModel, kvkeybase_t* physInfo, bool forceGroupSubdivision)
{
	m_srcModel = srcModel;
	m_physicsParams = physInfo;
	m_forceGroupSubdivision = forceGroupSubdivision;

	if(!m_srcModel || !m_physicsParams)
		return false;

	MsgInfo("Generating physics geometry...\n");

	bool bCompound = false;

	if(m_srcModel->bones.numElem() > 0)
		m_forceGroupSubdivision = true;

	m_forceGroupSubdivision = KV_GetValueBool(m_physicsParams->FindKeyBase("groupdivision"), 0, m_forceGroupSubdivision);

	kvkeybase_t* compoundkey = m_physicsParams->FindKeyBase("compound");
	if(compoundkey)
	{
		bCompound = KV_GetValueBool(compoundkey);
		m_forceGroupSubdivision = bCompound;
	}

	memset(m_props.comment_string, 0, sizeof(m_props.comment_string));
	strcpy(m_props.comment_string, KV_GetValueString(m_physicsParams->FindKeyBase("comments"), 0, ""));

	DkList<dsmvertex_t>		vertices;
	DkList<int>				indices;

	// if we've got ragdoll
	if( m_forceGroupSubdivision || (m_srcModel->bones.numElem() > 1)  )
	{
		// generate index groups
		DkList<indxgroup_t*> indexGroups;
		SubdivideModelParts(vertices, indices, indexGroups);

		// generate ragdoll
		if( m_srcModel->bones.numElem() > 1 )
		{
			CreateRagdollObjects( vertices, indices, indexGroups );
		}
		else // make compound model
		{
			CreateCompoundOrSeparateObjects( vertices, indices, indexGroups, bCompound);
		}

		for(int i = 0; i < indexGroups.numElem(); i++)
			delete indexGroups[i];

		indexGroups.clear();
	}
	else
	{
		m_props.model_usage = PHYSMODEL_USAGE_RIGID_COMP;

		// move all vertices and indices from groups to shared buffer (no multiple shapes)
		for(int i = 0 ; i < m_srcModel->groups.numElem(); i++)
		{
			dsmgroup_t* group = m_srcModel->groups[i];

			for(int j = 0; j < group->verts.numElem(); j++)
			{
				indices.append(vertices.numElem());
				vertices.append(group->verts[j]);
				m_bbox.AddVertex(vertices[i].position);
			}
		}

		Msg("Processed %d verts and %d indices\n", vertices.numElem(), indices.numElem());

		CreateSingleObject( vertices, indices );
	}

	return true;
}

//
// file writeage
//

#define MAX_PHYSICSFILE_SIZE 16*1024*1024

ubyte* pData = NULL;
ubyte* pStart = NULL;

ubyte* CopyLumpToFile(ubyte* data, int lump_type, ubyte* toCopy, int toCopySize)
{
	ubyte* ldata = data;

	physmodellump_t *lumpdata = (physmodellump_t*)ldata;
	lumpdata->size = toCopySize;
	lumpdata->type = lump_type;

	ldata = ldata + sizeof(physmodellump_t);

	memcpy(ldata, toCopy, toCopySize);

	ldata = ldata + toCopySize;

	return ldata;
}

void WriteLumpToStream(IVirtualStream* stream, int lump_type, ubyte* data, uint dataSize)
{
	physmodellump_t lumpHdr;
	lumpHdr.type = lump_type;
	lumpHdr.size = dataSize;

	stream->Write(&lumpHdr, 1, sizeof(lumpHdr));
	stream->Write(data, 1, dataSize);
}

void CEGFPhysicsGenerator::SaveToFile(const char* filename)
{
	CMemoryStream lumpsStream;
	if(!lumpsStream.Open(NULL, VS_OPEN_WRITE, MAX_PHYSICSFILE_SIZE))
	{
		MsgError("Failed to allocate memory stream!\n");
		return;
	}

	WriteLumpToStream(&lumpsStream, PHYSLUMP_PROPERTIES, (ubyte*)&m_props, sizeof(physmodelprops_t));
	WriteLumpToStream(&lumpsStream, PHYSLUMP_GEOMETRYINFO, (ubyte*)m_shapes.ptr(), sizeof(physgeominfo_t) * m_shapes.numElem());

	// write names lump before objects lump
	// PHYSLUMP_OBJECTNAMES
	{
		CMemoryStream objNamesLump;
		objNamesLump.Open(NULL, VS_OPEN_WRITE, 2048);

		for(int i = 0; i < m_objects.numElem(); i++)
			objNamesLump.Write(m_objects[i].name, 1, strlen(m_objects[i].name)+1);

		char nullChar = '\0';
		objNamesLump.Write(&nullChar, 1, 1);

		WriteLumpToStream(&lumpsStream, PHYSLUMP_OBJECTNAMES,		(ubyte*)objNamesLump.GetBasePointer(), objNamesLump.Tell());

		objNamesLump.Close();
	}


	// PHYSLUMP_OBJECTS
	{
		CMemoryStream objDataLump;
		objDataLump.Open(NULL, VS_OPEN_WRITE, 2048);

		for(int i = 0; i < m_objects.numElem(); i++)
			objDataLump.Write(&m_objects[i].object, 1, sizeof(m_objects[i].object));

		WriteLumpToStream(&lumpsStream, PHYSLUMP_OBJECTS,		(ubyte*)objDataLump.GetBasePointer(), objDataLump.Tell());

		objDataLump.Close();
	}

	WriteLumpToStream(&lumpsStream, PHYSLUMP_INDEXDATA,	(ubyte*)m_indices.ptr(), sizeof(int) * m_indices.numElem());
	WriteLumpToStream(&lumpsStream, PHYSLUMP_VERTEXDATA,	(ubyte*)m_vertices.ptr(), sizeof(Vector3D) * m_vertices.numElem());
	WriteLumpToStream(&lumpsStream, PHYSLUMP_JOINTDATA,	(ubyte*)m_joints.ptr(), sizeof(physjoint_t) * m_joints.numElem());

	Msg("Total lumps size: %d\n", lumpsStream.GetSize());

	Msg("Total:\n");
	Msg("  Vertex count: %d\n", m_vertices.numElem());
	Msg("  Index count: %d\n", m_indices.numElem());
	Msg("  Shape count: %d\n", m_shapes.numElem());
	Msg("  Object count: %d\n", m_objects.numElem());
	Msg("  Joints count: %d\n", m_joints.numElem());
	
	IFile* outputFile = g_fileSystem->Open(filename, "wb");

	if(!outputFile)
	{
		MsgError("Failed to create file '%s' for writing!\n", filename);
		return;
	}

	physmodelhdr_t header;
	header.ident = PHYSMODEL_ID;
	header.version = PHYSMODEL_VERSION;
	header.num_lumps = PHYSLUMP_LUMPS;

	outputFile->Write(&header, 1, sizeof(header));
	lumpsStream.WriteToFileStream(outputFile);

	g_fileSystem->Close(outputFile);
}