//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics model generator
//
//				TODO: refactoring of code here
//////////////////////////////////////////////////////////////////////////////////

#include "physgen_process.h"
#include "coord.h"
#include "mtriangle_framework.h"

using namespace MTriangle;

// physics
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "utils/geomtools.h"

#include "physmodel.h"

#define POINT_TO_BBOX(p, bmin, bmax)\
{									\
	if ( p.x < bmin.x )				\
		bmin.x = p.x;				\
	if ( p.x > bmax.x )				\
		bmax.x = p.x;				\
									\
	if ( p.y < bmin.y )				\
		bmin.y = p.y;				\
	if ( p.y > bmax.y )				\
		bmax.y = p.y;				\
									\
	if ( p.z < bmin.z )				\
		bmin.z = p.z;				\
	if ( p.z > bmax.z )				\
		bmax.z = p.z;	}

// TODO: move to utils
ColorRGB UTIL_StringToColor3(const char *str)
{
	ColorRGB col(0,0,0);

	if(str)
		sscanf(str, "%f %f %f", &col.x, &col.y, &col.z);

	return col;
}

// physgen globals

dsmmodel_t*					model = NULL;
kvkeybase_t*				script = NULL;
bool						forcegroupdivision = false;

int							modeltype = PHYSMODEL_USAGE_RIGID_COMP; // default is rigid or compound mesh

extern Vector3D				g_scalemodel;

physmodelprops_t			g_props;
DkList<physobject_t>		g_physicsobjects;
DkList<physgeominfo_t>		g_geometryinfos;
DkList<Vector3D>			g_vertices;
DkList<int>					g_indices;
DkList<physjoint_t>			g_joints;

Vector3D					g_modelBoxMins(MAX_COORD_UNITS);
Vector3D					g_modelBoxMaxs(-MAX_COORD_UNITS);

struct ragdolljoint_t
{
	Matrix4x4 localTrans;
	Matrix4x4 absTrans;
};

DkList<ragdolljoint_t>		g_ragdoll_joints;

void SetupRagdollBones()
{
	g_ragdoll_joints.setNum( model->bones.numElem() );

	// setup each bone's transformation
	for(int i = 0; i < model->bones.numElem(); i++)
	{
		ragdolljoint_t* joint = &g_ragdoll_joints[i];
		dsmskelbone_t* bone = model->bones[i];

		// setup transformation
		joint->localTrans = identity4();

		joint->localTrans.setRotation(bone->angles);
		joint->localTrans.setTranslation(bone->position);

		if(bone->parent_id != -1)
			joint->absTrans = joint->localTrans * g_ragdoll_joints[bone->parent_id].absTrans;
		else
			joint->absTrans = joint->localTrans;
	}
}

// adds shape to datas
int AddShape(DkList<dsmvertex_t> &vertices, DkList<int> &indices, int shapeType = PHYSSHAPE_TYPE_CONVEX, bool assumedAsConvex = false)
{
	physgeominfo_t geom_info;
	geom_info.type = shapeType;

	geom_info.startIndices = g_indices.numElem();

	// make hull
	if( geom_info.type == PHYSSHAPE_TYPE_CONVEX)
	{
		if(assumedAsConvex)
		{
			int startIndex = g_vertices.numElem();

			for(int i = 0; i < indices.numElem(); i+=3)
			{
				Vector3D pos1 = vertices[indices[i]].position;
				Vector3D pos2 = vertices[indices[i+1]].position;
				Vector3D pos3 = vertices[indices[i+2]].position;

				g_vertices.append(pos1);
				g_vertices.append(pos2);
				g_vertices.append(pos3);

				g_indices.append(i+startIndex);
				g_indices.append(i+1+startIndex);
				g_indices.append(i+2+startIndex);
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

			int startIndex = g_vertices.numElem();

			for(int i = 0; i < shape_hull.numVertices(); i++)
			{
				btVector3 vertex = shape_hull.getVertexPointer()[i];
				g_vertices.append(Vector3D(vertex.x(),vertex.y(),vertex.z()));
			}

			for(int i = 0; i < shape_hull.numIndices(); i++)
			{
				g_indices.append(shape_hull.getIndexPointer()[i] + startIndex);
			}

			geom_info.numIndices = shape_hull.numIndices();

			Msg("Adding convex shape, %d verts %d indices\n", shape_hull.numVertices(), shape_hull.numIndices());
		}
	}
	else
	{
		// just make trimesh

		DkList<Vector3D> shapeVerts;

		int start_vertex = g_vertices.numElem();

		for(int i = 0; i < indices.numElem(); i++)
		{
			Vector3D pos = vertices[indices[i]].position;

			int found_idx = shapeVerts.findIndex(pos);
			if(found_idx == -1 && indices[i] != found_idx)
			{
				int nVerts = shapeVerts.append(pos);
				g_indices.append(nVerts + start_vertex);
			}
			else
				g_indices.append(found_idx + start_vertex);
		}

		geom_info.numIndices = g_indices.numElem() - geom_info.startIndices;

		Msg("Adding shape, %d verts\n", indices.numElem());

		for(int i = 0; i < shapeVerts.numElem(); i++)
		{
			g_vertices.append(shapeVerts[i]);
		}
	}

	return g_geometryinfos.append(geom_info);
}

struct itriangle
{
	int idxs[3];
};

typedef DkList<itriangle> indxgroup_t;

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

//
// Joint valid parenting
//

int find_joint(char* name)
{
	for(int i = 0; i < g_joints.numElem(); i++)
	{
		if(!stricmp(g_joints[i].name, name))
		{
			return i;
		}
	}

	return -1;
}


int make_valid_parent(int index)
{
	int parent = model->bones[index]->parent_id;

	int joint_index = -1;

	if(parent != -1)
	{
		joint_index = find_joint(model->bones[parent]->name);

		if(joint_index == -1)
			joint_index = make_valid_parent(parent);
		else
			return joint_index;
	}

	return joint_index;
}

// this procedure useful for ragdolls
// it collects information about neighbour surfaces and joins triangles into subparts
void SubdivideModelParts( DkList<dsmvertex_t>& vertices, DkList<int>& indices, DkList<indxgroup_t*>& groups )
{
	for(int i = 0 ; i < model->groups.numElem(); i++)
	{
		for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
		{
			indices.append(vertices.numElem());
			vertices.append(model->groups[i]->verts[j]);
			POINT_TO_BBOX(vertices[i].position, g_modelBoxMins,g_modelBoxMaxs )
		}
	}

	// connect same vertices
	for(int i = 0; i < indices.numElem(); i++)
	{
		int index = indices[i];
		dsmvertex_t vertex = vertices[index];

		int found_index = find_vertex(vertices, vertex);

		if(found_index != i && found_index != -1)
		{
			indices[i] = found_index;
		}
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

void CreateRagdollObjects( DkList<dsmvertex_t>& vertices, DkList<int>& indices, DkList<indxgroup_t*>& indexGroups )
{
	// setup bones for it
	SetupRagdollBones();

	kvkeybase_t* pBonesSect = script->FindKeyBase("Bones",KV_FLAG_SECTION);

	kvkeybase_t* pair = script->FindKeyBase("IsDynamic");

	if(KV_GetValueBool(pair))
	{
		modeltype = PHYSMODEL_USAGE_DYNAMIC;
		Msg("  Model is dynamic\n");
	}
	else
	{
		modeltype = PHYSMODEL_USAGE_RAGDOLL;
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
			Msg("Group %d uses bone %s\n", i+1, model->bones[bone_index]->name);
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
				exit(0);
			}
						
		}
	}

	kvkeybase_t* pDefaultSurfaceProps = script->FindKeyBase("SurfaceProps");

	for(int i = 0; i < model->bones.numElem(); i++)
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

					POINT_TO_BBOX(vertices[ indexGroups[j]->ptr()[k].idxs[0] ].position, bboxMins, bboxMaxs);
					POINT_TO_BBOX(vertices[ indexGroups[j]->ptr()[k].idxs[1] ].position, bboxMins, bboxMaxs);
					POINT_TO_BBOX(vertices[ indexGroups[j]->ptr()[k].idxs[2] ].position, bboxMins, bboxMaxs);
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

		kvkeybase_t* pThisBoneSec = pBonesSect->FindKeyBase(model->bones[i]->name, KV_FLAG_SECTION);

		object.body_part = 0;
		object.numShapes = 1;
		object.shape_indexes[0] = shapeID;
		object.offset = object_center;
		object.mass_center = vec3_zero;
		object.mass = DEFAULT_MASS;

		strcpy(object.surfaceprops, KV_GetValueString( pDefaultSurfaceProps, 0, "default" ));

		if( pThisBoneSec )
		{
			object.mass = KV_GetValueFloat( pThisBoneSec->FindKeyBase("Mass"), 0, DEFAULT_MASS );
			object.body_part = KV_GetValueInt(pThisBoneSec->FindKeyBase("bodypart"), 0, 0);

			kvkeybase_t* surfPropsPair = pThisBoneSec->FindKeyBase("SurfaceProps");

			if(surfPropsPair)
				strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair));
		}

		// add object after building
		g_physicsobjects.append(object);

		// build joint information
		physjoint_t joint;

		memset(joint.name, 0, sizeof(joint.name));
		strcpy(joint.name, model->bones[i]->name);

		// setup default limits
		joint.minLimit = vec3_zero;
		joint.maxLimit = vec3_zero;

		// set bone position
		Vector3D bone_position = g_ragdoll_joints[i].absTrans.rows[3].xyz();
		joint.position = bone_position;

		joint.object_indexA = g_physicsobjects.numElem() - 1;

		int parent = model->bones[i]->parent_id;

		if(parent == -1)
		{
			// join to itself
			joint.object_indexB = joint.object_indexA;
		}
		else
		{
			int phys_parent = make_valid_parent(i);

			if(phys_parent == -1)
				joint.object_indexB = joint.object_indexA;
			else
				joint.object_indexB = phys_parent;
		}

		if(pThisBoneSec)
		{
			// get axis and check limits
			for(int i = 0; i < pThisBoneSec->keys.numElem(); i++)
			{
				int axis_idx = -1;

				kvkeybase_t* pKey = pThisBoneSec->keys[i];

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
		g_joints.append(joint);
	}

	// next
}

void CreateCompoundOrSeparateObjects( DkList<dsmvertex_t>& vertices, DkList<int>& indices, DkList<indxgroup_t*>& indexGroups, bool bCompound )
{
	modeltype = PHYSMODEL_USAGE_RIGID_COMP;

	if(indexGroups.numElem() == 1)
		Msg("  Model is single\n");
	else
		Msg("  Model is compound\n");

	// shape types ignored on compound
	bool isConcave = KV_GetValueBool( script->FindKeyBase("concave"), 0, false );
	bool isStatic = KV_GetValueBool( script->FindKeyBase("static"), 0, false );

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

		kvkeybase_t* surfPropsPair = script->FindKeyBase("SurfaceProps");
				
		memset(object.surfaceprops, 0, 0);

		if(!surfPropsPair)
			strcpy(object.surfaceprops, "default");
		else
			strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair));

		kvkeybase_t* pair = script->FindKeyBase("Mass");

		if(pair)
			object.mass = KV_GetValueFloat(pair);
		else
			object.mass = DEFAULT_MASS;

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

		pair = script->FindKeyBase("MassCenter");
		if(pair)
			object.mass_center = KV_GetVector3D( pair );
		else
			object.mass_center = (g_modelBoxMins+g_modelBoxMaxs)*0.5f;

		g_physicsobjects.append(object);
	}
	else
	{
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
			physobject_t object;

			object.body_part = 0;

			kvkeybase_t* surfPropsPair = script->FindKeyBase("SurfaceProps");
				
			memset(object.surfaceprops, 0, 0);

			if(!surfPropsPair)
				strcpy(object.surfaceprops, "default");
			else
				strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair));

			kvkeybase_t* pair = script->FindKeyBase("Mass");


			if(pair)
				object.mass = KV_GetValueFloat(pair);
			else
				object.mass = DEFAULT_MASS;

			object.numShapes = 1;
			memset(object.shape_indexes, -1, sizeof(object.shape_indexes));
			object.shape_indexes[0] = shapeID;
			object.offset = vec3_zero;
			object.mass_center = vec3_zero;

			g_physicsobjects.append(object);
		}
	}
}

void CreateSingleObject( DkList<dsmvertex_t>& vertices, DkList<int>& indices )
{
	// shape types ignored on compound
	bool isConcave = KV_GetValueBool( script->FindKeyBase("concave"), 0, false );
	bool isStatic = KV_GetValueBool( script->FindKeyBase("static"), 0, false );
	bool isAssumedAsConvex = KV_GetValueBool( script->FindKeyBase("dont_simplify"), 0, false );

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

	kvkeybase_t* surfPropsPair = script->FindKeyBase("SurfaceProps");
		
	memset(object.surfaceprops, 0, 0);

	if(!surfPropsPair)
		strcpy(object.surfaceprops, "default");
	else
		strcpy(object.surfaceprops, KV_GetValueString(surfPropsPair));

	kvkeybase_t* pair = script->FindKeyBase("Mass");

	if(pair)
		object.mass = KV_GetValueFloat(pair);
	else
		object.mass = DEFAULT_MASS;

	pair = script->FindKeyBase("MassCenter");
	if(pair)
		object.mass_center = KV_GetVector3D(pair);
	else
		object.mass_center = (g_modelBoxMins+g_modelBoxMaxs)*0.5f;

	object.numShapes = 1;
	memset(object.shape_indexes, -1, sizeof(object.shape_indexes));
	object.shape_indexes[0] = shapeID;
	object.offset = vec3_zero;

	g_physicsobjects.append(object);
}

void MakeGeometry()
{
	bool bCompound = false;

	if(model->bones.numElem() > 0)
		forcegroupdivision = true;

	kvkeybase_t* groupdivision = script->FindKeyBase("groupdivision");

	if(groupdivision)
		forcegroupdivision = KV_GetValueBool(groupdivision);

	kvkeybase_t* compoundkey = script->FindKeyBase("compound");
	if(compoundkey)
	{
		bCompound = KV_GetValueBool(compoundkey);
		forcegroupdivision = bCompound;
	}

	memset(g_props.comment_string, 0, sizeof(g_props.comment_string));

	kvkeybase_t* comments = script->FindKeyBase("comments");

	if(comments)
		strcpy(g_props.comment_string, KV_GetValueString(comments));

	DkList<dsmvertex_t>		vertices;
	DkList<int>				indices;

	// if we've got ragdoll
	if( forcegroupdivision || (model->bones.numElem() > 1)  )
	{
		// generate index groups
		DkList<indxgroup_t*> indexGroups;
		SubdivideModelParts(vertices, indices, indexGroups);

		// generate ragdoll
		if( model->bones.numElem() > 1 )
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
		modeltype = PHYSMODEL_USAGE_RIGID_COMP;

		// move all vertices and indices from groups to shared buffer (no multiple shapes)
		for(int i = 0 ; i < model->groups.numElem(); i++)
		{
			for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
			{
				indices.append(vertices.numElem());
				vertices.append(model->groups[i]->verts[j]);
				POINT_TO_BBOX(vertices[i].position,g_modelBoxMins,g_modelBoxMaxs);
			}
		}

		Msg("Processed %d verts and %d indices\n", vertices.numElem(), indices.numElem());

		CreateSingleObject( vertices, indices );
	}
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

#pragma warning(disable : 4307) // integral constant overflow

void WritePODFile(const char* filename)
{
	pData = (ubyte*)PPAlloc(MAX_PHYSICSFILE_SIZE);
	
	pStart = pData;

	physmodelhdr_t* pHdr = (physmodelhdr_t*)pData;
	pData += sizeof(physmodelhdr_t);

	pHdr->ident = PHYSMODEL_ID;
	pHdr->version = PHYSMODEL_VERSION;
	pHdr->num_lumps = 0;

	pData = CopyLumpToFile(pData, PHYSLUMP_PROPERTIES,	(ubyte*)&g_props, sizeof(physmodelprops_t));
	pData = CopyLumpToFile(pData, PHYSLUMP_GEOMETRYINFO,(ubyte*)g_geometryinfos.ptr(), sizeof(physgeominfo_t) * g_geometryinfos.numElem());
	pData = CopyLumpToFile(pData, PHYSLUMP_OBJECTS,		(ubyte*)g_physicsobjects.ptr(), sizeof(physobject_t) * g_physicsobjects.numElem());
	pData = CopyLumpToFile(pData, PHYSLUMP_INDEXDATA,	(ubyte*)g_indices.ptr(), sizeof(int) * g_indices.numElem());
	pData = CopyLumpToFile(pData, PHYSLUMP_VERTEXDATA,	(ubyte*)g_vertices.ptr(), sizeof(Vector3D) * g_vertices.numElem());

	pData = CopyLumpToFile(pData, PHYSLUMP_JOINTDATA,	(ubyte*)g_joints.ptr(), sizeof(physjoint_t) * g_joints.numElem());

	pHdr->num_lumps = 6;

	DKFILE* pFile = g_fileSystem->Open(filename, "wb", SP_MOD);

	if(pFile)
	{
		int filesize = pData - pStart;
		pFile->Write(pStart, 1, filesize);

		Msg("Total written bytes: %d\n", filesize);
		g_fileSystem->Close(pFile);
	}
	else
	{
		MsgError("Can't create file %s for writing\n", filename);
	}

	PPFree( pStart );
}

//
// Main
//
void GeneratePhysicsModel( physgenmakeparams_t* params)
{
	if(!params)
		return;

	model = params->pModel;
	script = params->phys_section;
	forcegroupdivision = params->forcegroupdivision;

	if(!model)
	{
		MsgError("No model loaded.\n");
		return;
	}

	MsgInfo("Processing...\n");

	MakeGeometry();

	g_props.model_usage = modeltype;

	WritePODFile( params->outputname );

	Msg("Total:\n");
	Msg("  Vertex count: %d\n", g_vertices.numElem());
	Msg("  Index count: %d\n", g_indices.numElem());
	Msg("  Shape count: %d\n", g_geometryinfos.numElem());
	Msg("  Object count: %d\n", g_physicsobjects.numElem());
	Msg("  Joints count: %d\n", g_joints.numElem());
}