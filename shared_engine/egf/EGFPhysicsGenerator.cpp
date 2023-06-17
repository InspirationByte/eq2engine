//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics object generator
//
//				TODO: refactoring of code here
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "utils/AdjacentTriangles.h"
#include "EGFPhysicsGenerator.h"
#include "dsm_loader.h"

using namespace AdjacentTriangles;
using namespace SharedModel;

// physics
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

struct RagdollJoint
{
	Matrix4x4 localTrans;
	Matrix4x4 absTrans;
};

struct PhyNamedObject
{
	char name[32]{ 0 };
	physobject_t object;
};

//---------------------------------------------------------------------------------------------------

CEGFPhysicsGenerator::CEGFPhysicsGenerator() : m_srcModel(nullptr), m_physicsParams(nullptr), m_forceGroupSubdivision(false)
{
	m_props.usageType = PHYSMODEL_USAGE_RIGID_COMP;
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

void CEGFPhysicsGenerator::SetupRagdollJoints(Array<RagdollJoint>& boneArray)
{
	// setup each bone's transformation
	for(int i = 0; i < m_srcModel->bones.numElem(); i++)
	{
		RagdollJoint& joint = boneArray[i];
		dsmskelbone_t* bone = m_srcModel->bones[i];

		// setup transformation
		joint.localTrans = identity4;

		joint.localTrans.setRotation(bone->angles);
		joint.localTrans.setTranslation(bone->position);

		if(bone->parent_id != -1)
			joint.absTrans = joint.localTrans * boneArray[bone->parent_id].absTrans;
		else
			joint.absTrans = joint.localTrans;
	}
}

// adds shape to datas
int CEGFPhysicsGenerator::AddShape(Array<dsmvertex_t> &vertices, Array<int> &indices, int shapeType, bool assumedAsConvex)
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

			Msg("Adding convex shape, %d verts %d indices\n", vertices.numElem(), geom_info.numIndices);
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

			Msg("Adding generated convex shape, %d verts %d indices\n", shape_hull.numVertices(), shape_hull.numIndices());
		}
	}
	else
	{
		// just make trimesh

		Array<Vector3D> shapeVerts(PP_SL);

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

		Msg("Adding trimesh shape, %d verts\n", indices.numElem());

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

int CEGFPhysicsGenerator::FindJointIdx(const char* name)
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
void CEGFPhysicsGenerator::SubdivideModelParts( Array<dsmvertex_t>& vertices, Array<int>& indices, Array<IdxIsland>& indexGroups)
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
		const int index = indices[i];
		dsmvertex_t& vertex = vertices[index];

		const int found_index = vertices.findIndex([&vertex](const dsmvertex_t& other) {
			return vertex.position == other.position;
		});

		if(found_index != i && found_index != -1)
			indices[i] = found_index;
	}

	Msg("Building neighbour triangle table... (%d indices)\n", indices.numElem());

	CAdjacentTriangleGraph triangleGraph;
	const Array<Triangle>& triangles = triangleGraph.GetTriangles();

	// build neighbours
	triangleGraph.Build(indices.ptr(),indices.numElem());
	triangleGraph.GetIslands(indexGroups);

	MsgInfo("Detected %d groups out of %d triangles\n", indexGroups.numElem(), triangles.numElem());
}

bool CEGFPhysicsGenerator::CreateRagdollObjects( Array<dsmvertex_t>& vertices, Array<int>& indices, Array<IdxIsland>& indexGroups )
{
	// setup pose bones
	Array<RagdollJoint> ragJoints(PP_SL);
	ragJoints.setNum(m_srcModel->bones.numElem());
	SetupRagdollJoints(ragJoints);

	const KVSection* bonesSect = m_physicsParams->FindSection("Bones", KV_FLAG_SECTION);
	const KVSection* isDynamicProp = m_physicsParams->FindSection("IsDynamic");

	if(KV_GetValueBool(isDynamicProp))
	{
		m_props.usageType = PHYSMODEL_USAGE_DYNAMIC;
		Msg("  Model is dynamic\n");
	}
	else
	{
		m_props.usageType = PHYSMODEL_USAGE_RAGDOLL;
		Msg("  Model is ragdoll\n");
	}

	Msg("Assigning bones to groups...\n");

	Array<int> bone_group_indices(PP_SL);

	for(int i = 0; i < indexGroups.numElem(); i++)
	{
		const IdxIsland& list = indexGroups[i];
		const int firsttri_indx0 = list[0][0];

		int bone_index = -1;
		
		if(vertices[firsttri_indx0].weights.numElem() > 0)
			bone_index = vertices[firsttri_indx0].weights[0].bone;

		if(bone_index != -1)
			Msg("Group %d uses bone %s\n", i+1, m_srcModel->bones[bone_index]->name);
		else
			Msg("Group %d doesn't use bones, it will be static\n", i+1);

		bone_group_indices.append(bone_index);

		for(int j = 1; j < list.numElem(); j++)
		{
			const int idx0 = list[j][0];
			const int idx1 = list[j][1];
			const int idx2 = list[j][2];

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

	const KVSection* pDefaultSurfaceProps = m_physicsParams->FindSection("SurfaceProps");

	for(int i = 0; i < m_srcModel->bones.numElem(); i++)
	{
		Array<int> bone_geom_indices(PP_SL);

		BoundingBox localBox;

		// add indices of attached groups and also build bounding box
		for(int j = 0; j < indexGroups.numElem(); j++)
		{
			const IdxIsland& list = indexGroups[j];

			if (bone_group_indices[j] != i)
				continue;

			for(int k = 0; k < list.numElem(); k++)
			{
				bone_geom_indices.append(list[k][0]);
				bone_geom_indices.append(list[k][1]);
				bone_geom_indices.append(list[k][2]);

				localBox.AddVertex(vertices[list[k][0]].position);
				localBox.AddVertex(vertices[list[k][1]].position);
				localBox.AddVertex(vertices[list[k][2]].position);				
			}
		}

		m_bbox.Merge(localBox);

		// we should have at least more than 3 triangles for convex shapes
		if( bone_geom_indices.numElem() <= 9 )
			continue;

		// compute object center
		const Vector3D object_center = localBox.GetCenter();
				
		// transform objects to origin
		Array<int> processed_index(PP_SL);
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

		memset(object.shapeIndex, -1, sizeof(object.shapeIndex));

		const KVSection* thisBoneSec = bonesSect->FindSection(m_srcModel->bones[i]->name, KV_FLAG_SECTION);

		object.bodyPartId = 0;
		object.numShapes = 1;
		object.shapeIndex[0] = shapeID;
		object.offset = object_center;
		object.massCenter = vec3_zero;
		object.mass = PHYS_DEFAULT_MASS;

		strcpy(object.surfaceprops, KV_GetValueString( pDefaultSurfaceProps, 0, "default" ));

		if( thisBoneSec )
		{
			object.mass = KV_GetValueFloat( thisBoneSec->FindSection("Mass"), 0, PHYS_DEFAULT_MASS );
			object.bodyPartId = KV_GetValueInt(thisBoneSec->FindSection("bodypart"), 0, 0);

			const KVSection* surfPropsPair = thisBoneSec->FindSection("SurfaceProps");

			if(surfPropsPair)
				strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair));
		}

		// build joint information
		physjoint_t joint;

		memset(joint.name, 0, sizeof(joint.name));
		strcpy(joint.name, m_srcModel->bones[i]->name);

		PhyNamedObject obj;
		memset(obj.name, 0, sizeof(obj.name));
		strcpy(obj.name, m_srcModel->bones[i]->name);
		obj.object = object;

		// add object after building
		m_objects.append(obj);

		// setup default limits
		joint.minLimit = vec3_zero;
		joint.maxLimit = vec3_zero;

		// set bone position
		Vector3D bone_position = ragJoints[i].absTrans.rows[3].xyz();
		joint.position = bone_position;

		joint.objA = m_objects.numElem() - 1;

		int parent = m_srcModel->bones[i]->parent_id;

		if(parent == -1)
		{
			// join to itself
			joint.objB = joint.objA;
		}
		else
		{
			int phys_parent = MakeBoneValidParent(i);

			if(phys_parent == -1)
				joint.objB = joint.objA;
			else
				joint.objB = phys_parent;
		}

		if(thisBoneSec)
		{
			// get axis and check limits
			for(int i = 0; i < thisBoneSec->keys.numElem(); i++)
			{
				int axis_idx = -1;

				const KVSection* pKey = thisBoneSec->keys[i];

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

bool CEGFPhysicsGenerator::CreateCompoundOrSeparateObjects( Array<dsmvertex_t>& vertices, Array<int>& indices, Array<IdxIsland>& indexGroups, bool bCompound )
{
	m_props.usageType = PHYSMODEL_USAGE_RIGID_COMP;

	if(indexGroups.numElem() == 1)
		Msg("  Model is single\n");
	else
		Msg("  Model is compound\n");

	// shape types ignored on compound
	bool isConcave = KV_GetValueBool( m_physicsParams->FindSection("concave"), 0, false );
	bool isStatic = KV_GetValueBool( m_physicsParams->FindSection("static"), 0, false );

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
		Array<int> shape_ids(PP_SL);

		for(int i = 0; i < indexGroups.numElem(); i++)
		{
			IdxIsland& list = indexGroups[i];
			Array<int> tmpIndices(PP_SL);

			for(int j = 0; j < list.numElem(); j++)
			{
				tmpIndices.append( list[j][0] );
				tmpIndices.append( list[j][1] );
				tmpIndices.append( list[j][2] );
			}

			int shapeID = AddShape(vertices, tmpIndices);
			shape_ids.append(shapeID);
		}

		physobject_t object;
		object.bodyPartId = 0;

		const KVSection* surfPropsPair = m_physicsParams->FindSection("SurfaceProps");
				
		memset(object.surfaceprops, 0, sizeof(object.surfaceprops));
		strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair, 0, "default"));

		object.mass = KV_GetValueFloat(m_physicsParams->FindSection("Mass"), 0, PHYS_DEFAULT_MASS);

		object.numShapes = shape_ids.numElem();

		if(object.numShapes > MAX_PHYS_GEOM_PER_OBJECT)
		{
			MsgWarning("Exceeded physics shape count (%d)\n", object.numShapes);
			object.numShapes = MAX_PHYS_GEOM_PER_OBJECT;
		}

		memset(object.shapeIndex, -1, sizeof(object.shapeIndex));

		for(int i = 0; i < object.numShapes; i++)
			object.shapeIndex[i] = shape_ids[i];

		object.offset = vec3_zero;
		object.massCenter = KV_GetVector3D( m_physicsParams->FindSection("MassCenter"), 0, m_bbox.GetCenter() );

		PhyNamedObject obj;
		memset(obj.name, 0, sizeof(obj.name));
		strcpy(obj.name, KV_GetValueString(m_physicsParams, 0, EqString::Format("obj_%d", m_objects.numElem()).ToCString()));

		obj.object = object;

		m_objects.append(obj);
	}
	else
	{
		physobject_t object;
		object.bodyPartId = 0;

		memset(object.surfaceprops, 0, 0);
		strcpy(object.surfaceprops, KV_GetValueString(m_physicsParams->FindSection("SurfaceProps"), 0, "default"));

		object.mass = KV_GetValueFloat(m_physicsParams->FindSection("Mass"), 0, PHYS_DEFAULT_MASS);

		for(int i = 0; i < indexGroups.numElem(); i++)
		{
			IdxIsland& list = indexGroups[i];

			Array<int> tmpIndices(PP_SL);

			for(int j = 0; j < list.numElem(); j++)
			{
				tmpIndices.append( list[j][0] );
				tmpIndices.append( list[j][1] );
				tmpIndices.append( list[j][2] );
			}

			int shapeID = AddShape( vertices, tmpIndices, nShapeType );

			object.numShapes = 1;
			memset(object.shapeIndex, -1, sizeof(object.shapeIndex));

			object.shapeIndex[0] = shapeID;
			object.offset = vec3_zero;
			object.massCenter = vec3_zero;

			PhyNamedObject obj;
			obj.object = object;

			memset(obj.name, 0, sizeof(obj.name));

			if(m_physicsParams->values.numElem() > 0)
				strcpy(obj.name, EqString::Format("%s_part%d", KV_GetValueString(m_physicsParams), i).ToCString());
			else
				strcpy(obj.name, EqString::Format("obj_%d", m_objects.numElem()).ToCString());

			m_objects.append(obj);
		}
	}

	return true;
}

bool CEGFPhysicsGenerator::CreateSingleObject( Array<dsmvertex_t>& vertices, Array<int>& indices )
{
	// shape types ignored on compound
	bool isConcave = KV_GetValueBool( m_physicsParams->FindSection("concave"), 0, false );
	bool isStatic = KV_GetValueBool( m_physicsParams->FindSection("static"), 0, false );
	bool isAssumedAsConvex = KV_GetValueBool( m_physicsParams->FindSection("dont_simplify"), 0, false );

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

	object.bodyPartId = 0;

	memset(object.surfaceprops, 0, sizeof(object.surfaceprops));
	strcpy(object.surfaceprops, KV_GetValueString(m_physicsParams->FindSection("SurfaceProps"), 0, "default"));

	object.mass = KV_GetValueFloat(m_physicsParams->FindSection("Mass"), 0, PHYS_DEFAULT_MASS);
	object.massCenter = KV_GetVector3D(m_physicsParams->FindSection("MassCenter"), 0, m_bbox.GetCenter());

	object.numShapes = 1;
	memset(object.shapeIndex, -1, sizeof(object.shapeIndex));
	object.shapeIndex[0] = shapeID;
	object.offset = vec3_zero;

	PhyNamedObject obj;
	memset(obj.name, 0, sizeof(obj.name));
	strcpy(obj.name, KV_GetValueString(m_physicsParams, 0, EqString::Format("obj_%d", m_objects.numElem()).ToCString()));

	obj.object = object;

	m_objects.append(obj);

	return true;
}

bool CEGFPhysicsGenerator::GenerateGeometry(dsmmodel_t* srcModel, const KVSection* physInfo, bool forceGroupSubdivision)
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

	m_forceGroupSubdivision = KV_GetValueBool(m_physicsParams->FindSection("groupdivision"), 0, m_forceGroupSubdivision);

	const KVSection* compoundkey = m_physicsParams->FindSection("compound");
	if(compoundkey)
	{
		bCompound = KV_GetValueBool(compoundkey);
		m_forceGroupSubdivision = bCompound;
	}

	memset(m_props.commentStr, 0, sizeof(m_props.commentStr));
	strcpy(m_props.commentStr, KV_GetValueString(m_physicsParams->FindSection("comments"), 0, ""));

	Array<dsmvertex_t>		vertices(PP_SL);
	Array<int>				indices(PP_SL);

	// if we've got ragdoll
	if( m_forceGroupSubdivision || (m_srcModel->bones.numElem() > 1)  )
	{
		// generate index groups
		Array<IdxIsland> indexGroups(PP_SL);
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
	}
	else
	{
		m_props.usageType = PHYSMODEL_USAGE_RIGID_COMP;

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

ubyte* pData = nullptr;
ubyte* pStart = nullptr;


void WriteLumpToStream(IVirtualStream* stream, int lump_type, ubyte* data, uint dataSize)
{
	lumpfilelump_t lump;
	lump.type = lump_type;
	lump.size = dataSize;

	stream->Write(&lump, 1, sizeof(lump));
	stream->Write(data, 1, dataSize);
}

void CEGFPhysicsGenerator::SaveToFile(const char* filename)
{
	CMemoryStream lumpsStream(nullptr, VS_OPEN_WRITE, MAX_PHYSICSFILE_SIZE);

	WriteLumpToStream(&lumpsStream, PHYSFILE_PROPERTIES, (ubyte*)&m_props, sizeof(physmodelprops_t));
	WriteLumpToStream(&lumpsStream, PHYSFILE_GEOMETRYINFO, (ubyte*)m_shapes.ptr(), sizeof(physgeominfo_t) * m_shapes.numElem());

	// write names lump before objects lump
	// PHYSLUMP_OBJECTNAMES
	{
		CMemoryStream objNamesLump(nullptr, VS_OPEN_WRITE, 2048);

		for(int i = 0; i < m_objects.numElem(); i++)
			objNamesLump.Write(m_objects[i].name, 1, strlen(m_objects[i].name)+1);

		char nullChar = '\0';
		objNamesLump.Write(&nullChar, 1, 1);

		WriteLumpToStream(&lumpsStream, PHYSFILE_OBJECTNAMES,		(ubyte*)objNamesLump.GetBasePointer(), objNamesLump.Tell());
	}


	// PHYSLUMP_OBJECTS
	{
		CMemoryStream objDataLump(nullptr, VS_OPEN_WRITE, 2048);

		for(int i = 0; i < m_objects.numElem(); i++)
			objDataLump.Write(&m_objects[i].object, 1, sizeof(m_objects[i].object));

		WriteLumpToStream(&lumpsStream, PHYSFILE_OBJECTS,		(ubyte*)objDataLump.GetBasePointer(), objDataLump.Tell());
	}

	WriteLumpToStream(&lumpsStream, PHYSFILE_INDEXDATA,	(ubyte*)m_indices.ptr(), sizeof(int) * m_indices.numElem());
	WriteLumpToStream(&lumpsStream, PHYSFILE_VERTEXDATA, (ubyte*)m_vertices.ptr(), sizeof(Vector3D) * m_vertices.numElem());
	WriteLumpToStream(&lumpsStream, PHYSFILE_JOINTDATA,	(ubyte*)m_joints.ptr(), sizeof(physjoint_t) * m_joints.numElem());

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

	lumpfilehdr_t header;
	header.ident = PHYSFILE_ID;
	header.version = PHYSFILE_VERSION;
	header.numLumps = PHYSFILE_LUMPS;

	outputFile->Write(&header, 1, sizeof(header));
	lumpsStream.WriteToFileStream(outputFile);

	g_fileSystem->Close(outputFile);
}