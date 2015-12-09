//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Billboard list file and renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef BILLBOARDLIST_H
#define BILLBOARDLIST_H

#include "EqParticles.h"
#include "TextureAtlas.h"

struct blbsprite_t
{
	TexAtlasEntry_t*	entry;		// texture atlas entry
	Vector3D			position;	// sprite position
	float				scale;		// sprite size
	float				distfactor;	// distance disappearrance factor
};

class CBillboardList
{
public:
	PPMEM_MANAGED_OBJECT();

	CBillboardList();
	virtual ~CBillboardList();

	void					LoadBlb( kvkeybase_t* kvs );
	void					SaveBlb( const char* filename );

	void					Generate( Vector3D* pointList, int numPoints, float minSize, float maxSize );

	void					DestroyBlb();

	virtual void			DrawBillboards(); // draws billboards with applied matrices

	CPFXAtlasGroup*			m_renderGroup;	// used render group, only for reference!!!

	EqString				m_name;

protected:

	DkList<blbsprite_t>		m_sprites;

	BoundingBox				m_aabb;
};

#ifdef TREEGEN
bool LoadObjAsPointList(const char* filename, DkList<Vector3D>& outPoints);
#endif // TREEGEN

#endif // BILLBOARDLIST_H