//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model shape cache interface
///////////////////////////////////////////////////////////////////////////////////

#ifndef ISTUDIOSHAPECACHE_H
#define ISTUDIOSHAPECACHE_H

#include "egf/model.h"
#include "core/InterfaceManager.h"
#include "core/IDkCore.h"

#define SHAPECACHE_INTERFACE_VERSION		"Physics_StudioShapeCache_001"

class IStudioShapeCache : public IEqCoreModule
{
public:
	virtual ~IStudioShapeCache() {}

	// checks the shape is initialized for the cache
	virtual bool	IsShapeCachePresent( studioPhysShapeCache_t* shapeInfo ) = 0;

	// initializes whole studio shape model with all objects
	virtual void	InitStudioCache( studioPhysData_t* studioData ) = 0;
	virtual void	DestroyStudioCache( studioPhysData_t* studioData ) = 0;

	// does all shape cleanup
	virtual void	Cleanup_Invalidate() = 0;
};


// empty cache
class CEmptyStudioShapeCache : public IStudioShapeCache
{
public:
	CEmptyStudioShapeCache()
	{
		GetCore()->RegisterInterface(SHAPECACHE_INTERFACE_VERSION, this);
	}

	bool			IsInitialized() const { return true; }
	const char*		GetInterfaceName() const { return SHAPECACHE_INTERFACE_VERSION; }

	bool			IsShapeCachePresent( studioPhysShapeCache_t* shapeInfo ) {return false;}

	void			InitStudioCache( studioPhysData_t* studioData )  {}
	void			DestroyStudioCache( studioPhysData_t* studioData )  {}

	void			Cleanup_Invalidate() {}
};

INTERFACE_SINGLETON(IStudioShapeCache, StudioShapeCache, SHAPECACHE_INTERFACE_VERSION, g_studioShapeCache)


#endif // ISTUDIOSHAPECACHE_H