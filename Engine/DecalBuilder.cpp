//******************* Copyright (C) Illusion Way, L.L.C 2011 *********************
//
// Description: DarkTech Engine decal builder
//
// Thanks to donor's (Tenebrae) developers
//
//****************************************************************************

#include "DecalBuilder.h"
#include "Math/math_util.h"
#include "coord.h"
#include "IDebugOverlay.h"

#define DECAL_EPSILON 0.00001f
#define MAX_PATCH_VERTICES

static Plane leftPlane, rightPlane, bottomPlane, topPlane, backPlane, frontPlane;
static bool onBrushOnly;

extern BSP *s_pBSP;

int DecalClipPolygonAgainstPlane(Plane *plane, int vertexCount, Vector3D *vertex, Vector3D *newVertex)
{

	bool negative[MAX_DECAL_VERTICES*16];
	int	a, count, b, c;
	float t;
	Vector3D v, v1, v2;

	// Classify vertices
	int negativeCount = 0;
	for (a = 0; a < vertexCount; a++)
	{
		bool neg = (plane->Distance(vertex[a]) < 0.0);
		negative[a] = neg;
		negativeCount += neg;
	}
	
	// Discard this polygon if it's completely culled
	if (negativeCount == vertexCount)
	{
		return (0);
	}

	count = 0;
	for (b = 0; b < vertexCount; b++)
	{
		// c is the index of the previous vertex
		c = (b != 0) ? b - 1 : vertexCount - 1;
		
		if (negative[b])
		{
			if (!negative[c])
			{
				// Current vertex is on negative side of plane,
				// but previous vertex is on positive side.
				v1 = vertex[c];
				v2 = vertex[b];
				//VectorCopy(vertex[c],v1);
				//VectorCopy(vertex[b],v2);

				t = plane->Distance(v1) / (
					  plane->normal.x * (v1.x - v2.x)
					+ plane->normal.y * (v1.y - v2.y) 
					+ plane->normal.z * (v1.z - v2.z));
				
				newVertex[count] = v1*(1.0 - t);
				VectorMA(newVertex[count],t,v2,newVertex[count]);
				
				count++;
			}
		}
		else
		{
			if (negative[c])
			{
				// Current vertex is on positive side of plane,
				// but previous vertex is on negative side.
				v1 = vertex[b];
				v2 = vertex[c];

				t = plane->Distance(v1) / (
					  plane->normal[0] * (v1[0] - v2[0])
					+ plane->normal[1] * (v1[1] - v2[1])
					+ plane->normal[2] * (v1[2] - v2[2]));

				
				newVertex[count] = v1*(1.0 - t);
				VectorMA(newVertex[count],t,v2,newVertex[count]);
				
				count++;
			}
			
			// Include current vertex
			newVertex[count] = vertex[b];
			count++;
		}
	}
	
	// Return number of vertices in clipped polygon
	return count;
}

int DecalClipPolygon(int vertexCount, Vector3D *vertices, Vector3D *newVertex)
{
	Vector3D tempVertex[MAX_DECAL_VERTICES];
	
	// Clip against all six planes
	int count = DecalClipPolygonAgainstPlane(&leftPlane, vertexCount, vertices, tempVertex);
	if (count != 0)
	{
		count = DecalClipPolygonAgainstPlane(&rightPlane, count, tempVertex, newVertex);
		if (count != 0)
		{
			count = DecalClipPolygonAgainstPlane(&bottomPlane, count, newVertex, tempVertex);
			if (count != 0)
			{
				count = DecalClipPolygonAgainstPlane(&topPlane, count, tempVertex, newVertex);
				if (count != 0)
				{
					count = DecalClipPolygonAgainstPlane(&backPlane, count, newVertex, tempVertex);
					if (count != 0)
					{
						count = DecalClipPolygonAgainstPlane(&frontPlane, count, tempVertex, newVertex);
					}
				}
			}
		}
	}
	
	return count;
}

bool DecalAddPolygon(decal_t *dec, int vertcount, Vector3D *vertices)
{
	uint16 *triangle;
	int count;
	int a, b;

	count = dec->vertexCount;
	if (count + vertcount >= MAX_DECAL_VERTICES)
		return false;
	
	if (dec->triangleCount + vertcount-2 >= MAX_DECAL_TRIANGLES)
		return false;

	// Add polygon as a triangle fan
	triangle = &dec->triangleArray[dec->triangleCount][0];
	for (a = 2; a < vertcount; a++)
	{
		dec->triangleArray[dec->triangleCount][0] = count;
		dec->triangleArray[dec->triangleCount][1] = (count + a - 1);
		dec->triangleArray[dec->triangleCount][2] = (count + a );
		dec->triangleCount++;
	}
	
	// Assign vertex colors
	for (b = 0; b < vertcount; b++)
	{
		dec->vertexArray[count].point = vertices[b];
		count++;
	}
	
	dec->vertexCount = count;
	return true;
}

#define DP_TO_BBOX(p, bmin, bmax)\
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

void DecalClipLeaf(decal_t *dec, bsphwleaf_t *leaf)
{
	Vector3D		newVertex[MAX_DECAL_VERTICES], t1, t2, t3;
	int	c;
	bsphwsurface_t **surf;

	c = leaf->numFaces;
	surf = leaf->firstsurface;

	//for all surfaces in the leaf
	for(c = 0; c < leaf->numFaces; c++, surf++)
	{
		if((*surf)->nFlags & SF_NO_DECALS)
		{
			continue;
		}

		// check for volume intersection of next types
		Volume surface_volume;
		surface_volume.LoadBoundingBox((*surf)->min,(*surf)->max);

		if(!surface_volume.IsSphereInside(dec->origin, dec->radius))
			continue;

		int surface_type = (*surf)->surfaceType;

		if(surface_type == MST_PLANAR)
		{
			int numVerts = (*surf)->planarInfo.numVerts;
			int nFirstVertex = (*surf)->planarInfo.firstVert;

			bsphwvertex_t *verts = (*surf)->planarInfo.polyVerts;

			if(!verts)
				continue;

			// copy normal
			t3 = *(Vector3D*)(*verts).normal;
			for(int i = 0; i < numVerts ; i++, verts++)
			{
				newVertex[i] = (*verts).position;
			}	
			
			//avoid backfacing and ortogonal facing faces to recieve decal parts
			if (dot(dec->normal, t3) > DECAL_EPSILON)
			{
				int count = DecalClipPolygon(numVerts, newVertex, newVertex);
				//Msg("Surface clipped vertices (%d)\n", count);
				if ((count != 0) && (!DecalAddPolygon(dec, count, newVertex))) continue;
			}
		}		
		
		if(!onBrushOnly)
		{
			if((*surf)->surfaceType == MST_PATCH)
			{
				int numVerts = (*surf)->patchInfo.m_patchDraw.m_numVertices;
				int numIndices = (*surf)->patchInfo.m_patchDraw.m_numIndices;

				bsphwvertex_t *verts = (*surf)->patchInfo.m_patchDraw.m_pVertices;
				int *indxs = (*surf)->patchInfo.m_patchDraw.m_pIndices;

				if(dec->triangleCount >= MAX_DECAL_TRIANGLES)
				{
					break;
				}

				for(int i = 0; i < numIndices; i+=3)
				{
					if(i+1 >= numIndices || i+2 >= numIndices)
						break;

					Vector3D v0, v1, v2;
					v0 = verts[indxs[i]].position;
					v1 = verts[indxs[i+1]].position;
					v2 = verts[indxs[i+2]].position;

					Vector3D TriangleBoxMin = Vector3D(MAX_COORD_UNITS);
					Vector3D TriangleBoxMax = Vector3D(-MAX_COORD_UNITS);

					DP_TO_BBOX(v0, TriangleBoxMin,TriangleBoxMax);
					DP_TO_BBOX(v1, TriangleBoxMin,TriangleBoxMax);
					DP_TO_BBOX(v2, TriangleBoxMin,TriangleBoxMax);

					Volume boxfrust;
					boxfrust.LoadBoundingBox(TriangleBoxMin, TriangleBoxMax);

					Vector3D normal = NormalOfTriangle(v0,v1,v2);

					if(dot(dec->normal, normal) > DECAL_EPSILON)
					{
						Vector3D new_verts[8];
						Vector3D clip_verts[] = {v0,v1,v2};

						int new_verts_count = DecalClipPolygon(3,clip_verts, new_verts);
						//Msg("Surface clipped vertices (%d)\n", new_verts_count);
						if(new_verts_count != 0 && (!DecalAddPolygon(dec, new_verts_count, new_verts))) continue;
					}
				}
			}
			else if((*surf)->surfaceType == MST_MESHFACE)
			{
				int numIndices = (*surf)->meshfaceInfo.numMeshIndices;

				bsphwvertex_t *verts = (*surf)->meshfaceInfo.vertsPtr;
				int *indxs = (*surf)->meshfaceInfo.indicesPtr;

				if(dec->triangleCount >= MAX_DECAL_TRIANGLES)
				{
					break;
				}

				for(int i = 0; i < numIndices; i+=3)
				{
					if(i+1 >= numIndices || i+2 >= numIndices)
						break;

					Vector3D v0, v1, v2;
					v0 = verts[indxs[i]].position;
					v1 = verts[indxs[i+1]].position;
					v2 = verts[indxs[i+2]].position;

					Vector3D TriangleBoxMin = Vector3D(MAX_COORD_UNITS);
					Vector3D TriangleBoxMax = Vector3D(-MAX_COORD_UNITS);

					DP_TO_BBOX(v0, TriangleBoxMin, TriangleBoxMax);
					DP_TO_BBOX(v1, TriangleBoxMin, TriangleBoxMax);
					DP_TO_BBOX(v2, TriangleBoxMin, TriangleBoxMax);

					Volume boxfrust;
					boxfrust.LoadBoundingBox(TriangleBoxMin, TriangleBoxMax);

					Vector3D normal = NormalOfTriangle(v0,v1,v2);

					if(dot(dec->normal, normal) > DECAL_EPSILON)
					{
						Vector3D new_verts[8];
						Vector3D clip_verts[] = {v0,v1,v2};
/*
						debugoverlay->Line(v0, v1, ColorRGBA(1,0,0,1), 10);
						debugoverlay->Line(v1, v2, ColorRGBA(1,0,0,1), 10);
						debugoverlay->Line(v2, v0, ColorRGBA(1,0,0,1), 10);
*/
						//debugoverlay->Line(dec->origin, dec->origin + dec->normal, ColorRGBA(1,1,0,1), 10);

						int new_verts_count = DecalClipPolygon(3,clip_verts, new_verts);
						if(new_verts_count != 0 && (!DecalAddPolygon(dec, new_verts_count, new_verts))) continue;
					}
				}
			}
		}
		
	}
}

void DecalWalkBsp_R(decal_t *dec, int nnode, enginebspnode_t *nodes, bsphwleaf_t* leaves)
{
	Plane*				plane;
	float				dist;
	enginebspnode_t*	node;
	bsphwleaf_t*		leaf;

	if(nnode < 0) 
	{
		leaf = &leaves[~nnode];

		//we are in a leaf
		DecalClipLeaf(dec, leaf);
		return;
	}

	node = &nodes[nnode];

	plane = node->plane;
	dist = plane->Distance(dec->origin);
	
	if (dist > dec->radius)
	{
		DecalWalkBsp_R(dec, node->children[0], nodes, leaves);
		return;
	}
	if (dist < -dec->radius)
	{
		DecalWalkBsp_R (dec, node->children[1], nodes, leaves);
		return;
	}

	DecalWalkBsp_R (dec, node->children[0], nodes, leaves);
	DecalWalkBsp_R (dec, node->children[1], nodes, leaves);
}

decal_t *R_SpawnDecal(Vector3D &center, Vector3D &normal, Vector3D &tangent, enginebspnode_t *nodes, bsphwleaf_t* leaves, float radius)
{
	onBrushOnly = false;

	float width, height, depth, d;
	Vector3D binormal;
	decal_t	*dec;
	float one_over_w, one_over_h;
	int	a;
	Vector3D test(0.5, 0.5, 0.5);
	//Vector3D t;

	//allocate decal
	dec = DNew(decal_t);

	test = normalize(test);
	tangent = cross(normal,test);

	//Con_Printf("StartDecail\n");
	dec->origin = center;
	dec->tangent = tangent;
	dec->normal = normal;

	tangent = normalize(tangent);
	normal = normalize(normal);
	binormal = cross(normal, tangent);
	binormal = normalize(binormal);
	
	width = radius;
	height = width;
	depth = width*0.5;

	dec->radius = max(max(width,height),depth);

	// Calculate boundary planes
	d = dot(center, tangent);
	leftPlane.normal = tangent;
	leftPlane.offset = width * 0.5 - d;

	rightPlane.normal = -tangent;
	rightPlane.offset = width * 0.5 + d;
	
	d = dot(center, binormal);

	bottomPlane.normal = binormal;
	bottomPlane.offset = height * 0.5 - d;

	topPlane.normal = -binormal;
	topPlane.offset = height * 0.5 + d;
	
	d = dot(center, normal);
	backPlane.normal = normal;
	backPlane.offset = depth - d;

	frontPlane.normal = -normal;
	frontPlane.offset = depth + d;

	// Begin with empty mesh
	dec->vertexCount = 0;
	dec->triangleCount = 0;
	
	//Clip decal to bsp
	DecalWalkBsp_R(dec, 0, nodes, leaves);

	//This happens when a decal is to far from any surface or the surface is to steeply sloped
	if (dec->triangleCount == 0) 
	{
		DDelete(dec);
		return NULL;
	}

	//Assign texture mapping coordinates
	one_over_w  = 1.0F / width;
	one_over_h = 1.0F / height;

	for (a = 0; a < dec->vertexCount; a++)
	{
		float s, t;
		Vector3D v;
		v = dec->vertexArray[a].point - center;

		s = dot(v, tangent) * one_over_w + 0.5F;
		t = dot(v, binormal) * one_over_h + 0.5F;

		dec->vertexArray[a].texcoord.x = s;
		dec->vertexArray[a].texcoord.y = t;

		dec->vertexArray[a].normal = normal;
	}

	return dec;
}

decal_t *R_SpawnCustomDecal(Vector3D &center, Vector3D &mins, Vector3D &maxs, enginebspnode_t *nodes, bsphwleaf_t* leaves)
{
	onBrushOnly = true;

	float width, height, depth, d;
	Vector3D binormal;
	decal_t	*dec;
	float one_over_w, one_over_h;
	int	a;
	Vector3D test(0, 0, 1);
	//Vector3D t;

	//allocate decal
	dec = DNew(decal_t);

	Vector3D cast_normals[] = 
	{
		Vector3D(1,0,0),
		Vector3D(-1,0,0),

		Vector3D(0,-1,0),
		Vector3D(0,1,0),

		Vector3D(0,0,-1),
		Vector3D(0,0,1),
	};

	// Begin with empty mesh
	dec->vertexCount = 0;
	dec->triangleCount = 0;

	width = (maxs.y-mins.y);
	height = (maxs.z-mins.z);
	depth = (maxs.x-mins.x);

	Vector3D coord_align = maxs-mins;

	dec->radius = length(maxs);

	int nStartVerts = 0;

	for(int i = 0; i < 6; i++)
	{
		Vector3D normal = cast_normals[i];

		test = normalize(test);
		Vector3D tangent = cross(normal,test);

		//Con_Printf("StartDecail\n");
		dec->origin = center;
		dec->tangent = tangent;
		dec->normal = normal;

		tangent = normalize(tangent);
		normal = normalize(normal);
		binormal = cross(normal, tangent);
		binormal = normalize(binormal);

		if(i == 0)
		{
			// Calculate boundary planes
			d = dot(center, tangent);
			leftPlane.normal = tangent;
			leftPlane.offset = width * 0.5 - d;

			rightPlane.normal = -tangent;
			rightPlane.offset = width * 0.5 + d;
			
			d = dot(center, binormal);

			bottomPlane.normal = binormal;
			bottomPlane.offset = height * 0.5 - d;

			topPlane.normal = -binormal;
			topPlane.offset = height * 0.5 + d;
			
			d = dot(center, normal);
			backPlane.normal = normal;
			backPlane.offset = depth * 0.5 - d;

			frontPlane.normal = -normal;
			frontPlane.offset = depth * 0.5 + d;
		}

		//Clip decal to bsp
		DecalWalkBsp_R(dec, 0, nodes, leaves);

		//Assign valid texture mapping coordinates
		one_over_w  = 1.0F / dot(coord_align, tangent);
		one_over_h = 1.0F / dot(coord_align, binormal);

		for (a = nStartVerts; a < dec->vertexCount; a++)
		{
			float s, t;
			Vector3D v;
			v = dec->vertexArray[a].point - center;

			t = dot(v, tangent) * one_over_w + 0.5F;
			s = dot(v, binormal) * one_over_h + 0.5F;

			dec->vertexArray[a].texcoord.x = 1.0f-s;
			dec->vertexArray[a].texcoord.y = 1.0f-t;

			dec->vertexArray[a].normal = normal;
		}

		nStartVerts = dec->vertexCount;
	}

	//This happens when a decal is to far from any surface or the surface is to steeply sloped
	if (dec->triangleCount == 0) 
	{
		DDelete(dec);
		return NULL;
	}

	return dec;
}