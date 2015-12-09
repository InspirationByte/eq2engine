//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model shape cache interface
///////////////////////////////////////////////////////////////////////////////////

#ifndef ISTUDIOSHAPECACHE_H
#define ISTUDIOSHAPECACHE_H

#include "model.h"

class IStudioShapeCache
{
public:
	virtual ~IStudioShapeCache() {}

	// checks the shape is initialized for the cache
	virtual bool	IsShapeCachePresent( physmodelshapecache_t* shapeInfo ) = 0;

	// initializes whole studio shape model with all objects
	virtual void	InitStudioCache( physmodeldata_t* studioData ) = 0;
	virtual void	DestroyStudioCache( physmodeldata_t* studioData ) = 0;

	// does all shape cleanup
	virtual void	Cleanup_Invalidate() = 0;
};

extern IStudioShapeCache* g_pStudioShapeCache;


// empty cache
class CEmptyStudioShapeCache : public IStudioShapeCache
{
public:
	// checks the shape is initialized for the cache
	bool	IsShapeCachePresent( physmodelshapecache_t* shapeInfo ) {return false;}

	// initializes whole studio shape model with all objects
	void	InitStudioCache( physmodeldata_t* studioData )  {}
	void	DestroyStudioCache( physmodeldata_t* studioData )  {}

	// does all shape cleanup
	void	Cleanup_Invalidate() {}
};

#endif // ISTUDIOSHAPECACHE_H