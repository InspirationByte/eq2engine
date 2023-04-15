//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model shape cache interface
///////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

class IStudioShapeCache : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_StudioShapeCache_001")

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
	bool			IsInitialized() const { return true; }

	bool			IsShapeCachePresent( studioPhysShapeCache_t* shapeInfo ) {return false;}

	void			InitStudioCache( studioPhysData_t* studioData )  {}
	void			DestroyStudioCache( studioPhysData_t* studioData )  {}

	void			Cleanup_Invalidate() {}
};

INTERFACE_SINGLETON(IStudioShapeCache, StudioShapeCache, g_studioShapeCache)
