//**************** Copyright (C) Parallel Prevision, L.L.C 2012 ******************
//
// Description: EQWC BSP definitions
//
//				Equilibrium Engine uses BSP only for building portals and
//				use visibility. Because we create world geometry in external editors,
//				engine needs additional information about spacial subdivision.
//
//********************************************************************************

#include "eqwc.h"
#include "eqwc_bsp.h"

DkList<Plane> g_Planes; 

// makes face list to build BSP
bspbrushface_t* CreateBrushFaceList()
{
	bspbrushface_t* faceList = NULL;

	cwbrush_t* list = g_pBrushList;

	g_Planes.resize(g_numBrushes*32);

	for ( ; list ; list = list->next )
	{
		cwbrush_t* pBrush = list;

		for ( int j = 0 ; j < pBrush->numfaces ; j++ )
		{
			cwbrushface_t* pBrushFace = pBrush->faces;

			if(!pBrushFace->winding)
				continue;

			// TODO: area portal materials

			bspbrushface_t* face = new bspbrushface_t;

			face->winding = CopyWinding(pBrushFace->winding);
			face->face_id = g_Planes.append(pBrushFace->Plane);
			face->next = faceList;

			pBrushFace->planenum = face->face_id;

			faceList = face;
		}
	}

	return faceList;
}

// allocates tree
tree_t* AllocTree()
{
	tree_t	*tree;

	tree = new tree_t;
	memset (tree, 0, sizeof(*tree));
	tree->bounds.Reset();

	return tree;
}

// allocates node
node_t* AllocNode()
{
	node_t* pNode = new node_t;
	memset(pNode, 0, sizeof(node_t));

	return pNode;
}

int c_faceLeafs = 0;

// builds bsp faces
tree_t* BuildBSPFaceTree( bspbrushface_t *list )
{
	tree_t* tree = AllocTree();

	// compute tree bounds
	int count = 0;
	for ( bspbrushface_t* face = list ; face ; face = face->next )
	{
		count++;

		for ( int i = 0 ; i < face->winding->numVerts ; i++ )
			tree->bounds.AddVertex( face->winding->vertices[i].position);
	}

	Msg("BSP faces: %d\n", count);

	tree->headnode = AllocNode();
	tree->headnode->bounds = tree->bounds;
	c_faceLeafs = 0;

	BuildFaceTree_r(tree->headnode, list);

	Msg("face leafs: %d\n", c_faceLeafs);

	Msg("BSP Tree successfully buildt\n");

	return tree;
}

ClassifySide_e ClassifySide(brushwinding_t* pWinding, Plane &plane)
{
	bool	bFront = false, bBack = false;
	float	dist;

	for ( int i = 0; i < pWinding->numVerts; i++ )
	{
		dist = plane.Distance(pWinding->vertices[i].position);

		if ( dist > 0.01f )
		{
			if ( bBack )
				return CS_CROSS;

			bFront = true;
		}
		else if ( dist > -0.01f )
		{
			if ( bFront )
				return CS_CROSS;

			bBack = true;
		}
	}

	if ( bFront )
		return CS_FRONT;
	else if ( bBack )
		return CS_BACK;

	return CS_ONPLANE;
}

#define	NORMAL_EPSILON			0.00001f
#define	DIST_EPSILON			0.01f

int FindFloatPlane( const Plane &plane )
{
	for(int i = 0; i < g_Planes.numElem(); i++)
	{
		if(g_Planes[i].CompareEpsilon(plane, DIST_EPSILON, NORMAL_EPSILON))
			return i;
	}

	return -1;
}

#define	BLOCK_SIZE	1024
int SelectSplitPlaneNum( node_t *node, bspbrushface_t *list )
{
	bspbrushface_t*	split;
	bspbrushface_t*	check;
	bspbrushface_t*	bestSplit;

	int			splits, facing, front, back;
	Plane		plane;
	int			planenum;
	
	
	// if it is crossing a 1k block boundary, force a split
	// this prevents epsilon problems from extending an
	// arbitrary distance across the map

	Vector3D halfSize = node->bounds.GetSize()*0.5f;

	float dist = 0.0f;
	int	value, bestValue;
	//bool havePortals = false;

	/*
	for ( int axis = 0; axis < 3; axis++ )
	{
		if ( halfSize[axis] > BLOCK_SIZE )
			dist = BLOCK_SIZE * ( floor( ( node->bounds.minPoint[axis] + halfSize[axis] ) / BLOCK_SIZE ) + 1.0f );
		else
			dist = BLOCK_SIZE * ( floor( node->bounds.minPoint[axis] / BLOCK_SIZE ) + 1.0f );

		if ( dist > node->bounds.minPoint[axis] + 1.0f && dist < node->bounds.maxPoint[axis] - 1.0f )
		{
			plane.normal = vec3_zero;
			plane.normal[axis] = 1.0f;
			plane.offset = dist;

			planenum = FindFloatPlane( plane );

			return planenum;
		}
	}
	*/


	// pick one of the face planes
	// if we have any portal faces at all, only
	// select from them, otherwise select from
	// all faces
	bestValue = -999999;
	bestSplit = list;

	
	for ( split = list ; split ; split = split->next )
	{
		split->checked = false;
		//if ( split->portal )
		//	havePortals = true;
	}
	

	for ( split = list ; split ; split = split->next )
	{
		if ( split->checked )
			continue;

		//if ( havePortals != split->portal )
		//	continue;

		Plane* mapPlane = &g_Planes[ split->face_id ];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;
		for ( check = list ; check ; check = check->next )
		{
			if ( check->face_id == split->face_id )
			{
				facing++;
				check->checked = true;	// won't need to test this plane again
				continue;
			}

			int side = ClassifySide(check->winding, *mapPlane );

			if ( side == CS_CROSS )
				splits++;
			else if ( side == CS_FRONT )
				front++;
			else if ( side == CS_BACK )
				back++;

		}
		value =  5*facing - 5*splits; // - abs(front-back);

		//if ( mapPlane->Type() < PLANETYPE_TRUEAXIAL )
		//	value+=5;		// axial is better

		if ( value > bestValue )
		{
			bestValue = value;
			bestSplit = split;
		}
	}

	if ( bestValue == -999999 )
	{
		return -1;
	}

	return bestSplit->face_id;
}

void BuildFaceTree_r( node_t *node, bspbrushface_t *list )
{
	bspbrushface_t*	split;
	bspbrushface_t*	next;
	ClassifySide_e	side;
	bspbrushface_t*	newFace;
	bspbrushface_t*	childLists[2];
	brushwinding_t	*frontWinding, *backWinding;

	// get the best plane that will do the splitting
	int splitPlaneNum = SelectSplitPlaneNum( node, list );

	// if we don't have any more faces, this is a node
	if ( splitPlaneNum == -1 )
	{
		node->planenum = PLANE_LEAF;

		c_faceLeafs++;
		return;
	}

	// partition the list
	node->planenum = splitPlaneNum;
	Plane &plane = g_Planes[ splitPlaneNum ];

	childLists[0] = NULL;
	childLists[1] = NULL;

	for ( split = list ; split ; split = next )
	{
		next = split->next;

		if ( split->face_id == node->planenum )
		{
			if(split->winding)
				FreeWinding(split->winding);

			delete split;
			continue;
		}

		side = ClassifySide(split->winding, plane );

		if ( side == CS_CROSS )
		{
			Split(split->winding, plane, &frontWinding, &backWinding );
			if ( frontWinding )
			{
				newFace = new bspbrushface_t;
				newFace->winding = frontWinding;
				newFace->next = childLists[0];
				newFace->face_id = split->face_id;
				childLists[0] = newFace;
			}
			if ( backWinding )
			{
				newFace = new bspbrushface_t;

				newFace->winding = backWinding;
				newFace->next = childLists[1];
				newFace->face_id = split->face_id;
				childLists[1] = newFace;
			}

			if(split->winding)
				FreeWinding(split->winding);

			delete split;
		}
		else if ( side == CS_FRONT )
		{
			split->next = childLists[0];
			childLists[0] = split;
		}
		else if ( side == CS_BACK )
		{
			split->next = childLists[1];
			childLists[1] = split;
		}
	}

	// recursively process children
	for ( int i = 0 ; i < 2 ; i++ )
	{
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for ( int i = 0 ; i < 3 ; i++ ) 
	{
		if ( fabs( plane.normal[i] - 1.0 ) < 0.001 )
		{
			node->children[0]->bounds.minPoint[i] = plane.offset;
			node->children[1]->bounds.maxPoint[i] = plane.offset;
			break;
		}
	}

	for ( int i = 0 ; i < 2 ; i++ )
		BuildFaceTree_r( node->children[i], childLists[i]);
}

void ClipFaceByTree_r( brushwinding_t *w, cwbrushface_t *side, node_t *node )
{
	if(!w)
		return;

	brushwinding_t *front, *back;

	if ( node->planenum != PLANE_LEAF )
	{
		if ( side->planenum == node->planenum )
		{
			ClipFaceByTree_r( w, side, node->children[0] );
			return;
		}
		if ( side->planenum == ( node->planenum ^ 1) )
		{
			ClipFaceByTree_r( w, side, node->children[1] );
			return;
		}

		Split(w, g_Planes[ node->planenum ], &front, &back );
		FreeWinding(w);

		ClipFaceByTree_r( front, side, node->children[0] );
		ClipFaceByTree_r( back, side, node->children[1] );

		return;
	}

	// if opaque leaf, don't add
	
	if ( !node->opaque )
	{
		if ( !side->visibleHull )
			side->visibleHull = CopyWinding(w);
		//else
		//	side->visibleHull->AddToConvexHull( w, dmapGlobals.mapPlanes[ side->planenum ].Normal() );
	}

	delete w;
}

void ClipFacesByTree(tree_t* pTree)
{
	cwbrush_t* pBrush;
	for ( pBrush = g_pBrushList ; pBrush ; pBrush = pBrush->next )
	{
		for ( int i = 0 ; i < pBrush->numfaces ; i++ )
		{
			cwbrushface_t* face = &pBrush->faces[i];

			if ( !face->winding)
				continue;

			brushwinding_t* w = CopyWinding(face->winding);
			face->visibleHull = NULL;

			ClipFaceByTree_r( w, face, pTree->headnode );

			/*
			// for debugging, we can choose to use the entire original side
			// but we skip this if the side was completely clipped away
			if ( face->visibleHull)
			{
				FreeWinding(face->visibleHull);
				face->visibleHull = CopyWinding(face->winding);
			}*/
			
		}
	}
}