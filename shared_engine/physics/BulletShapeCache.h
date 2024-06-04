//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model cache for bullet physics
//				Generates real shapes for Bullet Collision
///////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "physics/IStudioShapeCache.h"

class btCollisionShape;

class CBulletStudioShapeCache : public IStudioShapeCache
{
public:
	CBulletStudioShapeCache();

	bool						IsInitialized() const;

	// checks the shape is initialized for the cache
	bool						IsShapeCachePresent( StudioPhyShapeData* shapeInfo );

	// initializes whole studio shape model with all objects
	void						InitStudioCache( StudioPhysData* studioData );
	void						DestroyStudioCache( StudioPhysData* studioData );

	// does all shape cleanup
	void						Cleanup_Invalidate();

protected:

	// cached shapes
	Array<btCollisionShape*>	m_collisionShapes{ PP_SL };
};
