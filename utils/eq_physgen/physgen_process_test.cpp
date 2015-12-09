//**************** Copyright (C) Parallel Prevision, L.L.C 2012 ******************
//
// Description: Equilibrium physics model processor
//
//********************************************************************************

#include "physgen_process.h"
#include "coord.h"

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

// physics
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "utils/geomtools.h"

#include "physmodel.h"

dsmmodel_t*		model = NULL;
kvsection_t*	script = NULL;
bool			forcegroupdivision = false;

int				modeltype = PHYSMODEL_USAGE_RIGID_COMP; // default is rigid or compound mesh

physmodelprops_t			g_props;
DkList<physobject_t>		g_physicsobjects;
DkList<physgeominfo_t>		g_geometryinfos;
DkList<Vector3D>			g_vertices;
DkList<int>					g_indices;
DkList<physjoint_t>			g_joints;

extern Vector3D				g_scalemodel;

int AddShape(DkList<dsmvertex_t> &vertices, DkList<int> &indices, bool convex = true)
{
	physgeominfo_t geom_info;
	geom_info.type = convex ? PHYSSHAPE_TYPE_CONVEX : PHYSSHAPE_TYPE_MOVEABLECONCAVE;

	geom_info.startIndices = g_indices.numElem();

	// proceed to generate physics model
	if(convex)
	{
		// first generate triangle mesh
		btTriangleMesh trimesh(true, false);

		for(int i = 0; i < indices.numElem(); i+=3)
		{
			Vector3D pos1 = vertices[indices[i+2]].position;
			Vector3D pos2 = vertices[indices[i+1]].position;
			Vector3D pos3 = vertices[indices[i]].position;

			trimesh.addTriangle(	btVector3(pos1.x,pos1.y,pos1.z),
									btVector3(pos2.x,pos2.y,pos2.z),
									btVector3(pos3.x,pos3.y,pos3.z),
									true);
		}

		// second is to generate convex mesh
		btConvexTriangleMeshShape tmpConvexShape(&trimesh);

		// make shape hull
		btShapeHull shape_hull(&tmpConvexShape);

		// cook hull
		shape_hull.buildHull(tmpConvexShape.getMargin());

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
	else
	{
		DkList<Vector3D> shapeVerts;

		int start_vertex = g_vertices.numElem();

		for(int i = 0; i < indices.numElem(); i++)
		{
			Vector3D pos = vertices[indices[i]].position;

			int found_idx = shapeVerts.findIndex(pos);
			if(found_idx == -1 && indices[i] != found_idx)
			{
				shapeVerts.append(pos);
				g_indices.append(i + start_vertex);
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

int AddBoneShape(DkList<dsmvertex_t> &vertices, int nBone)
{
	physgeominfo_t geom_info;
	geom_info.type = PHYSSHAPE_TYPE_CONVEX;

	geom_info.startIndices = g_indices.numElem();

	// generate convex mesh
	btConvexHullShape tmpConvexShape;

	for(int i = 0; i < vertices.numElem(); i++)
	{
		if(vertices[i].weight_bones[0] == nBone)
			tmpConvexShape.addPoint( btVector3(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z) );
	}

	// make shape hull
	btShapeHull shape_hull( &tmpConvexShape );

	// cook hull
	shape_hull.buildHull(tmpConvexShape.getMargin());

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

	return g_geometryinfos.append(geom_info);
}

void ProcessModel(const char* filename);

void GeneratePhysicsModel( physgenmakeparams_t* params)
{
	if(params)
	{
		model = params->pModel;
		script = params->phys_section;
		forcegroupdivision = params->forcegroupdivision;

		ProcessModel(params->outputname);
	}
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

void AddTriangleWithAllNeighbours(indxgroup_t *group, NBtriangle_t* triangle)
{
	itriangle first = {triangle->triangle[0], triangle->triangle[1], triangle->triangle[2]};

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

		for(int i = 0; i < triangle->neighbours.numElem(); i++)
		{
			AddTriangleWithAllNeighbours(group, triangle->neighbours[i]); 
		}
	}
}

struct tempjoint_t
{
	Matrix4x4 localTrans;
	Matrix4x4 absTrans;
};

DkList<tempjoint_t> temp_joints;

void SetupBones()
{
	for(int i = 0; i < model->bones.numElem(); i++)
	{
		tempjoint_t joint;

		temp_joints.append(joint);
	}

	// setup each bone's transformation
	for(int i = 0; i < model->bones.numElem(); i++)
	{
		tempjoint_t* joint = &temp_joints[i];
		dsmskelbone_t* bone = model->bones[i];

		// setup transformation
		joint->localTrans = identity4();

		joint->localTrans.setRotation(bone->angles);
		joint->localTrans.setTranslation(bone->position);

		if(bone->parent_id != -1)
			joint->absTrans = joint->localTrans*temp_joints[bone->parent_id].absTrans;
		else
			joint->absTrans = joint->localTrans;
	}
}

struct jointmergedata_t
{
	int real_index;
	int parent_index;
};

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

ColorRGB UTIL_StringToColor3(const char *str)
{
	ColorRGB col(0,0,0);

	if(str)
		sscanf(str, "%f %f %f", &col.x, &col.y, &col.z);

	return col;
}

void MakeGeometry()
{
	kvsection_t* pMainSec = NULL;
	
	if(script)
		pMainSec = script;

	bool bCompound = false;

	if(model->bones.numElem() > 0)
		forcegroupdivision = true;

	kvkeyvaluepair_t* groupdivision = KeyValues_FindKey(pMainSec, "groupdivision");

	if(groupdivision)
		forcegroupdivision = KeyValuePair_GetValueBool(groupdivision);

	kvkeyvaluepair_t* compoundkey = KeyValues_FindKey(pMainSec, "compound");
	if(compoundkey)
	{
		bCompound = KeyValuePair_GetValueBool(compoundkey);
		forcegroupdivision = bCompound;
	}

	memset(g_props.comment_string, 0, sizeof(g_props.comment_string));

	kvkeyvaluepair_t* comments = KeyValues_FindKey(pMainSec, "comments");

	if(comments)
		strcpy(g_props.comment_string, KeyValuePair_GetValueString(comments));
	

	DkList<dsmvertex_t>		vertices;
	DkList<int>				indices;

	if(forcegroupdivision)
	{
		DkList<NBtriangle_t>	triangle_table;

		Vector3D modelBoxMins(MAX_COORD_UNITS);
		Vector3D modelBoxMaxs(-MAX_COORD_UNITS);

		for(int i = 0 ; i < model->groups.numElem(); i++)
		{
			vertices.resize(vertices.numElem() + model->groups[i]->verts.numElem());
			indices.resize(indices.numElem() + model->groups[i]->verts.numElem());

			for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
			{
				indices.append(vertices.numElem());
				vertices.append(model->groups[i]->verts[j]);
				POINT_TO_BBOX(vertices[i].position,modelBoxMins,modelBoxMaxs)
			}
		}

		/*
		// connect same vertices
		for(int i = 0; i < indices.numElem(); i++)
		{
			int index = indices[i];
			dsmvertex_t vertex = vertices[index];

			int found_index = find_vertex(vertices, vertex);

			if(found_index != i && found_index != -1)
				indices[i] = found_index;
		}
		*/
		
		/*
		Msg("Building neighbour triangle table... (%d indices)\n", indices.numElem());

		// build neighbours
		BuildNeighbourTable(indices.ptr(), indices.numElem(), &triangle_table);

		Msg("Num. triangles parsed: %d\n", triangle_table.numElem());
		*/
		// sort triangles to new groups
		DkList<indxgroup_t*> allgroups;

		indxgroup_t *main_group = new indxgroup_t;
		allgroups.append(main_group);

		/*
		Msg("Building groups...\n");

		// add tri with all of it's neighbour's herarchy
		AddTriangleWithAllNeighbours(main_group, &triangle_table[0]);

		Msg("Processed group 1, %d tris\n", main_group->numElem());

		for(int i = 1; i < triangle_table.numElem(); i++)
		{
			itriangle triangle = {triangle_table[i].triangle[0],triangle_table[i].triangle[1],triangle_table[i].triangle[2]};

			bool found = false;

			// find this triangle in all previous groups
			for(int j = 0; j < allgroups.numElem(); j++)
			{
				if(find_triangle(allgroups[j], triangle) != -1)
				{
					found = true;
					break;
				}
			}

			// if not found, create new group and add triangle with all of it's neighbours
			if(!found)
			{
				indxgroup_t *new_group = new indxgroup_t;
				allgroups.append(new_group);

				// add tri with all of it's neighbour's herarchy
				AddTriangleWithAllNeighbours(new_group, &triangle_table[i]);

				Msg("Processed group %d, %d tris\n", allgroups.numElem(), new_group->numElem());
			}
		}

		MsgInfo("Detected groups: %d\n", allgroups.numElem());
		*/

		// get shared surfaceprops parameter
		kvkeyvaluepair_t* surfMainPropsPair = KeyValues_FindKey(pMainSec, "SurfaceProps");

		DkList<jointmergedata_t> joint_merge_data;

		if(model->bones.numElem() > 0)
		{
			// setup bones for it
			SetupBones();

			kvsection_t* pBonesSect = KeyValues_FindSectionByName(pMainSec, "Bones");

			kvkeyvaluepair_t* pair = KeyValues_FindKey(pMainSec, "IsDynamic");

			if(KeyValuePair_GetValueBool(pair))
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

			/*
			DkList<int> bone_group_indices;

			for(int i = 0; i < allgroups.numElem(); i++)
			{
				int firsttri_indx0 = allgroups[i]->ptr()[0].idxs[0];
				int bone_index = vertices[firsttri_indx0].weight_bones[0];

				if(bone_index != -1)
					Msg("Group %d uses bone %s\n", i+1, model->bones[bone_index]->name);
				else
					Msg("Group %d doesn't use bones, it will be static\n", i+1);

				bone_group_indices.append(bone_index);

				for(int j = 1; j < allgroups[i]->numElem(); j++)
				{
					int idx0 = allgroups[i]->ptr()[j].idxs[0];
					int idx1 = allgroups[i]->ptr()[j].idxs[1];
					int idx2 = allgroups[i]->ptr()[j].idxs[2];

					if( vertices[idx0].weight_bones[0] != bone_index ||
						vertices[idx1].weight_bones[0] != bone_index ||
						vertices[idx2].weight_bones[0] != bone_index)
					{
						MsgError("Invalid bone id. Mesh part must use single bone index.\n");
						MsgError("Please separate model parts for bones.\n");
						exit(0);
					}
						
				}
			}
			*/

			for(int i = 0; i < model->bones.numElem(); i++)
			{
				DkList<int> bone_geom_indices;

				Vector3D bboxMins(MAX_COORD_UNITS);
				Vector3D bboxMaxs(-MAX_COORD_UNITS);

				/*
				// add indices of attached groups and also build bounding box
				for(int j = 0; j < allgroups.numElem(); j++)
				{
					if(bone_group_indices[j] == i)
					{
						for(int k = 0; k < allgroups[j]->numElem(); k++)
						{
							bone_geom_indices.append(allgroups[j]->ptr()[k].idxs[0]);
							bone_geom_indices.append(allgroups[j]->ptr()[k].idxs[1]);
							bone_geom_indices.append(allgroups[j]->ptr()[k].idxs[2]);

							POINT_TO_BBOX(vertices[allgroups[j]->ptr()[k].idxs[0]].position, bboxMins, bboxMaxs);
							POINT_TO_BBOX(vertices[allgroups[j]->ptr()[k].idxs[1]].position, bboxMins, bboxMaxs);
							POINT_TO_BBOX(vertices[allgroups[j]->ptr()[k].idxs[2]].position, bboxMins, bboxMaxs);
						}
					}
				}
				*/

				for(int j = 0; j < vertices.numElem(); j++)
				{
					if(vertices[j].weight_bones[0] == i)
						POINT_TO_BBOX(vertices[j].position, bboxMins, bboxMaxs);
				}

				Vector3D size = bboxMins-bboxMaxs;

				// don't allow small bones
				if(length(size) < 2.0f)
					continue;

				// if we have only one triangle, so continue to the next parts
				//if(bone_geom_indices.numElem() <= 3)
				//	continue;

				// compute object center
				Vector3D object_center = (bboxMins+bboxMaxs)*0.5f;
				
				// setup vertices by bone transform
				/*
				DkList<int> processed_index;
				for(int j = 0; j < bone_geom_indices.numElem(); j++)
				{
					if(processed_index.findIndex(bone_geom_indices[j])==-1)
					{
						// multiply vector by matrix for transformation
						//vertices[bone_geom_indices[j]].m_vPoint = inverseTranslateVec(vertices[bone_geom_indices[j]].m_vPoint, temp_joints[i].absTrans);
						//vertices[bone_geom_indices[j]].m_vPoint = inverseRotateVec(vertices[bone_geom_indices[j]].m_vPoint, temp_joints[i].absTrans);

						vertices[bone_geom_indices[j]].position -= object_center;
						processed_index.append(bone_geom_indices[j]);
					}
				}
				*/

				for(int j = 0; j < vertices.numElem(); j++)
				{
					if(vertices[j].weight_bones[0] == i)
					{
						vertices[j].position -= object_center;
					}
				}
				
				// generate physics shape
				Msg("Adding bone shape %d\n", i);
				int shapeID = AddBoneShape(vertices, i);//AddShape(vertices, bone_geom_indices);

				// build object data
				physobject_t object;

				kvsection_t* pThisBoneSec = KeyValues_FindSectionByName(pBonesSect, model->bones[i]->name);
				kvkeyvaluepair_t* pair = KeyValues_FindKey(pThisBoneSec, "Mass");
				kvkeyvaluepair_t* surfPropsPair = KeyValues_FindKey(pThisBoneSec, "SurfaceProps");
				
				memset(object.surfaceprops, 0, 0);

				if(!surfPropsPair)
				{
					if(surfMainPropsPair)
						strcpy(object.surfaceprops, KeyValuePair_GetValueString(surfMainPropsPair));
					else
						strcpy(object.surfaceprops, "default");
				}
				else
					strcpy(object.surfaceprops, KeyValuePair_GetValueString(surfPropsPair));

				memset(object.shape_indexes, -1, sizeof(object.shape_indexes));

				if(pair)
					object.mass = KeyValuePair_GetValueFloat(pair);
				else
					object.mass = DEFAULT_MASS;

				object.numShapes = 1;
				object.shape_indexes[0] = shapeID;
				object.offset = object_center;
				object.mass_center = vec3_zero;

				// add object after building
				g_physicsobjects.append(object);

				// build joint information
				physjoint_t joint;
				memset(joint.name,0,sizeof(joint.name));
				strcpy(joint.name, model->bones[i]->name);

				// setup default limits
				joint.minLimit = vec3_zero;
				joint.maxLimit = vec3_zero;

				// set bone position
				Vector3D bone_position = temp_joints[i].absTrans.rows[3].xyz();
				joint.position = bone_position;

				joint.object_indexA = g_physicsobjects.numElem() - 1;/* - numskipped*/;

				int parent = model->bones[i]->parent_id;

				if(parent == -1)
				{
					// join to itself
					joint.object_indexB = joint.object_indexA;
				}
				else
				{
					int phys_parent = make_valid_parent(i);//find_joint(model->pBone(parent)->m_szName);

					if(phys_parent == -1)
						joint.object_indexB = joint.object_indexA;
					else
						joint.object_indexB = phys_parent;
				}

				// get limit parameters of joints
				kvkeyvaluepair_t *pXParams = KeyValues_FindKey(pThisBoneSec, "x_axis");
				if(pXParams)
				{
					float lo_limit = 0;
					float hi_limit = 0;

					float offs = 0;

					sscanf(pXParams->value, "limit %f %f %f", &lo_limit, &hi_limit, &offs);

					joint.minLimit.x = DEG2RAD(lo_limit+offs);
					joint.maxLimit.x = DEG2RAD(hi_limit+offs);
				}

				kvkeyvaluepair_t *pYParams = KeyValues_FindKey(pThisBoneSec, "y_axis");
				if(pYParams)
				{
					float lo_limit = 0;
					float hi_limit = 0;

					float offs = 0;

					sscanf(pYParams->value, "limit %f %f %f", &lo_limit, &hi_limit, &offs);

					joint.minLimit.y = DEG2RAD(lo_limit+offs);
					joint.maxLimit.y = DEG2RAD(hi_limit+offs);
				}

				kvkeyvaluepair_t *pZParams = KeyValues_FindKey(pThisBoneSec, "z_axis");
				if(pZParams)
				{
					float lo_limit = 0;
					float hi_limit = 0;

					float offs = 0;

					sscanf(pZParams->value, "limit %f %f %f", &lo_limit, &hi_limit, &offs);

					joint.minLimit.z = DEG2RAD(lo_limit+offs);
					joint.maxLimit.z = DEG2RAD(hi_limit+offs);
				}

				// add new joint
				g_joints.append(joint);
			}

			// next
		}
		else // make compound model
		{
			modeltype = PHYSMODEL_USAGE_RIGID_COMP;

			if(allgroups.numElem() == 1)
				Msg("  Model is single\n");
			else
				Msg("  Model is compound\n");

			if(bCompound)
			{
				DkList<int> shape_ids;

				for(int i = 0; i < allgroups.numElem(); i++)
				{
					DkList<int> tmpIndices;

					for(int j = 0; j < allgroups[i]->numElem(); j++)
					{
						tmpIndices.append(allgroups[i]->ptr()[j].idxs[0]);
						tmpIndices.append(allgroups[i]->ptr()[j].idxs[1]);
						tmpIndices.append(allgroups[i]->ptr()[j].idxs[2]);
					}

					int shapeID = AddShape(vertices, tmpIndices);
					shape_ids.append(shapeID);
				}

				physobject_t object;

				kvkeyvaluepair_t* surfPropsPair = KeyValues_FindKey(pMainSec, "SurfaceProps");
				
				memset(object.surfaceprops, 0, 0);

				if(!surfPropsPair)
					strcpy(object.surfaceprops, "default");
				else
					strcpy(object.surfaceprops, KeyValuePair_GetValueString(surfPropsPair));

				kvkeyvaluepair_t* pair = KeyValues_FindKey(pMainSec, "Mass");

				if(pair)
					object.mass = KeyValuePair_GetValueFloat(pair);
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

				pair = KeyValues_FindKey(pMainSec, "MassCenter");
				if(pair)
					object.mass_center = UTIL_StringToColor3(pair->value);
				else
					object.mass_center = (modelBoxMins+modelBoxMaxs)*0.5f;

				g_physicsobjects.append(object);
			}
			else
			{
				for(int i = 0; i < allgroups.numElem(); i++)
				{
					DkList<int> tmpIndices;

					for(int j = 0; j < allgroups[i]->numElem(); j++)
					{
						tmpIndices.append(allgroups[i]->ptr()[j].idxs[0]);
						tmpIndices.append(allgroups[i]->ptr()[j].idxs[1]);
						tmpIndices.append(allgroups[i]->ptr()[j].idxs[2]);
					}

					int shapeID = AddShape(vertices, tmpIndices);
					physobject_t object;

					kvkeyvaluepair_t* surfPropsPair = KeyValues_FindKey(pMainSec, "SurfaceProps");
				
					memset(object.surfaceprops, 0, 0);

					if(!surfPropsPair)
						strcpy(object.surfaceprops, "default");
					else
						strcpy(object.surfaceprops, KeyValuePair_GetValueString(surfPropsPair));

					kvkeyvaluepair_t* pair = KeyValues_FindKey(pMainSec, "Mass");


					if(pair)
						object.mass = KeyValuePair_GetValueFloat(pair);
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
	}
	else
	{
		modeltype = PHYSMODEL_USAGE_RIGID_COMP;

		Vector3D modelBoxMins(MAX_COORD_UNITS);
		Vector3D modelBoxMaxs(-MAX_COORD_UNITS);

		// move all vertices and indices from groups to shared buffer (no multiple shapes)
		for(int i = 0 ; i < model->groups.numElem(); i++)
		{
			for(int j = 0; j < model->groups[i]->verts.numElem(); j++)
			{
				indices.append(vertices.numElem());
				vertices.append(model->groups[i]->verts[j]);
				POINT_TO_BBOX(vertices[i].position,modelBoxMins,modelBoxMaxs);
			}
		}

		Msg("Processed %d verts and %d indices\n", vertices.numElem(), indices.numElem());

		// generate big shape in this case
		int shapeID = AddShape(vertices, indices);

		physobject_t object;

		kvkeyvaluepair_t* surfPropsPair = KeyValues_FindKey(pMainSec, "SurfaceProps");
		
		memset(object.surfaceprops, 0, 0);

		if(!surfPropsPair)
			strcpy(object.surfaceprops, "default");
		else
			strcpy(object.surfaceprops, KeyValuePair_GetValueString(surfPropsPair));

		kvkeyvaluepair_t* pair = KeyValues_FindKey(pMainSec, "Mass");

		if(pair)
			object.mass = KeyValuePair_GetValueFloat(pair);
		else
			object.mass = DEFAULT_MASS;

		pair = KeyValues_FindKey(pMainSec, "MassCenter");
		if(pair)
			object.mass_center = UTIL_StringToColor3(pair->value);
		else
			object.mass_center = (modelBoxMins+modelBoxMaxs)*0.5f;

		object.numShapes = 1;
		memset(object.shape_indexes, -1, sizeof(object.shape_indexes));
		object.shape_indexes[0] = shapeID;
		object.offset = vec3_zero;

		g_physicsobjects.append(object);
	}
}

/*
modelheader_t*	model = NULL;
KeyValues*		script = NULL;
bool			forcegroupdivision = false;

int				modeltype = PHYSMODEL_USAGE_RIGID_COMP; // default is rigid or compound mesh

physmodelprops_t			g_props;
DkList<physobject_t>		g_physicsobjects;
DkList<physgeominfo_t>		g_geometryinfos;
DkList<Vector3D>			g_vertices;
DkList<int>					g_indices;
DkList<physjoint_t>			g_joints;
*/

#define MAX_PHYSICSFILE_SIZE 4096*1024*1024

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
	int marker = HunkGetLowMark();
	pData = (ubyte*)HunkAlloc(MAX_PHYSICSFILE_SIZE);
	
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

	DKFILE* pFile = GetFileSystem()->Open(filename, "wb", SP_MOD);

	if(pFile)
	{
		int filesize = pData - pStart;
		GetFileSystem()->Write(pStart, 1, filesize, pFile);

		Msg("Total written bytes: %d\n", filesize);
		GetFileSystem()->Close(pFile);
	}
	else
	{
		MsgError("Can't create file %s for writing\n", filename);
	}

	HunkFreeToLowMark(marker);
}

void ProcessModel(const char* filename)
{
	if(!model)
	{
		MsgError("No model loaded.\n");
		return;
	}

	MsgInfo("Processing...\n");
	MakeGeometry();

	g_props.model_usage = modeltype;

	WritePODFile(filename);

	Msg("Total:\n");
	Msg("  Vertex count: %d\n", g_vertices.numElem());
	Msg("  Index count: %d\n", g_indices.numElem());
	Msg("  Shape count: %d\n", g_geometryinfos.numElem());
	Msg("  Object count: %d\n", g_physicsobjects.numElem());
	Msg("  Joints count: %d\n", g_joints.numElem());
}