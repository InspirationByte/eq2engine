//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	char			name[32]{ 0 };
	physobject_t	object;
};

//---------------------------------------------------------------------------------------------------

CEGFPhysicsGenerator::CEGFPhysicsGenerator()
	: m_srcModel(nullptr)
	, m_physicsParams(nullptr)
	, m_forceGroupSubdivision(false)
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
		const DSBone& bone = m_srcModel->bones[i];

		// setup transformation
		joint.localTrans = identity4;

		joint.localTrans.setRotation(bone.angles);
		joint.localTrans.setTranslation(bone.position);

		if(bone.parentIdx != -1)
			joint.absTrans = joint.localTrans * boneArray[bone.parentIdx].absTrans;
		else
			joint.absTrans = joint.localTrans;
	}
}

// adds shape to datas
int CEGFPhysicsGenerator::AddShape(ArrayCRef<DSVertex> vertices, ArrayCRef<int> indices, float margin, EPhysShapeType shapeType, bool assumedAsConvex)
{
	physgeominfo_t geomInfo;
	geomInfo.type = shapeType;

	geomInfo.startIndices = m_indices.numElem();

	// make hull
	if( geomInfo.type == PHYSSHAPE_TYPE_CONVEX)
	{
		if(assumedAsConvex)
		{
			const int startIndex = m_vertices.numElem();

			for(int i = 0; i < indices.numElem(); i+=3)
			{
				m_vertices.append(vertices[indices[i]].position);
				m_vertices.append(vertices[indices[i + 1]].position);
				m_vertices.append(vertices[indices[i + 2]].position);

				m_indices.append(i + startIndex);
				m_indices.append(i + 1 + startIndex);
				m_indices.append(i + 2 + startIndex);
			}

			geomInfo.numIndices = indices.numElem();

			Msg("Adding convex shape, %d verts %d indices\n", vertices.numElem(), geomInfo.numIndices);
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

			// make shape hull
			btShapeHull shapeHull(&tmpConvexShape);
			tmpConvexShape.setMargin(margin);

			// cook hull
			shapeHull.buildHull( 0.0 /*this even not work*/, 0 );

			// finally, add indices and vertices:
			const int startIndex = m_vertices.numElem();
			for(int i = 0; i < shapeHull.numVertices(); i++)
			{
				const btVector3& vertex = shapeHull.getVertexPointer()[i];
				m_vertices.append(Vector3D(vertex[0], vertex[1], vertex[2]));
			}

			for(int i = 0; i < shapeHull.numIndices(); i++)
				m_indices.append(shapeHull.getIndexPointer()[i] + startIndex);

			geomInfo.numIndices = shapeHull.numIndices();

			Msg("Adding generated convex shape, %d verts %d indices\n", shapeHull.numVertices(), shapeHull.numIndices());
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

			const int found_idx = arrayFindIndex(shapeVerts, pos);
			if(found_idx == -1 && indices[i] != found_idx)
			{
				int nVerts = shapeVerts.append(pos);
				m_indices.append(nVerts + start_vertex);
			}
			else
				m_indices.append(found_idx + start_vertex);
		}

		geomInfo.numIndices = m_indices.numElem() - geomInfo.startIndices;

		Msg("Adding trimesh shape, %d verts\n", indices.numElem());

		for(int i = 0; i < shapeVerts.numElem(); i++)
		{
			m_vertices.append(shapeVerts[i]);
		}
	}

	return m_shapes.append(geomInfo);
}

//
// Joint valid parenting
//

int CEGFPhysicsGenerator::FindJointIdx(const char* name)
{
	return arrayFindIndexF(m_joints, [=](const physjoint_t& joint) {
		return CString::CompareCaseIns(joint.name, name) == 0;
	});
}

int CEGFPhysicsGenerator::MakeBoneValidParent(int boneId)
{
	int parent = m_srcModel->bones[boneId].parentIdx;
	int jointIdx = -1;

	if(parent != -1)
	{
		jointIdx = FindJointIdx(m_srcModel->bones[parent].name);
		if(jointIdx == -1)
			jointIdx = MakeBoneValidParent(parent);
		else
			return jointIdx;
	}

	return jointIdx;
}

// this procedure useful for ragdolls
// it collects information about neighbour surfaces and joins triangles into subparts
void CEGFPhysicsGenerator::SubdivideModelParts( Array<DSVertex>& vertices, Array<int>& indices, Array<IdxIsland>& indexGroups)
{
	for(int i = 0 ; i < m_srcModel->meshes.numElem(); i++)
	{
		DSMesh* group = m_srcModel->meshes[i];
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
		DSVertex& vertex = vertices[index];

		const int found_index = arrayFindIndexF(vertices, [&vertex](const DSVertex& other) {
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

void CEGFPhysicsGenerator::CreateRagdollObjects(const KVSection* bonesSect, ArrayRef<DSVertex> vertices, ArrayCRef<int> indices, ArrayCRef<IdxIsland> indexGroups )
{
	// setup pose bones
	Array<RagdollJoint> ragJoints(PP_SL);
	ragJoints.setNum(m_srcModel->bones.numElem());
	SetupRagdollJoints(ragJoints);

	float defaultMass = PHYS_DEFAULT_MASS;
	m_physicsParams->Get("Mass").GetValues(defaultMass);

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

	Array<int> boneGroupIndices(PP_SL);
	for(int i = 0; i < indexGroups.numElem(); i++)
	{
		const IdxIsland& list = indexGroups[i];
		const int firstTriIdx = list.front()[0];

		int boneIdx = -1;
		if(vertices[firstTriIdx].weights.numElem() > 0)
			boneIdx = vertices[firstTriIdx].weights[0].bone;
		boneGroupIndices.append(boneIdx);

		if(boneIdx != -1)
			Msg("Mesh %d uses bone %s\n", i+1, m_srcModel->bones[boneIdx].name.ToCString());
		else
			Msg("Mesh %d doesn't use bones, it will be static\n", i+1);

		for(int j = 1; j < list.numElem(); j++)
		{
			const ITriangle tri = list[j];
			for (int k = 0; k < 3; ++k)
			{
				if (vertices[tri[k]].weights.front().bone != boneIdx)
				{
					MsgError("Invalid bone id. Mesh part must use single bone index.\n");
					MsgError("Please separate model parts for bones.\n");
					break;
				}
			}						
		}
	}

	Array<int> boneGeomIndices(PP_SL);
	EqStringRef defaultSurfaceProps;
	m_physicsParams->Get("SurfaceProps").GetValues(defaultSurfaceProps);

	for(int i = 0; i < m_srcModel->bones.numElem(); i++)
	{
		const DSBone& bone = m_srcModel->bones[i];
		boneGeomIndices.clear();

		BoundingBox localBox;

		// add indices of attached groups and also build bounding box
		for(int j = 0; j < indexGroups.numElem(); j++)
		{
			if (boneGroupIndices[j] != i)
				continue;

			const IdxIsland& list = indexGroups[j];
			for(const ITriangle& tri : list)
			{
				for (int k = 0; k < 3; ++k)
				{
					boneGeomIndices.append(tri[k]);
					localBox.AddVertex(vertices[tri[k]].position);
				}			
			}
		}

		m_bbox.Merge(localBox);

		// we should have at least more than 3 triangles for convex shapes
		if( boneGeomIndices.numElem() <= 9 )
			continue;

		// compute object center
		const Vector3D objCenter = localBox.GetCenter();
				
		// transform objects to origin
		Array<int> processed_index(PP_SL);
		for(int idx : boneGeomIndices)
		{
			if(arrayFindIndex(processed_index, idx) == -1)
			{
				vertices[idx].position -= objCenter;
				processed_index.append(idx);
			}
		}
				
		// generate physics shape
		const int shapeID = AddShape(vertices, boneGeomIndices);

		// build object data
		physobject_t object;
		memset(object.shapeIndex, -1, sizeof(object.shapeIndex));
		object.bodyPartId = 0;
		object.numShapes = 1;
		object.shapeIndex[0] = shapeID;
		object.offset = objCenter;
		object.massCenter = vec3_zero;
		object.mass = defaultMass;

		EqStringRef surfaceProps = defaultSurfaceProps;

		const KVSection* thisBoneSec = bonesSect->FindSection(bone.name, KV_FLAG_SECTION);
		if( thisBoneSec )
		{
			thisBoneSec->Get("Mass").GetValues(object.mass);
			thisBoneSec->Get("BodyPart").GetValues(object.bodyPartId);
		}

		strcpy(object.surfaceprops, surfaceProps);

		// build joint information
		physjoint_t joint;

		memset(joint.name, 0, sizeof(joint.name));
		strcpy(joint.name, bone.name);

		PhyNamedObject obj;
		memset(obj.name, 0, sizeof(obj.name));
		strcpy(obj.name, bone.name);
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

		if(bone.parentIdx == -1)
		{
			// join to itself
			joint.objB = joint.objA;
		}
		else
		{
			const int physParentId = MakeBoneValidParent(i);
			if(physParentId == -1)
				joint.objB = joint.objA;
			else
				joint.objB = physParentId;
		}

		if(thisBoneSec)
		{
			// get axis and check limits
			for(const KVSection& pKey : thisBoneSec->Keys())
			{
				int axisIdx = -1;
				if( !pKey.name.CompareCaseIns("x_axis") )
					axisIdx = 0;
				else if( !pKey.name.CompareCaseIns("y_axis") )
					axisIdx = 1;
				else if( !pKey.name.CompareCaseIns("z_axis") )
					axisIdx = 2;

				if (axisIdx == -1)
					continue;

				// check the value for arguments
				for(int j = 0; j < pKey.values.numElem(); j++)
				{
					if( !CString::CompareCaseIns(KV_GetValueString(&pKey, j), "limit" ))
					{
						// read limits
						const float highLimit = KV_GetValueFloat(&pKey, j+1);
						const float lowLimit = KV_GetValueFloat(&pKey, j+2);

						joint.minLimit[axisIdx] = DEG2RAD(highLimit);
						joint.maxLimit[axisIdx] = DEG2RAD(lowLimit);
					}
					else if( !CString::CompareCaseIns(KV_GetValueString(&pKey, j), "limitOffset" ))
					{
						const float offs = KV_GetValueFloat(&pKey, j+1);

						joint.minLimit[axisIdx] += DEG2RAD(offs);
						joint.maxLimit[axisIdx] += DEG2RAD(offs);
					}
				}
			}
		}

		// add new joint
		m_joints.append(joint);
	}
}

void CEGFPhysicsGenerator::CreateCompoundObject(ArrayCRef<DSVertex> vertices, ArrayCRef<int> indices, ArrayCRef<IdxIsland> indexGroups)
{
	EqString objName = EqString::Format("obj_%d", m_objects.numElem());
	m_physicsParams->GetValues(objName);

	PhyNamedObject& obj = m_objects.append();
	strncpy(obj.name, objName, sizeof(obj.name));
	obj.name[sizeof(obj.name) - 1] = 0;

	physobject_t& object = obj.object;
	memset(object.shapeIndex, -1, sizeof(object.shapeIndex));
	object.bodyPartId = 0;
	object.numShapes = 0;
	object.offset = vec3_zero;
	object.massCenter = m_bbox.GetCenter();

	m_physicsParams->Get("MassCenter").GetValues(object.massCenter);
	m_physicsParams->Get("Mass").GetValues(object.mass);

	EqStringRef surfaceProps = "default";
	m_physicsParams->Get("SurfaceProps").GetValues(surfaceProps);
	strcpy(object.surfaceprops, surfaceProps);

	for (const IdxIsland& tris : indexGroups)
	{
		if (object.numShapes >= MAX_PHYS_GEOM_PER_OBJECT)
		{
			MsgWarning("Exceeded physics shape count (%d)\n", object.numShapes);
			break;
		}

		object.shapeIndex[object.numShapes++] = AddShape(vertices, ArrayCRef(&tris[0].x, tris.numElem() * 3), PHYSSHAPE_TYPE_CONVEX);
	}
}


void CEGFPhysicsGenerator::CreateMultipleObjects(ArrayCRef<DSVertex> vertices, ArrayCRef<int> indices, ArrayCRef<IdxIsland> indexGroups)
{
	if (indexGroups.numElem() == 1)
		Msg("  Model is single\n");
	else
		Msg("  Model is compound\n");

	bool isAssumedAsConvex = false;
	m_physicsParams->Get("dont_simplify").GetValues(isAssumedAsConvex);

	bool isConcave = false;
	bool isStatic = false;
	m_physicsParams->Get("static").GetValues(isStatic);
	m_physicsParams->Get("concave").GetValues(isConcave);

	EPhysShapeType shapeType = isConcave ? PHYSSHAPE_TYPE_MOVABLECONCAVE : PHYSSHAPE_TYPE_CONVEX;
	if (isStatic && isConcave)
		shapeType = PHYSSHAPE_TYPE_CONCAVE;

	EqStringRef surfaceProps = "default";
	m_physicsParams->Get("SurfaceProps").GetValues(surfaceProps);

	float objectMass = PHYS_DEFAULT_MASS;
	m_physicsParams->Get("Mass").GetValues(objectMass);

	float margin = 0.0f;
	m_physicsParams->Get("Margin").GetValues(margin);

	int islandIdx = 0;
	for (const IdxIsland& tris : indexGroups)
	{
		EqString objName;
		if (m_physicsParams->values.numElem() > 0)
			objName = EqString::Format("%s_part%d", KV_GetValueString(m_physicsParams), islandIdx);
		else
			objName = EqString::Format("obj_%d", m_objects.numElem());

		PhyNamedObject& obj = m_objects.append();
		strncpy(obj.name, objName, sizeof(obj.name));
		obj.name[sizeof(obj.name) - 1] = 0;

		physobject_t& object = obj.object;
		memset(object.shapeIndex, -1, sizeof(object.shapeIndex));
		object.shapeIndex[0] = AddShape(vertices, ArrayCRef(&tris[0].x, tris.numElem() * 3), margin, shapeType, isAssumedAsConvex);
		object.numShapes = 1;
		object.bodyPartId = islandIdx;
		object.offset = vec3_zero;
		object.massCenter = vec3_zero;
		object.mass = objectMass;

		strncpy(object.surfaceprops, surfaceProps, sizeof(object.surfaceprops));
		object.surfaceprops[sizeof(object.surfaceprops) - 1] = 0;

		++islandIdx;
	}
}

void CEGFPhysicsGenerator::CreateSingleObject(ArrayCRef<DSVertex> vertices, ArrayCRef<int> indices )
{
	EqString objName = EqString::Format("obj_%d", m_objects.numElem());
	m_physicsParams->GetValues(objName);

	bool isAssumedAsConvex = false;
	m_physicsParams->Get("dont_simplify").GetValues(isAssumedAsConvex);

	bool isConcave = false;
	bool isStatic = false;
	m_physicsParams->Get("static").GetValues(isStatic);
	m_physicsParams->Get("concave").GetValues(isConcave);

	float margin = 0.0f;
	m_physicsParams->Get("Margin").GetValues(margin);

	EPhysShapeType shapeType = isConcave ? PHYSSHAPE_TYPE_MOVABLECONCAVE : PHYSSHAPE_TYPE_CONVEX;
	if (isStatic && isConcave)
		shapeType = PHYSSHAPE_TYPE_CONCAVE;

	EqStringRef surfaceProps = "default";
	m_physicsParams->Get("SurfaceProps").GetValues(surfaceProps);

	PhyNamedObject& obj = m_objects.append();
	strncpy(obj.name, objName, sizeof(obj.name));
	obj.name[sizeof(obj.name) - 1] = 0;

	physobject_t& object = obj.object;
	memset(object.shapeIndex, -1, sizeof(object.shapeIndex));
	object.shapeIndex[0] = AddShape(vertices, indices, margin, shapeType, isAssumedAsConvex);
	object.numShapes = 1;
	object.offset = vec3_zero;
	object.mass = KV_GetValueFloat(m_physicsParams->FindSection("Mass"), 0, PHYS_DEFAULT_MASS);
	object.massCenter = KV_GetVector3D(m_physicsParams->FindSection("MassCenter"), 0, m_bbox.GetCenter());
	object.bodyPartId = 0;

	strncpy(object.surfaceprops, surfaceProps, sizeof(object.surfaceprops));
	object.surfaceprops[sizeof(object.surfaceprops) - 1] = 0;
}

bool CEGFPhysicsGenerator::GenerateGeometry(DSModel* srcModel, const KVSection& physInfo, bool forceGroupSubdivision)
{
	m_srcModel = srcModel;
	m_physicsParams = &physInfo;
	m_forceGroupSubdivision = forceGroupSubdivision;

	if(!m_srcModel || !m_physicsParams)
		return false;

	MsgInfo("Generating physics geometry...\n");

	if(m_srcModel->bones.numElem() > 0)
		m_forceGroupSubdivision = true;
	m_physicsParams->Get("groupdivision").GetValues(m_forceGroupSubdivision);

	bool bCompound = false;
	if(m_physicsParams->Get("compound").GetValues(bCompound))
		m_forceGroupSubdivision = bCompound;

	memset(m_props.commentStr, 0, sizeof(m_props.commentStr));
	strcpy(m_props.commentStr, KV_GetValueString(m_physicsParams->FindSection("comments"), 0, ""));

	Array<DSVertex> vertices(PP_SL);
	Array<int> indices(PP_SL);

	m_props.usageType = PHYSMODEL_USAGE_RIGID_COMP;

	// if we've got ragdoll
	if( m_forceGroupSubdivision || (m_srcModel->bones.numElem() > 1)  )
	{
		// generate index groups
		Array<IdxIsland> indexGroups(PP_SL);
		SubdivideModelParts(vertices, indices, indexGroups);

		Msg("Processed %d verts and %d indices\n", vertices.numElem(), indices.numElem());

		const KVSection* bonesSect = m_physicsParams->FindSection("bones");

		if (!bonesSect && m_srcModel->bones.numElem() > 1)
			MsgError("No physics.bones section found, compiling as regular physics model\n");

		// generate ragdoll
		if(bonesSect && m_srcModel->bones.numElem() > 1)
			CreateRagdollObjects(bonesSect, vertices, indices, indexGroups);
		else if(bCompound)
			CreateCompoundObject(vertices, indices, indexGroups);
		else
			CreateMultipleObjects(vertices, indices, indexGroups);
	}
	else
	{
		// move all vertices and indices from groups to shared buffer (no multiple shapes)
		for(const DSMesh* group : m_srcModel->meshes)
		{
			for(DSVertex& vert : group->verts)
			{
				indices.append(vertices.numElem());
				vertices.append(vert);

				m_bbox.AddVertex(vert.position);
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

void WriteLumpToStream(IFileStream* stream, int lump_type, ubyte* data, uint dataSize)
{
	lumpfilelump_t lump;
	lump.type = lump_type;
	lump.size = dataSize;

	stream->Write(&lump, 1, sizeof(lump));
	stream->Write(data, 1, dataSize);
}

void CEGFPhysicsGenerator::SaveToFile(const char* filename)
{
	CMemoryStream lumpsStream(nullptr, FS_OPEN_WRITE, MAX_PHYSICSFILE_SIZE, PP_SL);

	WriteLumpToStream(&lumpsStream, PHYSFILE_PROPERTIES, (ubyte*)&m_props, sizeof(physmodelprops_t));
	WriteLumpToStream(&lumpsStream, PHYSFILE_SHAPEINFO, (ubyte*)m_shapes.ptr(), sizeof(physgeominfo_t) * m_shapes.numElem());

	// write names lump before objects lump
	// PHYSLUMP_OBJECTNAMES
	{
		CMemoryStream objNamesLump(nullptr, FS_OPEN_WRITE, 2048, PP_SL);

		for(int i = 0; i < m_objects.numElem(); i++)
			objNamesLump.Write(m_objects[i].name, 1, strlen(m_objects[i].name)+1);

		char nullChar = '\0';
		objNamesLump.Write(&nullChar, 1, 1);

		WriteLumpToStream(&lumpsStream, PHYSFILE_OBJECTNAMES,		(ubyte*)objNamesLump.GetBasePointer(), objNamesLump.Tell());
	}


	// PHYSLUMP_OBJECTS
	{
		CMemoryStream objDataLump(nullptr, FS_OPEN_WRITE, 2048, PP_SL);

		for(int i = 0; i < m_objects.numElem(); i++)
			objDataLump.Write(&m_objects[i].object, 1, sizeof(m_objects[i].object));

		WriteLumpToStream(&lumpsStream, PHYSFILE_OBJECTS,		(ubyte*)objDataLump.GetBasePointer(), objDataLump.Tell());
	}

	WriteLumpToStream(&lumpsStream, PHYSFILE_INDEXDATA,	(ubyte*)m_indices.ptr(), sizeof(int) * m_indices.numElem());
	WriteLumpToStream(&lumpsStream, PHYSFILE_VERTEXDATA, (ubyte*)m_vertices.ptr(), sizeof(Vector3D) * m_vertices.numElem());
	WriteLumpToStream(&lumpsStream, PHYSFILE_JOINTDATA,	(ubyte*)m_joints.ptr(), sizeof(physjoint_t) * m_joints.numElem());

	Msg("Total lumps size: %" PRId64 "\n", lumpsStream.GetSize());

	Msg("Total:\n");
	Msg("  Vertex count: %d\n", m_vertices.numElem());
	Msg("  Index count: %d\n", m_indices.numElem());
	Msg("  Shape count: %d\n", m_shapes.numElem());
	Msg("  Object count: %d\n", m_objects.numElem());
	Msg("  Joints count: %d\n", m_joints.numElem());
	
	IFileStreamPtr outputFile = g_fileSystem->Open(filename, FS_OPEN_WRITE);
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
	lumpsStream.WriteToStream(outputFile);
}